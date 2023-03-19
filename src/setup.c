#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "core/cp.h"
#include "core/log.h"
#include "core/sysinfo.h"
#include "core/sha256sum.h"
#include "core/base16.h"
#include "core/tar.h"
#include "core/exe.h"
#include "core/self.h"
#include "core/rm-r.h"
#include "core/exe.h"
#include "autotools-setup.h"

#define LOG_STEP(output2Terminal, logLevel, stepN, msg) \
    if (logLevel != AutotoolsSetupLogLevel_silent) { \
        if (output2Terminal) { \
            fprintf(stderr, "\n%s=>> STEP %u : %s%s\n", COLOR_PURPLE, stepN, msg, COLOR_OFF); \
        } else { \
            fprintf(stderr, "\n=>> STEP %u : %s\n", stepN, msg); \
        } \
    }

#define LOG_RUN_CMD(output2Terminal, logLevel, cmd) \
    if (logLevel != AutotoolsSetupLogLevel_silent) { \
        if (output2Terminal) { \
            fprintf(stderr, "%s==>%s %s%s%s\n", COLOR_PURPLE, COLOR_OFF, COLOR_GREEN, cmd, COLOR_OFF); \
        } else { \
            fprintf(stderr, "==> %s\n", cmd); \
        } \
    }

static int run_cmd(char * cmd, int redirectOutput2FD) {
    pid_t pid = fork();

    if (pid < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (pid == 0) {
        if (redirectOutput2FD > 0) {
            if (dup2(redirectOutput2FD, STDOUT_FILENO) < 0) {
                perror(NULL);
                exit(125);
            }

            if (dup2(redirectOutput2FD, STDERR_FILENO) < 0) {
                perror(NULL);
                exit(126);
            }
        }

        ////////////////////////////////////////

        size_t argc = 0;
        char* argv[10] = {0};

        char * arg = strtok(cmd, " ");

        while (arg != NULL) {
            argv[argc] = arg;
            argc++;
            arg = strtok(NULL, " ");
        }

        ////////////////////////////////////////

        execv(argv[0], argv);
        perror(argv[0]);
        exit(127);
    } else {
        int childProcessExitStatusCode;

        if (waitpid(pid, &childProcessExitStatusCode, 0) < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (childProcessExitStatusCode == 0) {
            return AUTOTOOLS_SETUP_OK;
        }

        if (WIFEXITED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' exit with status code: %d\n", cmd, WEXITSTATUS(childProcessExitStatusCode));
        } else if (WIFSIGNALED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' killed by signal: %d\n", cmd, WTERMSIG(childProcessExitStatusCode));
        } else if (WIFSTOPPED(childProcessExitStatusCode)) {
            fprintf(stderr, "running command '%s' stopped by signal: %d\n", cmd, WSTOPSIG(childProcessExitStatusCode));
        }

        return AUTOTOOLS_SETUP_ERROR;
    }
}

static int autotools_setup_download_and_uncompress(const char * url, const char * sha, const char * downloadsDir, size_t downloadsDirLength, const char * uncompressDir, AutotoolsSetupLogLevel logLevel) {
    size_t   urlLength = strlen(url);

    size_t   fetchPhaseCmdLength = urlLength + 10;
    char     fetchPhaseCmd[fetchPhaseCmdLength];
    snprintf(fetchPhaseCmd, fetchPhaseCmdLength, "Fetching %s", url);

    LOG_RUN_CMD(true, logLevel, fetchPhaseCmd)

    ////////////////////////////////////////////////////////////////////////

    char    urlCopy[urlLength + 1];
    strncpy(urlCopy, url, urlLength + 1);

    char *   fileName = basename(urlCopy);

    size_t   fileNameLength = strlen(urlCopy);

    size_t   filePathLength = downloadsDirLength + fileNameLength + 2;
    char     filePath[filePathLength];
    snprintf(filePath, filePathLength, "%s/%s", downloadsDir, fileName);

    bool needFetch = true;

    struct stat st;

    int ret = AUTOTOOLS_SETUP_OK;

    if (stat(filePath, &st) == 0 && S_ISREG(st.st_mode)) {
        char actualSHA256SUM[65] = {0};

        if (sha256sum_of_file(actualSHA256SUM, filePath) != 0) {
            perror(filePath);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (strcmp(actualSHA256SUM, sha) == 0) {
            needFetch = false;

            if (logLevel >= AutotoolsSetupLogLevel_verbose) {
                fprintf(stderr, "%s already have been fetched.\n", filePath);
            }
        }
    }

    if (needFetch) {
        ret = autotools_setup_http_fetch_to_file(url, filePath, logLevel >= AutotoolsSetupLogLevel_verbose, logLevel != AutotoolsSetupLogLevel_silent);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        char actualSHA256SUM[65] = {0};

        if (sha256sum_of_file(actualSHA256SUM, filePath) != 0) {
            perror(filePath);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (strcmp(actualSHA256SUM, sha) != 0) {
            fprintf(stderr, "sha256sum mismatch.\n    expect : %s\n    actual : %s\n", sha, actualSHA256SUM);
            return AUTOTOOLS_SETUP_ERROR_SHA256_MISMATCH;
        }
    }

    size_t   uncompressPhaseCmdLength = filePathLength + 36;
    char     uncompressPhaseCmd[uncompressPhaseCmdLength];
    snprintf(uncompressPhaseCmd, uncompressPhaseCmdLength, "Uncompressing %s --strip-components=1", filePath);

    LOG_RUN_CMD(true, logLevel, uncompressPhaseCmd)

    return tar_extract(uncompressDir, filePath, ARCHIVE_EXTRACT_TIME, logLevel >= AutotoolsSetupLogLevel_verbose, 1);
}

static int autotools_setup_write_env(const char * envFilePath, const char * setupBinDir, const char * setupAclocalDir) {
    FILE * envFile = fopen(envFilePath, "w");

    if (envFile == NULL) {
        perror(envFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    fprintf(envFile, "#!/bin/sh\n\nexport PATH=\"%s:$PATH\"\n\nif [ -z  \"$ACLOCAL_PATH\" ] ; then\n    export ACLOCAL_PATH='%s'\nelse\n    export ACLOCAL_PATH=\"%s:$ACLOCAL_PATH\"\nfi", setupBinDir, setupAclocalDir, setupAclocalDir);

    if (ferror(envFile)) {
        perror(envFilePath);
        fclose(envFile);
        return AUTOTOOLS_SETUP_ERROR;
    } else {
        fclose(envFile);
        return AUTOTOOLS_SETUP_OK;
    }
}

static int autotools_setup_write_receipt(const char * setupDir, size_t setupDirLength, const char * binUrlGmake, const char * binShaGmake, AutotoolsSetupConfig config, SysInfo sysinfo) {
    size_t   receiptFilePathLength = setupDirLength + 13;
    char     receiptFilePath[receiptFilePathLength];
    snprintf(receiptFilePath, receiptFilePathLength, "%s/receipt.yml", setupDir);

    FILE *   receiptFile = fopen(receiptFilePath, "w");

    if (receiptFile == NULL) {
        perror(receiptFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    fprintf(receiptFile, "bin-url-gmake:    %s\n",   binUrlGmake == NULL ? "" : binUrlGmake);
    fprintf(receiptFile, "bin-sha-gmake:    %s\n\n", binShaGmake == NULL ? "" : binShaGmake);

    fprintf(receiptFile, "src-url-gm4:      %s\n",   config.src_url_gm4);
    fprintf(receiptFile, "src-sha-gm4:      %s\n\n", config.src_sha_gm4);

    fprintf(receiptFile, "src-url-perl:     %s\n",   config.src_url_perl);
    fprintf(receiptFile, "src-sha-perl:     %s\n\n", config.src_sha_perl);

    fprintf(receiptFile, "src-url-pkgconf:  %s\n",   config.src_url_pkgconf);
    fprintf(receiptFile, "src-sha-pkgconf:  %s\n\n", config.src_sha_pkgconf);

    fprintf(receiptFile, "src-url-libtool:  %s\n",   config.src_url_libtool);
    fprintf(receiptFile, "src-sha-libtool:  %s\n\n", config.src_sha_libtool);

    fprintf(receiptFile, "src-url-automake: %s\n",   config.src_url_automake);
    fprintf(receiptFile, "src-sha-automake: %s\n\n", config.src_sha_automake);

    fprintf(receiptFile, "src-url-autoconf: %s\n",   config.src_url_autoconf);
    fprintf(receiptFile, "src-sha-autoconf: %s\n\n", config.src_sha_autoconf);

    fprintf(receiptFile, "\nsignature: %s\ntimestamp: %lu\n\n", AUTOTOOLS_SETUP_VERSION, time(NULL));

    fprintf(receiptFile, "build-on:\n    os-arch: %s\n    os-kind: %s\n    os-type: %s\n    os-name: %s\n    os-vers: %s\n    os-ncpu: %u\n    os-euid: %u\n    os-egid: %u\n", sysinfo.arch, sysinfo.kind, sysinfo.type, sysinfo.name, sysinfo.vers, sysinfo.ncpu, sysinfo.euid, sysinfo.egid);

    fclose(receiptFile);

    return AUTOTOOLS_SETUP_OK;
}

typedef struct {
    const char * name;
    const char * src_url;
    const char * src_sha;
} Package;

static int autotools_setup_install_the_given_package(Package package, const char * autotoolsSetupDownloadsDir, size_t autotoolsSetupDownloadsDirLength, const char * autotoolsSetupInstallingRootDir, size_t autotoolsSetupInstallingRootDirLength, const char * setupDir, size_t setupDirLength, AutotoolsSetupLogLevel logLevel, unsigned int jobs, struct stat st, const char * gmakePath, size_t gmakePathLength, bool output2Terminal, int redirectOutput2FD) {
    size_t   packageInstallingDirLength = autotoolsSetupInstallingRootDirLength + strlen(package.name);
    char     packageInstallingDir[packageInstallingDirLength];
    snprintf(packageInstallingDir, packageInstallingDirLength, "%s/%s", autotoolsSetupInstallingRootDir, package.name);

    if (stat(packageInstallingDir, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (rm_r(packageInstallingDir, logLevel >= AutotoolsSetupLogLevel_verbose) != 0) {
                perror(packageInstallingDir);
                return AUTOTOOLS_SETUP_ERROR;
            }
        } else {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", packageInstallingDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    if (mkdir(packageInstallingDir, S_IRWXU) != 0) {
        perror(packageInstallingDir);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    int ret = autotools_setup_download_and_uncompress(package.src_url, package.src_sha, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, packageInstallingDir, logLevel);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (chdir(packageInstallingDir) != 0) {
        perror(packageInstallingDir);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (strcmp(package.name, "perl") == 0) {
        size_t   configurePhaseCmdLength = setupDirLength + 110;
        char     configurePhaseCmd[configurePhaseCmdLength];
        snprintf(configurePhaseCmd, configurePhaseCmdLength, "./Configure -Dprefix=%s -des -Dmake=gmake -Duselargefiles -Duseshrplib -Dusethreads -Dusenm=false -Dusedl=true", setupDir);

        LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd, redirectOutput2FD);
    } else {
        size_t   configurePhaseCmdLength = setupDirLength + 32;
        char     configurePhaseCmd[configurePhaseCmdLength];
        snprintf(configurePhaseCmd, configurePhaseCmdLength, "./configure --prefix=%s %s", setupDir, logLevel == AutotoolsSetupLogLevel_silent ? "--silent" : "");

        LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd, redirectOutput2FD);
    }

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 12;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int autotools_setup_setup_internal(const char * setupDir, AutotoolsSetupConfig config, AutotoolsSetupLogLevel logLevel, unsigned int jobs, SysInfo sysinfo) {
    bool output2Terminal = isatty(STDOUT_FILENO);

    char * userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   autotoolsSetupHomeDirLength = userHomeDirLength + 18;
    char     autotoolsSetupHomeDir[autotoolsSetupHomeDirLength];
    snprintf(autotoolsSetupHomeDir, autotoolsSetupHomeDirLength, "%s/.autotools-setup", userHomeDir);

    struct stat st;

    if (stat(autotoolsSetupHomeDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", autotoolsSetupHomeDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(autotoolsSetupHomeDir, S_IRWXU) != 0) {
            perror(autotoolsSetupHomeDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   defaultSetupDirLength = autotoolsSetupHomeDirLength + 11;
    char     defaultSetupDir[defaultSetupDirLength];
    snprintf(defaultSetupDir, defaultSetupDirLength, "%s/autotools", autotoolsSetupHomeDir);

    if (setupDir == NULL) {
        setupDir = defaultSetupDir;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t setupDirLength = strlen(setupDir);

    size_t   setupBinDirLength = setupDirLength + 5;
    char     setupBinDir[setupBinDirLength];
    snprintf(setupBinDir, setupBinDirLength, "%s/bin", setupDir);

    size_t   setupAclocalDirLength = setupDirLength + 15;
    char     setupAclocalDir[setupAclocalDirLength];
    snprintf(setupAclocalDir, setupAclocalDirLength, "%s/share/aclocal", setupDir);

    // https://www.gnu.org/software/automake/manual/html_node/Macro-Search-Path.html
    if (setenv("ACLOCAL_PATH", setupAclocalDir, 1) != 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    char * PATH = getenv("PATH");

    if (PATH == NULL || strcmp(PATH, "") == 0) {
        return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
    }

    size_t   newPATHLength = setupBinDirLength + strlen(PATH) + 2;
    char     newPATH[newPATHLength];
    snprintf(newPATH, newPATHLength, "%s:%s", setupBinDir, PATH);

    if (setenv("PATH", newPATH, 1) != 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (unsetenv("CPPFLAGS") != 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (logLevel == AutotoolsSetupLogLevel_very_verbose) {
        if (setenv("CFLAGS", "-v -fPIC", 1) != 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (setenv("LDFLAGS", "-Wl,-v", 1) != 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (setenv("CFLAGS", "-fPIC", 1) != 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (unsetenv("LDFLAGS") != 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   autotoolsSetupDownloadsDirLength = autotoolsSetupHomeDirLength + 11;
    char     autotoolsSetupDownloadsDir[autotoolsSetupDownloadsDirLength];
    snprintf(autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, "%s/downloads", autotoolsSetupHomeDir);

    if (stat(autotoolsSetupDownloadsDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", autotoolsSetupDownloadsDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(autotoolsSetupDownloadsDir, S_IRWXU) != 0) {
            perror(autotoolsSetupDownloadsDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   autotoolsSetupInstallingRootDirLength = autotoolsSetupHomeDirLength + 12;
    char     autotoolsSetupInstallingRootDir[autotoolsSetupInstallingRootDirLength];
    snprintf(autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, "%s/installing", autotoolsSetupHomeDir);

    if (stat(autotoolsSetupInstallingRootDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", autotoolsSetupInstallingRootDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(autotoolsSetupInstallingRootDir, S_IRWXU) != 0) {
            perror(autotoolsSetupInstallingRootDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   envFilePathLength = setupDirLength + 8;
    char     envFilePath[envFilePathLength];
    snprintf(envFilePath, envFilePathLength, "%s/env.sh", setupDir);

    //////////////////////////////////////////////////////////////////////////////

    const char * binUrlGmake = NULL;
    const char * binShaGmake = NULL;

    if (strcmp(sysinfo.kind, "linux") == 0) {
        if (strcmp(sysinfo.arch, "x86_64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-x86_64.tar.xz";
            binShaGmake = "b68f0033f4163bd94af6fb93b281c61fc02bc4af2cc7e80e74722dbf4c639dd3";
        } else if (strcmp(sysinfo.arch, "aarch64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-aarch64.tar.xz";
            binShaGmake = "8ba11716b9d473dbc100cd87474d381bd2bcb7822cc029b50e5a8307c6695691";
        } else if (strcmp(sysinfo.arch, "ppc64le") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-ppc64le.tar.xz";
            binShaGmake = "635c8e41683e318f39a81b964deac2ae1fa736009dc05a04f1110393fa0c9480";
        } else if (strcmp(sysinfo.arch, "s390x") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-linux-s390x.tar.xz";
            binShaGmake = "4e25857f78bb0a1932cf5e0de1ad7b424a42875775d174753362c3af61ceeb0d";
        }
    } else if (strcmp(sysinfo.kind, "darwin") == 0) {
        if (strcmp(sysinfo.arch, "x86_64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-macos11.0-x86_64.tar.xz";
            binShaGmake = "f22660038bc9e318fc37660f406767fe3e2a0ccc205defaae3f4b2bc0708e3a9";
        } else if (strcmp(sysinfo.arch, "aarch64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-macos11.0-arm64.tar.xz";
            binShaGmake = "41680f6d1270497f1a3c717ac6150b4239b44430cfbfde4b9f51ff4d4dd1d52c";
        }
    } else if (strcmp(sysinfo.kind, "freebsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-freebsd-amd64.tar.xz";
            binShaGmake = "8bab8e9b83afc8d8e08a4406b2167f8f66efd05fa4d4ba4f8c2b14a543860888";
        }
    } else if (strcmp(sysinfo.kind, "openbsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-openbsd-amd64.tar.xz";
            binShaGmake = "a7d61765f08d536942c6894c0d81fb7e7052906aa2590586237ada8d09cbdf45";
        }
    } else if (strcmp(sysinfo.kind, "netbsd") == 0) {
        if (strcmp(sysinfo.arch, "amd64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-netbsd-amd64.tar.xz";
            binShaGmake = "8ba11716b9d473dbc100cd87474d381bd2bcb7822cc029b50e5a8307c6695691";
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    int redirectOutput2FD = -1;

    if (logLevel < AutotoolsSetupLogLevel_verbose) {
        size_t   logFilePathLength = autotoolsSetupInstallingRootDirLength + 9;
        char     logFilePath[logFilePathLength];
        snprintf(logFilePath, logFilePathLength, "%s/log.txt", autotoolsSetupInstallingRootDir);

        redirectOutput2FD = open("/dev/null", O_CREAT | O_TRUNC | O_WRONLY, 0666);

        if (redirectOutput2FD < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    int STDERR_FILENO2 = STDERR_FILENO;

    if (logLevel == AutotoolsSetupLogLevel_silent) {
        STDERR_FILENO2 = dup(STDERR_FILENO);

        if (STDERR_FILENO2 < 0) {
            perror(NULL);
            close(redirectOutput2FD);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (dup2(redirectOutput2FD, STDOUT_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(redirectOutput2FD);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (dup2(redirectOutput2FD, STDERR_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(redirectOutput2FD);
            return AUTOTOOLS_SETUP_ERROR;
        }

        close(redirectOutput2FD);
        redirectOutput2FD = -1;
    }

    //////////////////////////////////////////////////////////////////////////////

    unsigned int stepN = 1;

    int ret = AUTOTOOLS_SETUP_OK;

    //////////////////////////////////////////////////////////////////////////////

    size_t   defaultGmakePathLength = setupBinDirLength + 7;
    char     defaultGmakePath[defaultGmakePathLength];
    snprintf(defaultGmakePath, defaultGmakePathLength, "%s/gmake", setupBinDir);

    //////////////////////////////////////////////////////////////////////////////

    char * gmakePath = NULL;

    size_t gmakePathLength = 0;

    bool   gmakePathNeedsToBeFreed;

    if (binUrlGmake == NULL) {
        gmakePathNeedsToBeFreed = true;

        LOG_STEP(output2Terminal, logLevel, stepN++, "finding gmake")

        switch (exe_lookup("gmake", &gmakePath, &gmakePathLength)) {
            case  0:
                break;
            case -1:
                perror("gmake");
                return AUTOTOOLS_SETUP_ERROR;
            case -2:
                return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
            case -3:
                return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
            default:
                return AUTOTOOLS_SETUP_ERROR;
        }

        if (gmakePath == NULL) {
            switch (exe_lookup("make", &gmakePath, &gmakePathLength)) {
                case  0:
                    break;
                case -1:
                    perror("make");
                    return AUTOTOOLS_SETUP_ERROR;
                case -2:
                    return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
                case -3:
                    return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
                default:
                    return AUTOTOOLS_SETUP_ERROR;
            }
        }

        if (gmakePath == NULL) {
            fprintf(stderr, "neither 'gmake' nor 'make' command was found in PATH.\n");
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        gmakePathNeedsToBeFreed = false;
        gmakePath =       defaultGmakePath;
        gmakePathLength = defaultGmakePathLength - 1;

        LOG_STEP(output2Terminal, logLevel, stepN++, "installing gmake")

        ret = autotools_setup_download_and_uncompress(binUrlGmake, binShaGmake, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, setupDir, logLevel);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    Package packages[6];
    
    packages[0] = (Package){ "gm4",      config.src_url_gm4,      config.src_sha_gm4      };
    packages[1] = (Package){ "perl",     config.src_url_perl,     config.src_sha_perl     };
    packages[2] = (Package){ "pkgconf",  config.src_url_pkgconf,  config.src_sha_pkgconf  };
    packages[3] = (Package){ "libtool",  config.src_url_libtool,  config.src_sha_libtool  };
    packages[4] = (Package){ "autoconf", config.src_url_autoconf, config.src_sha_autoconf };
    packages[5] = (Package){ "automake", config.src_url_automake, config.src_sha_automake };

    for (unsigned int i = 0; i < 6; i++) {
        Package package = packages[i];

        if (logLevel != AutotoolsSetupLogLevel_silent) { \
            fprintf(stderr, "\n%s=>> STEP %d : installing %s%s\n", COLOR_PURPLE, stepN++, package.name, COLOR_OFF);
        }

        ret = autotools_setup_install_the_given_package(package, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, output2Terminal, redirectOutput2FD);

        if (ret != AUTOTOOLS_SETUP_OK) {
            goto finalize;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    if (chdir(setupBinDir) != 0) {
        perror(setupBinDir);
        ret = AUTOTOOLS_SETUP_ERROR;
        goto finalize;
    }

    unlink("gm4");

    if (symlink("m4", "gm4") != 0) {
        perror("m4");
        ret = AUTOTOOLS_SETUP_ERROR;
        goto finalize;
    }

    unlink("pkg-config");

    if (symlink("pkgconf", "pkg-config") != 0) {
        perror("pkgconf");
        ret = AUTOTOOLS_SETUP_ERROR;
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(output2Terminal, logLevel, stepN++, "generating env.sh")

    ret = autotools_setup_write_env(envFilePath, setupBinDir, setupAclocalDir);

    if (ret != AUTOTOOLS_SETUP_OK) {
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(output2Terminal, logLevel, stepN++, "generating receipt.yml")

    ret = autotools_setup_write_receipt(setupDir, setupDirLength, binUrlGmake, binShaGmake, config, sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "\n%s[✔] successfully setup.%s\n", COLOR_GREEN, COLOR_OFF);
    fprintf(stderr, "\n%s🔔  to use this, run %s'. %s'%s in your terminal.%s\n\n", COLOR_YELLOW, COLOR_OFF, envFilePath, COLOR_YELLOW, COLOR_OFF);

    //////////////////////////////////////////////////////////////////////////////

    if (rm_r(autotoolsSetupInstallingRootDir, logLevel >= AutotoolsSetupLogLevel_verbose) != 0) {
        perror(autotoolsSetupInstallingRootDir);
        ret = AUTOTOOLS_SETUP_ERROR;
    }

finalize:
    if (gmakePathNeedsToBeFreed) {
        free(gmakePath);
    }

    return ret;
}

int autotools_setup_setup(const char * configFilePath, const char * setupDir, AutotoolsSetupLogLevel logLevel, unsigned int jobs) {
    SysInfo sysinfo = {0};

    if (sysinfo_make(&sysinfo) < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (jobs == 0) {
        jobs = sysinfo.ncpu;
    }

    //////////////////////////////////////////////////////////////////////////////

    AutotoolsSetupConfig config = {0};

    config.src_url_gm4 = DEFAULT_SRC_URL_GM4;
    config.src_sha_gm4 = DEFAULT_SRC_SHA_GM4;

    config.src_url_perl = DEFAULT_SRC_URL_PERL;
    config.src_sha_perl = DEFAULT_SRC_SHA_PERL;

    config.src_url_pkgconf = DEFAULT_SRC_URL_PKGCONF;
    config.src_sha_pkgconf = DEFAULT_SRC_SHA_PKGCONF;

    config.src_url_libtool = DEFAULT_SRC_URL_LIBTOOL;
    config.src_sha_libtool = DEFAULT_SRC_SHA_LIBTOOL;

    config.src_url_automake = DEFAULT_SRC_URL_AUTOMAKE;
    config.src_sha_automake = DEFAULT_SRC_SHA_AUTOMAKE;

    config.src_url_autoconf = DEFAULT_SRC_URL_AUTOCONF;
    config.src_sha_autoconf = DEFAULT_SRC_SHA_AUTOCONF;

    AutotoolsSetupConfig * userSpecifiedConfig = NULL;

    int ret = AUTOTOOLS_SETUP_OK;

    if (configFilePath != NULL) {
        ret = autotools_setup_config_parse(configFilePath, &userSpecifiedConfig);

        if (ret != AUTOTOOLS_SETUP_OK) {
            goto finalize;
        }
 
        if (userSpecifiedConfig->src_url_gm4 != NULL) {
            config.src_url_gm4 = userSpecifiedConfig->src_url_gm4;
        }

        if (userSpecifiedConfig->src_sha_gm4 != NULL) {
            config.src_sha_gm4 = userSpecifiedConfig->src_sha_gm4;
        }

        if (userSpecifiedConfig->src_url_perl != NULL) {
            config.src_url_perl = userSpecifiedConfig->src_url_perl;
        }

        if (userSpecifiedConfig->src_sha_perl != NULL) {
            config.src_sha_perl = userSpecifiedConfig->src_sha_perl;
        }

        if (userSpecifiedConfig->src_url_pkgconf != NULL) {
            config.src_url_pkgconf = userSpecifiedConfig->src_url_pkgconf;
        }

        if (userSpecifiedConfig->src_sha_pkgconf != NULL) {
            config.src_sha_pkgconf = userSpecifiedConfig->src_sha_pkgconf;
        }

        if (userSpecifiedConfig->src_url_libtool != NULL) {
            config.src_url_libtool = userSpecifiedConfig->src_url_libtool;
        }

        if (userSpecifiedConfig->src_sha_libtool != NULL) {
            config.src_sha_libtool = userSpecifiedConfig->src_sha_libtool;
        }

        if (userSpecifiedConfig->src_url_automake != NULL) {
            config.src_url_automake = userSpecifiedConfig->src_url_automake;
        }

        if (userSpecifiedConfig->src_sha_automake != NULL) {
            config.src_sha_automake = userSpecifiedConfig->src_sha_automake;
        }

        if (userSpecifiedConfig->src_url_autoconf != NULL) {
            config.src_url_autoconf = userSpecifiedConfig->src_url_autoconf;
        }

        if (userSpecifiedConfig->src_sha_autoconf != NULL) {
            config.src_sha_autoconf = userSpecifiedConfig->src_sha_autoconf;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    ret = autotools_setup_setup_internal(setupDir, config, logLevel, jobs, sysinfo);

finalize:
    sysinfo_free(sysinfo);
    autotools_setup_config_free(userSpecifiedConfig);
    return ret;
}

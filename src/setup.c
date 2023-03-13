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
#include "core/http.h"
#include "core/sysinfo.h"
#include "core/sha256sum.h"
#include "core/base16.h"
#include "core/tar.h"
#include "core/exe.h"
#include "core/self.h"
#include "core/rm-r.h"
#include "core/exe.h"
#include "autotools-setup.h"

#define LOG_STEP(stdout2terminal, logLevel, stepN, msg) \
    if (logLevel != AutotoolsSetupLogLevel_silent) { \
        fprintf(stderr, "%s=>> STEP %d : %s%s\n", COLOR_PURPLE, stepN, msg, COLOR_OFF); \
    }

#define LOG_RUN_CMD(stdout2terminal, logLevel, cmd) \
    if (logLevel != AutotoolsSetupLogLevel_silent) { \
        fprintf(stderr, "%s==>%s %s%s%s\n", COLOR_PURPLE, COLOR_OFF, COLOR_GREEN, cmd, COLOR_OFF); \
    }

static int run_cmd(char * cmd) {
    pid_t pid = fork();

    if (pid < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (pid == 0) {
        size_t argc = 0;
        char* argv[10] = {0};

        char * arg = strtok(cmd, " ");

        while (arg != NULL) {
            argv[argc] = arg;
            argc++;
            arg = strtok(NULL, " ");
        }

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

static int autotools_setup_download_and_extract(const char * url, const char * sha, const char * downloadsDir, size_t downloadsDirLength, const char * extractDIR, AutotoolsSetupLogLevel logLevel) {
    size_t  urlCopyLength = strlen(url) + 1;
    char    urlCopy[urlCopyLength];
    strncpy(urlCopy, url, urlCopyLength);

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

        ret = sha256sum_of_file(actualSHA256SUM, filePath);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        if (strcmp(actualSHA256SUM, sha) == 0) {
            needFetch = false;

            if (logLevel >= AutotoolsSetupLogLevel_verbose) {
                fprintf(stderr, "%s already have been fetched.\n", filePath);
            }
        }
    }

    if (needFetch) {
        ret = http_fetch_to_file(url, filePath, logLevel >= AutotoolsSetupLogLevel_verbose, logLevel != AutotoolsSetupLogLevel_silent);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        char actualSHA256SUM[65] = {0};

        ret = sha256sum_of_file(actualSHA256SUM, filePath);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        if (strcmp(actualSHA256SUM, sha) != 0) {
            fprintf(stderr, "sha256sum mismatch.\n    expect : %s\n    actual : %s\n", sha, actualSHA256SUM);
            return AUTOTOOLS_SETUP_ERROR_SHA256_MISMATCH;
        }
    }

    return tar_extract(extractDIR, filePath, ARCHIVE_EXTRACT_TIME, logLevel >= AutotoolsSetupLogLevel_verbose, 1);
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

    fprintf(receiptFile, "build-on:\n    os-arch: %s\n    os-kind: %s\n    os-type: %s\n    os-name: %s\n    os-vers: %s\n    os-ncpu: %lu\n    os-euid: %u\n    os-egid: %u\n", sysinfo.arch, sysinfo.kind, sysinfo.type, sysinfo.name, sysinfo.vers, sysinfo.ncpu, sysinfo.euid, sysinfo.egid);

    fclose(receiptFile);

    return AUTOTOOLS_SETUP_OK;
}

typedef struct {
    const char * name;
    const char * src_url;
    const char * src_sha;
} Package;

static int autotools_setup_build_the_given_package(Package package, const char * autotoolsSetupDownloadsDir, size_t autotoolsSetupDownloadsDirLength, const char * autotoolsSetupInstallingRootDir, size_t autotoolsSetupInstallingRootDirLength, const char * setupDir, size_t setupDirLength, AutotoolsSetupLogLevel logLevel, unsigned int jobs, struct stat st, const char * gmakePath, size_t gmakePathLength, bool stdout2terminal) {
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

    int ret = autotools_setup_download_and_extract(package.src_url, package.src_sha, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, packageInstallingDir, logLevel);

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

        LOG_RUN_CMD(stdout2terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd);
    } else {
        size_t   configurePhaseCmdLength = setupDirLength + 32;
        char     configurePhaseCmd[configurePhaseCmdLength];
        snprintf(configurePhaseCmd, configurePhaseCmdLength, "./configure --prefix=%s %s", setupDir, logLevel == AutotoolsSetupLogLevel_silent ? "--silent" : "");

        LOG_RUN_CMD(stdout2terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd);
    }

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 20;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s %s --jobs=%u", gmakePath, logLevel == AutotoolsSetupLogLevel_silent ? "--silent" : "", jobs);

    LOG_RUN_CMD(stdout2terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 20;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s %s install", gmakePath, logLevel == AutotoolsSetupLogLevel_silent ? "--silent" : "");

    LOG_RUN_CMD(stdout2terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd);
}

static int autotools_setup_setup_internal(const char * setupDir, AutotoolsSetupConfig config, AutotoolsSetupLogLevel logLevel, unsigned int jobs, SysInfo sysinfo) {
    bool stdout2terminal = isatty(STDOUT_FILENO);

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
    setenv("ACLOCAL_PATH", setupAclocalDir, 1);

    //////////////////////////////////////////////////////////////////////////////

    char * PATH = getenv("PATH");

    if (PATH == NULL || strcmp(PATH, "") == 0) {
        return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
    }

    size_t   newPATHLength = setupBinDirLength + strlen(PATH) + 2;
    char     newPATH[newPATHLength];
    snprintf(newPATH, newPATHLength, "%s:%s", setupBinDir, PATH);

    setenv("PATH", newPATH, 1);

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
        }
    } else if (strcmp(sysinfo.kind, "darwin") == 0) {
        if (strcmp(sysinfo.arch, "x86_64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-MacOSX11.0-x86_64.tar.xz";
            binShaGmake = "cf5c83eae412858474a2ec0bd299e7510d7eb68b3c398f6d0600db8723dc2b6e";
        } else if (strcmp(sysinfo.arch, "aarch64") == 0) {
            binUrlGmake = "https://github.com/leleliu008/gmake-build/releases/download/4.3/gmake-4.3-MacOSX11.0-arm64.tar.xz";
            binShaGmake = "071b01f8c8f67e55fac1d163940bf250bf81f07aff4be5716458394edf15886a";
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

    int STDERR_FILENO2 = STDERR_FILENO;

    if (logLevel == AutotoolsSetupLogLevel_silent) {
        STDERR_FILENO2 = dup(STDERR_FILENO);

        if (STDERR_FILENO2 < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        size_t   logFilePathLength = setupDirLength + 9;
        char     logFilePath[logFilePathLength];
        snprintf(logFilePath, logFilePathLength, "%s/log.txt", setupDir);

        int fd = open(logFilePath, O_CREAT | O_TRUNC | O_WRONLY, 0666);

        if (fd < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(fd);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (dup2(fd, STDERR_FILENO) < 0) {
            perror(NULL);
            close(STDERR_FILENO2);
            close(fd);
            return AUTOTOOLS_SETUP_ERROR;
        }

        close(fd);
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

    size_t gmakePathLength;

    bool   gmakePathNeedsToBeFreed;

    if (binUrlGmake == NULL) {
        gmakePathNeedsToBeFreed = true;

        LOG_STEP(stdout2terminal, logLevel, stepN++, "finding gmake")

        ret = exe_lookup("gmake", &gmakePath, &gmakePathLength);

        if (ret == AUTOTOOLS_SETUP_ERROR_EXE_NOT_FOUND) {
            ret = exe_lookup("make", &gmakePath, &gmakePathLength);
        }

        if (ret == AUTOTOOLS_SETUP_ERROR_EXE_NOT_FOUND) {
            fprintf(stderr, "neither 'gmake' nor 'make' command was found in PATH.\n");
            return AUTOTOOLS_SETUP_ERROR;
        } else if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }
    } else {
        gmakePathNeedsToBeFreed = false;
        gmakePath =       defaultGmakePath;
        gmakePathLength = defaultGmakePathLength - 1;

        LOG_STEP(stdout2terminal, logLevel, stepN++, "installing gmake")

        ret = autotools_setup_download_and_extract(binUrlGmake, binShaGmake, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, setupDir, logLevel);

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

        ret = autotools_setup_build_the_given_package(package, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

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

    LOG_STEP(stdout2terminal, logLevel, stepN++, "generating env.sh")

    ret = autotools_setup_write_env(envFilePath, setupBinDir, setupAclocalDir);

    if (ret != AUTOTOOLS_SETUP_OK) {
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "generating receipt.yml")

    ret = autotools_setup_write_receipt(setupDir, setupDirLength, binUrlGmake, binShaGmake, config, sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        goto finalize;
    }

    //////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "\n%sautotools was successfully setup to %s\n\nrun '. %s' in your terminal to use it.%s\n", COLOR_GREEN, setupDir, envFilePath, COLOR_OFF);

finalize:
    if (gmakePathNeedsToBeFreed) {
        free(gmakePath);
    }

    return ret;
}

int autotools_setup_setup(const char * configFilePath, const char * setupDir, AutotoolsSetupLogLevel logLevel, unsigned int jobs) {
    SysInfo sysinfo = {0};

    int ret = sysinfo_make(&sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
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

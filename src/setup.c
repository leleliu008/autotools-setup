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
    if (logLevel != AUTOTOOLSSETUPLogLevel_silent) { \
        fprintf(stderr, "%s=>> STEP %d : %s%s\n", COLOR_PURPLE, stepN, msg, COLOR_OFF); \
    }

#define LOG_RUN_CMD(stdout2terminal, logLevel, cmd) \
    if (logLevel != AUTOTOOLSSETUPLogLevel_silent) { \
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
        return 127;
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

static int autotools_setup_download_and_extract(const char * url, const char * sha, const char * downloadsDir, size_t downloadsDirLength, const char * extractDIR, AUTOTOOLSSETUPLogLevel logLevel) {
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

            if (logLevel >= AUTOTOOLSSETUPLogLevel_verbose) {
                fprintf(stderr, "%s already have been fetched.\n", filePath);
            }
        }
    }

    if (needFetch) {
        ret = http_fetch_to_file(url, filePath, logLevel >= AUTOTOOLSSETUPLogLevel_verbose, logLevel != AUTOTOOLSSETUPLogLevel_silent);

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

    return tar_extract(extractDIR, filePath, ARCHIVE_EXTRACT_TIME, logLevel >= AUTOTOOLSSETUPLogLevel_verbose, 1);
}

static int autotools_setup_build_the_given_package(const char * packageName, const char * srcUrl, const char * srcSha, const char * autotoolsSetupDownloadsDir, size_t autotoolsSetupDownloadsDirLength, const char * autotoolsSetupInstallingRootDir, size_t autotoolsSetupInstallingRootDirLength, const char * setupDir, size_t setupDirLength, AUTOTOOLSSETUPLogLevel logLevel, unsigned int jobs, struct stat st, const char * gmakePath, size_t gmakePathLength, bool stdout2terminal) {
    size_t   packageInstallingDirLength = autotoolsSetupInstallingRootDirLength + strlen(packageName);
    char     packageInstallingDir[packageInstallingDirLength];
    snprintf(packageInstallingDir, packageInstallingDirLength, "%s/%s", autotoolsSetupInstallingRootDir, packageName);

    if (stat(packageInstallingDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", packageInstallingDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(packageInstallingDir, S_IRWXU) != 0) {
            perror(packageInstallingDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    int ret = autotools_setup_download_and_extract(srcUrl, srcSha, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, packageInstallingDir, logLevel);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (chdir(packageInstallingDir) != 0) {
        perror(packageInstallingDir);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (strcmp(packageName, "perl") == 0) {
        size_t   configurePhaseCmdLength = setupDirLength + 110;
        char     configurePhaseCmd[configurePhaseCmdLength];
        snprintf(configurePhaseCmd, configurePhaseCmdLength, "./Configure -Dprefix=%s -des -Dmake=gmake -Duselargefiles -Duseshrplib -Dusethreads -Dusenm=false -Dusedl=true", setupDir);

        LOG_RUN_CMD(stdout2terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd);
    } else {
        size_t   configurePhaseCmdLength = setupDirLength + 23;
        char     configurePhaseCmd[configurePhaseCmdLength];
        snprintf(configurePhaseCmd, configurePhaseCmdLength, "./configure --prefix=%s", setupDir);

        LOG_RUN_CMD(stdout2terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd);
    }

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   buildPhaseCmdLength = gmakePathLength + 11;
    char     buildPhaseCmd[buildPhaseCmdLength];
    snprintf(buildPhaseCmd, buildPhaseCmdLength, "%s --jobs=%u", gmakePath, jobs);

    LOG_RUN_CMD(stdout2terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   installPhaseCmdLength = gmakePathLength + 9;
    char     installPhaseCmd[installPhaseCmdLength];
    snprintf(installPhaseCmd, installPhaseCmdLength, "%s install", gmakePath);

    LOG_RUN_CMD(stdout2terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd);
}

static int autotools_setup_setup_internal(const char * configFilePath, const char * setupDir, AUTOTOOLSSETUPLogLevel logLevel, unsigned int jobs, SysInfo sysinfo, bool stdout2terminal) {
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

    unsigned int stepN = 1;

    //////////////////////////////////////////////////////////////////////////////

    const char * srcUrlGm4 = NULL;
    const char * srcShaGm4 = NULL;

    const char * srcUrlPerl = NULL;
    const char * srcShaPerl = NULL;

    const char * srcUrlPkgconf = NULL;
    const char * srcShaPkgconf = NULL;

    const char * srcUrlLibtool = NULL;
    const char * srcShaLibtool = NULL;

    const char * srcUrlAutomake = NULL;
    const char * srcShaAutomake = NULL;

    const char * srcUrlAutoconf = NULL;
    const char * srcShaAutoconf = NULL;

    int ret;

    if (configFilePath == NULL) {
        srcUrlGm4 = DEFAULT_SRC_URL_GM4;
        srcShaGm4 = DEFAULT_SRC_SHA_GM4;

        srcUrlPerl = DEFAULT_SRC_URL_PERL;
        srcShaPerl = DEFAULT_SRC_SHA_PERL;

        srcUrlPkgconf = DEFAULT_SRC_URL_PKGCONF;
        srcShaPkgconf = DEFAULT_SRC_SHA_PKGCONF;

        srcUrlLibtool = DEFAULT_SRC_URL_LIBTOOL;
        srcShaLibtool = DEFAULT_SRC_SHA_LIBTOOL;

        srcUrlAutomake = DEFAULT_SRC_URL_AUTOMAKE;
        srcShaAutomake = DEFAULT_SRC_SHA_AUTOMAKE;

        srcUrlAutoconf = DEFAULT_SRC_URL_AUTOCONF;
        srcShaAutoconf = DEFAULT_SRC_SHA_AUTOCONF;
    } else {
        AUTOTOOLSSETUPFormula * config = NULL;

        ret = autotools_setup_config_parse(configFilePath, &config);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        srcUrlGm4 = config->src_url_gm4;
        srcShaGm4 = config->src_sha_gm4;

        srcUrlPerl = config->src_url_perl;
        srcShaPerl = config->src_sha_perl;

        srcUrlPkgconf = config->src_url_pkgconf;
        srcShaPkgconf = config->src_sha_pkgconf;

        srcUrlLibtool = config->src_url_libtool;
        srcShaLibtool = config->src_sha_libtool;

        srcUrlAutomake = config->src_url_automake;
        srcShaAutomake = config->src_sha_automake;

        srcUrlAutoconf = config->src_url_autoconf;
        srcShaAutoconf = config->src_sha_autoconf;

        if (srcUrlGm4 == NULL) {
            srcUrlGm4 = DEFAULT_SRC_URL_GM4;
        }

        if (srcShaGm4 == NULL) {
            srcShaGm4 = DEFAULT_SRC_SHA_GM4;
        }

        if (srcUrlPerl == NULL) {
            srcUrlPerl = DEFAULT_SRC_URL_PERL;
        }

        if (srcShaPerl == NULL) {
            srcShaPerl = DEFAULT_SRC_SHA_PERL;
        }

        if (srcUrlPkgconf == NULL) {
            srcUrlPkgconf = DEFAULT_SRC_URL_PKGCONF;
        }

        if (srcShaPkgconf == NULL) {
            srcShaPkgconf = DEFAULT_SRC_SHA_PKGCONF;
        }

        if (srcUrlLibtool == NULL) {
            srcUrlLibtool = DEFAULT_SRC_URL_LIBTOOL;
        }

        if (srcShaLibtool == NULL) {
            srcShaLibtool = DEFAULT_SRC_SHA_LIBTOOL;
        }

        if (srcUrlAutomake == NULL) {
            srcUrlAutomake = DEFAULT_SRC_URL_AUTOMAKE;
        }

        if (srcShaAutomake == NULL) {
            srcShaAutomake = DEFAULT_SRC_SHA_AUTOMAKE;
        }

        if (srcUrlAutoconf == NULL) {
            srcUrlAutoconf = DEFAULT_SRC_URL_AUTOCONF;
        }

        if (srcShaAutoconf == NULL) {
            srcShaAutoconf = DEFAULT_SRC_SHA_AUTOCONF;
        }
    }

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

    if (binUrlGmake == NULL) {
        LOG_STEP(stdout2terminal, logLevel, stepN++, "finding gmake")

    } else {
        LOG_STEP(stdout2terminal, logLevel, stepN++, "installing gmake")

        ret = autotools_setup_download_and_extract(binUrlGmake, binShaGmake, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, setupDir, logLevel);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t   gmakePathLength = setupDirLength + 11;
    char     gmakePath[gmakePathLength];
    snprintf(gmakePath, gmakePathLength, "%s/bin/gmake", setupDir);

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

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing gm4")

    ret = autotools_setup_build_the_given_package("gm4", srcUrlGm4, srcShaGm4, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing perl")

    ret = autotools_setup_build_the_given_package("perl", srcUrlPerl, srcShaPerl, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing pkgconf")

    ret = autotools_setup_build_the_given_package("pkgconf", srcUrlPkgconf, srcShaPkgconf, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing libtool")

    ret = autotools_setup_build_the_given_package("libtool", srcUrlLibtool, srcShaLibtool, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing autoconf")

    ret = autotools_setup_build_the_given_package("autoconf", srcUrlAutoconf, srcShaAutoconf, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(stdout2terminal, logLevel, stepN++, "installing automake")

    ret = autotools_setup_build_the_given_package("automake", srcUrlAutomake, srcShaAutomake, autotoolsSetupDownloadsDir, autotoolsSetupDownloadsDirLength, autotoolsSetupInstallingRootDir, autotoolsSetupInstallingRootDirLength, setupDir, setupDirLength, logLevel, jobs, st, gmakePath, gmakePathLength, stdout2terminal);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

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

    fprintf(receiptFile, "src-url-gm4:      %s\n",   srcUrlGm4);
    fprintf(receiptFile, "src-sha-gm4:      %s\n\n", srcShaGm4);

    fprintf(receiptFile, "src-url-perl:     %s\n",   srcUrlPerl);
    fprintf(receiptFile, "src-sha-perl:     %s\n\n", srcShaPerl);

    fprintf(receiptFile, "src-url-pkgconf:  %s\n",   srcUrlPkgconf);
    fprintf(receiptFile, "src-sha-pkgconf:  %s\n\n", srcShaPkgconf);

    fprintf(receiptFile, "src-url-libtool:  %s\n",   srcUrlLibtool);
    fprintf(receiptFile, "src-sha-libtool:  %s\n\n", srcShaLibtool);

    fprintf(receiptFile, "src-url-automake: %s\n",   srcUrlAutomake);
    fprintf(receiptFile, "src-sha-automake: %s\n\n", srcShaAutomake);

    fprintf(receiptFile, "src-url-autoconf: %s\n",   srcUrlAutoconf);
    fprintf(receiptFile, "src-sha-autoconf: %s\n\n", srcShaAutoconf);

    fprintf(receiptFile, "\nsignature: %s\ntimestamp: %lu\n\n", AUTOTOOLS_SETUP_VERSION, time(NULL));

    fprintf(receiptFile, "build-on:\n    os-arch: %s\n    os-kind: %s\n    os-type: %s\n    os-name: %s\n    os-vers: %s\n    os-ncpu: %lu\n    os-euid: %u\n    os-egid: %u\n", sysinfo.arch, sysinfo.kind, sysinfo.type, sysinfo.name, sysinfo.vers, sysinfo.ncpu, sysinfo.euid, sysinfo.egid);

    fclose(receiptFile);

    //////////////////////////////////////////////////////////////////////////////

    size_t   envFilePathLength = setupDirLength + 8;
    char     envFilePath[envFilePathLength];
    snprintf(envFilePath, envFilePathLength, "%s/env.sh", setupDir);

    FILE *   envFile = fopen(envFilePath, "w");

    if (envFile == NULL) {
        perror(envFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    fprintf(envFile, "#!/bin/sh\n\nexport PATH=\"%s:$PATH\"\n\nif [ -z  \"$ACLOCAL_PATH\" ] ; then\n    export ACLOCAL_PATH='%s'\nelse\n    export ACLOCAL_PATH=\"%s:$ACLOCAL_PATH\"\nfi", setupBinDir, setupAclocalDir, setupAclocalDir);

    fclose(envFile);

    //////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "\n%sautotools was successfully setup to %s\n\nrun 'source %s' in your terminal to use it.%s\n", COLOR_GREEN, setupDir, envFilePath, COLOR_OFF);

    return AUTOTOOLS_SETUP_OK;
}

int autotools_setup_setup(const char * configFilePath, const char * setupDir, AUTOTOOLSSETUPLogLevel logLevel, unsigned int jobs) {
    SysInfo sysinfo = {0};

    int ret = sysinfo_make(&sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    if (jobs == 0) {
        jobs = sysinfo.ncpu;
    }

    ret = autotools_setup_setup_internal(configFilePath, setupDir, logLevel, jobs, sysinfo, isatty(STDOUT_FILENO));

    sysinfo_free(sysinfo);

    return ret;
}

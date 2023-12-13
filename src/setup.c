#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "core/cp.h"
#include "core/log.h"
#include "core/sysinfo.h"
#include "core/base16.h"
#include "core/tar.h"
#include "core/exe.h"
#include "core/self.h"
#include "core/exe.h"

#include "sha256sum.h"
#include "main.h"

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

        size_t argc = 0U;
        char*  argv[15] = {0};

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

static int autotools_setup_download_and_uncompress(const char * url, const char * uri, const char * expectedSHA256SUM, const char * downloadDIR, size_t downloadDIRLength, const char * unpackDIR, size_t unpackDIRLength, AutotoolsSetupLogLevel logLevel) {
    bool verbose = (logLevel == AutotoolsSetupLogLevel_verbose);

    char fileNameExtension[21] = {0};

    int ret = autotools_setup_examine_file_extension_from_url(url, fileNameExtension, 20);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    size_t fileNameCapacity = strlen(expectedSHA256SUM) + strlen(fileNameExtension) + 1U;
    char   fileName[fileNameCapacity];

    ret = snprintf(fileName, fileNameCapacity, "%s%s", expectedSHA256SUM, fileNameExtension);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t filePathCapacity = downloadDIRLength + fileNameCapacity + 1U;
    char   filePath[filePathCapacity];

    ret = snprintf(filePath, filePathCapacity, "%s/%s", downloadDIR, fileName);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    bool needFetch = true;

    struct stat st;

    if (stat(filePath, &st) == 0 && S_ISREG(st.st_mode)) {
        char actualSHA256SUM[65] = {0};

        ret = sha256sum_of_file(actualSHA256SUM, filePath);

        if (ret != 0) {
            return ret;
        }

        if (strcmp(actualSHA256SUM, expectedSHA256SUM) == 0) {
            needFetch = false;

            if (verbose) {
                fprintf(stderr, "%s already have been fetched.\n", filePath);
            }
        }
    }

    if (needFetch) {
        size_t tmpStrCapacity = strlen(url) + 30U;
        char   tmpStr[tmpStrCapacity];

        ret = snprintf(tmpStr, tmpStrCapacity, "%s|%ld|%d", url, time(NULL), getpid());

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        char tmpFileName[65] = {0};

        ret = sha256sum_of_string(tmpFileName, tmpStr);

        if (ret != 0) {
            return AUTOTOOLS_SETUP_ERROR;
        }

        size_t tmpFilePathCapacity = downloadDIRLength + 70U;
        char   tmpFilePath[tmpFilePathCapacity];

        ret = snprintf(tmpFilePath, tmpFilePathCapacity, "%s/%s.tmp", downloadDIR, tmpFileName);

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        ////////////////////////////////////////////////////////////////////////

        size_t fetchPhaseCmdCapacity = tmpFilePathCapacity + strlen(url) + 13U;
        char   fetchPhaseCmd[fetchPhaseCmdCapacity];

        ret = snprintf(fetchPhaseCmd, fetchPhaseCmdCapacity, "curl -L -o %s %s", tmpFilePath, url);

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        LOG_RUN_CMD(true, logLevel, fetchPhaseCmd)

        ////////////////////////////////////////////////////////////////////////

        ret = autotools_setup_http_fetch_to_file(url, tmpFilePath, verbose, true);

        if (ret != AUTOTOOLS_SETUP_OK) {
            if (uri != NULL) {
                ret = autotools_setup_http_fetch_to_file(uri, tmpFilePath, verbose, true);
            }
        }

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        char actualSHA256SUM[65] = {0};

        ret = sha256sum_of_file(actualSHA256SUM, tmpFilePath);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        if (strcmp(actualSHA256SUM, expectedSHA256SUM) == 0) {
            size_t renamePhaseCmdCapacity = tmpFilePathCapacity + filePathCapacity + 8U;
            char   renamePhaseCmd[renamePhaseCmdCapacity];

            ret = snprintf(renamePhaseCmd, renamePhaseCmdCapacity, "rename %s %s", tmpFilePath, filePath);

            if (ret < 0) {
                perror(NULL);
                return AUTOTOOLS_SETUP_ERROR;
            }

            LOG_RUN_CMD(true, logLevel, renamePhaseCmd)

            if (rename(tmpFilePath, filePath) == -1) {
                perror(filePath);
                return AUTOTOOLS_SETUP_ERROR;
            }
        } else {
            fprintf(stderr, "sha256sum mismatch.\n    expect : %s\n    actual : %s\n", expectedSHA256SUM, actualSHA256SUM);
            return AUTOTOOLS_SETUP_ERROR_SHA256_MISMATCH;
        }
    }

    if (strcmp(fileNameExtension, ".zip") == 0 ||
        strcmp(fileNameExtension, ".tgz") == 0 ||
        strcmp(fileNameExtension, ".txz") == 0 ||
        strcmp(fileNameExtension, ".tlz") == 0 ||
        strcmp(fileNameExtension, ".tbz2") == 0) {

        size_t uncompressPhaseCmdCapacity = filePathCapacity + unpackDIRLength + 36U;
        char   uncompressPhaseCmd[uncompressPhaseCmdCapacity];

        ret = snprintf(uncompressPhaseCmd, uncompressPhaseCmdCapacity, "bsdtar xf %s -C %s --strip-components=1", filePath, unpackDIR);

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        LOG_RUN_CMD(true, logLevel, uncompressPhaseCmd)

        ret = tar_extract(unpackDIR, filePath, ARCHIVE_EXTRACT_TIME, verbose, 1);

        if (ret != 0) {
            return abs(ret) + AUTOTOOLS_SETUP_ERROR_ARCHIVE_BASE;
        }
    } else {
        size_t toFilePathCapacity = unpackDIRLength + fileNameCapacity + 1U;
        char   toFilePath[toFilePathCapacity];

        ret = snprintf(toFilePath, toFilePathCapacity, "%s/%s", unpackDIR, fileName);

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        ret = autotools_setup_copy_file(filePath, toFilePath);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }
    }

    return AUTOTOOLS_SETUP_OK;
}

static int autotools_setup_write_env(const char * envFilePath, const char * setupBinDIR, const char * setupAclocalDIR) {
    FILE * envFile = fopen(envFilePath, "w");

    if (envFile == NULL) {
        perror(envFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    int ret = fprintf(envFile, "#!/bin/sh\n\nexport PATH=\"%s:$PATH\"\n\nif [ -z  \"$ACLOCAL_PATH\" ] ; then\n    export ACLOCAL_PATH='%s'\nelse\n    export ACLOCAL_PATH=\"%s:$ACLOCAL_PATH\"\nfi", setupBinDIR, setupAclocalDIR, setupAclocalDIR);

    if (ret < 0) {
        perror(envFilePath);
        fclose(envFile);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (ferror(envFile)) {
        perror(envFilePath);
        fclose(envFile);
        return AUTOTOOLS_SETUP_ERROR;
    } else {
        fclose(envFile);
        return AUTOTOOLS_SETUP_OK;
    }
}

static int autotools_setup_write_receipt(const char * setupDIR, size_t setupDIRLength, const char * binUrlGmake, const char * binShaGmake, AutotoolsSetupConfig config, SysInfo sysinfo) {
    size_t receiptFilePathCapacity = setupDIRLength + 13U;
    char   receiptFilePath[receiptFilePathCapacity];

    int ret = snprintf(receiptFilePath, receiptFilePathCapacity, "%s/receipt.yml", setupDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    FILE * receiptFile = fopen(receiptFilePath, "w");

    if (receiptFile == NULL) {
        perror(receiptFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    fprintf(receiptFile, "bin-url-gmake:    %s\n",   (binUrlGmake == NULL) ? "" : binUrlGmake);
    fprintf(receiptFile, "bin-sha-gmake:    %s\n\n", (binShaGmake == NULL) ? "" : binShaGmake);

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

static int autotools_setup_install_the_given_package(Package * package, const char * downloadDIR, size_t downloadDIRLength, const char * sessionDIR, size_t sessionDIRLength, const char * setupDIR, size_t setupDIRLength, AutotoolsSetupLogLevel logLevel, unsigned int jobs, struct stat st, const char * gmakePath, size_t gmakePathLength, bool output2Terminal, int redirectOutput2FD) {
    size_t workingDIRCapacity = sessionDIRLength + strlen(package->name);
    char   workingDIR[workingDIRCapacity];

    int ret = snprintf(workingDIR, workingDIRCapacity, "%s/%s", sessionDIR, package->name);
  
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (stat(workingDIR, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            ret = autotools_setup_rm_r(workingDIR, logLevel >= AutotoolsSetupLogLevel_verbose);

            if (ret != AUTOTOOLS_SETUP_OK) {
                return ret;
            }
        } else {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", workingDIR);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    if (mkdir(workingDIR, S_IRWXU) != 0) {
        perror(workingDIR);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    ret = autotools_setup_download_and_uncompress(package->src_url, NULL, package->src_sha, downloadDIR, downloadDIRLength, workingDIR, workingDIRCapacity, logLevel);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t cdPhaseCmdCapacity = workingDIRCapacity + 10U;
    char   cdPhaseCmd[cdPhaseCmdCapacity];

    ret = snprintf(cdPhaseCmd, cdPhaseCmdCapacity, "cd %s", workingDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    LOG_RUN_CMD(true, logLevel, cdPhaseCmd)

    if (chdir(workingDIR) != 0) {
        perror(workingDIR);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    if (strcmp(package->name, "perl") == 0) {
        size_t configurePhaseCmdCapacity = (setupDIRLength * 3) + 161U;
        char   configurePhaseCmd[configurePhaseCmdCapacity];

        ret = snprintf(configurePhaseCmd, configurePhaseCmdCapacity, "./Configure -des -Dprefix=%s -Dman1dir=%s/share/man/man1 -Dman3dir=%s/share/man/man3 -Dmake=gmake -Duselargefiles -Duseshrplib -Dusethreads -Dusenm=false -Dusedl=true", setupDIR, setupDIR, setupDIR);
 
        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd, redirectOutput2FD);
    } else {
        size_t configurePhaseCmdCapacity = setupDIRLength + 32U;
        char   configurePhaseCmd[configurePhaseCmdCapacity];

        ret = snprintf(configurePhaseCmd, configurePhaseCmdCapacity, "./configure --prefix=%s %s", setupDIR, (logLevel == AutotoolsSetupLogLevel_silent) ? "--silent" : "");
 
        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        LOG_RUN_CMD(output2Terminal, logLevel, configurePhaseCmd)

        ret = run_cmd(configurePhaseCmd, redirectOutput2FD);
    }

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t buildPhaseCmdCapacity = gmakePathLength + 12U;
    char   buildPhaseCmd[buildPhaseCmdCapacity];

    ret = snprintf(buildPhaseCmd, buildPhaseCmdCapacity, "%s --jobs=%u", gmakePath, jobs);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    LOG_RUN_CMD(output2Terminal, logLevel, buildPhaseCmd)

    ret = run_cmd(buildPhaseCmd, redirectOutput2FD);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t installPhaseCmdCapacity = gmakePathLength + 20U;
    char   installPhaseCmd[installPhaseCmdCapacity];

    ret = snprintf(installPhaseCmd, installPhaseCmdCapacity, "%s install", gmakePath);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    LOG_RUN_CMD(output2Terminal, logLevel, installPhaseCmd)

    return run_cmd(installPhaseCmd, redirectOutput2FD);
}

static int autotools_setup_setup_internal(const char * setupDIR, AutotoolsSetupConfig config, AutotoolsSetupLogLevel logLevel, unsigned int jobs, SysInfo sysinfo) {
    bool output2Terminal = isatty(STDOUT_FILENO);

    //////////////////////////////////////////////////////////////////////////////

    const char * const userHomeDIR = getenv("HOME");

    if (userHomeDIR == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    if (userHomeDIR[0] == '\0') {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t homeDIRCapacity = strlen(userHomeDIR) + 18U;
    char   homeDIR[homeDIRCapacity];

    int ret = snprintf(homeDIR, homeDIRCapacity, "%s/.autotools-setup", userHomeDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    struct stat st;

    if (stat(homeDIR, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "%s was expected to be a directory, but it was not.\n", homeDIR);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(homeDIR, S_IRWXU) != 0) {
            if (errno != EEXIST) {
                perror(homeDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////

    size_t runDIRCapacity = homeDIRCapacity + 5U;
    char   runDIR[runDIRCapacity];

    ret = snprintf(runDIR, runDIRCapacity, "%s/run", homeDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (lstat(runDIR, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            if (unlink(runDIR) != 0) {
                perror(runDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }

            if (mkdir(runDIR, S_IRWXU) != 0) {
                if (errno != EEXIST) {
                    perror(runDIR);
                    return AUTOTOOLS_SETUP_ERROR;
                }
            }
        }
    } else {
        if (mkdir(runDIR, S_IRWXU) != 0) {
            if (errno != EEXIST) {
                perror(runDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    size_t sessionDIRCapacity = runDIRCapacity + 20U;
    char   sessionDIR[sessionDIRCapacity];

    ret = snprintf(sessionDIR, sessionDIRCapacity, "%s/%d", runDIR, getpid());

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (lstat(sessionDIR, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            ret = autotools_setup_rm_r(sessionDIR, false);

            if (ret != AUTOTOOLS_SETUP_OK) {
                return ret;
            }

            if (mkdir(sessionDIR, S_IRWXU) != 0) {
                perror(sessionDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }
        } else {
            if (unlink(sessionDIR) != 0) {
                perror(sessionDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }

            if (mkdir(sessionDIR, S_IRWXU) != 0) {
                perror(sessionDIR);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }
    } else {
        if (mkdir(sessionDIR, S_IRWXU) != 0) {
            perror(sessionDIR);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t defaultSetupDIRCapacity = homeDIRCapacity + 11U;
    char   defaultSetupDIR[defaultSetupDIRCapacity];

    ret = snprintf(defaultSetupDIR, defaultSetupDIRCapacity, "%s/autotools", homeDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (setupDIR == NULL) {
        setupDIR = defaultSetupDIR;
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t setupDIRLength = strlen(setupDIR);

    size_t setupBinDIRCapacity = setupDIRLength + 5U;
    char   setupBinDIR[setupBinDIRCapacity];

    ret = snprintf(setupBinDIR, setupBinDIRCapacity, "%s/bin", setupDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t setupAclocalDIRCapacity = setupDIRLength + 15U;
    char   setupAclocalDIR[setupAclocalDIRCapacity];

    ret = snprintf(setupAclocalDIR, setupAclocalDIRCapacity, "%s/share/aclocal", setupDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    // https://www.gnu.org/software/automake/manual/html_node/Macro-Search-Path.html
    if (setenv("ACLOCAL_PATH", setupAclocalDIR, 1) != 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    const char * const PATH = getenv("PATH");

    if ((PATH == NULL) || (strcmp(PATH, "") == 0)) {
        return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
    }

    size_t   newPATHCapacity = setupBinDIRCapacity + strlen(PATH) + 2U;
    char     newPATH[newPATHCapacity];

    ret = snprintf(newPATH, newPATHCapacity, "%s:%s", setupBinDIR, PATH);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

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

    const char* unsetenvs[] = {
        "LIBS",
        "TARGET_ARCH",
        "AUTOCONF",
        "AUTOHEADER",
        "AUTOM4TE",
        "AUTOMAKE",
        "AUTOPOINT",
        "ACLOCAL",
        "GTKDOCIZE",
        "INTLTOOLIZE",
        "LIBTOOLIZE",
        "M4", 
        "MAKE",
        NULL
    };

    for (int i = 0; ;i++) {
        const char * name = unsetenvs[i];

        if (name == NULL) {
            break;
        }

        if (unsetenv(name) != 0) {
            perror(name);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t downloadDIRCapacity = homeDIRCapacity + 11U;
    char   downloadDIR[downloadDIRCapacity];

    ret = snprintf(downloadDIR, downloadDIRCapacity, "%s/downloads", homeDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (stat(downloadDIR, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", downloadDIR);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(downloadDIR, S_IRWXU) != 0) {
            perror(downloadDIR);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    size_t envFilePathCapacity = setupDIRLength + 8U;
    char   envFilePath[envFilePathCapacity];

    ret = snprintf(envFilePath, envFilePathCapacity, "%s/env.sh", setupDIR);
 
    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
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
        } else if (strcmp(sysinfo.arch, "arm64") == 0) {
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
        size_t logFilePathCapacity = sessionDIRCapacity + 9U;
        char   logFilePath[logFilePathCapacity];

        ret = snprintf(logFilePath, logFilePathCapacity, "%s/log.txt", sessionDIR);
 
        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

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

    unsigned int stepN = 1U;

    //////////////////////////////////////////////////////////////////////////////

    size_t gmakePathLength = 0U;
    char   gmakePath[PATH_MAX];
           gmakePath[0] = '\0';

    if (binUrlGmake == NULL) {
        LOG_STEP(output2Terminal, logLevel, stepN++, "finding gmake")

        const char * names[2] = { "gmake", "make" };

        for (int i = 0; i < 2; i++) {
             switch (exe_where(names[i], gmakePath, PATH_MAX, &gmakePathLength)) {
                case  0:
                    break;
                case -1:
                    perror(names[i]);
                    return AUTOTOOLS_SETUP_ERROR;
                case -2:
                    return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
                case -3:
                    return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
                default:
                    return AUTOTOOLS_SETUP_ERROR;
            }
        }

        if (gmakePath[0] == '\0') {
            fprintf(stderr, "neither 'gmake' nor 'make' command was found in PATH.\n");
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        LOG_STEP(output2Terminal, logLevel, stepN++, "installing gmake")

        ret = autotools_setup_download_and_uncompress(binUrlGmake, NULL, binShaGmake, downloadDIR, downloadDIRCapacity, setupDIR, setupDIRLength, logLevel);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }

        //////////////////////////////////////////////////////////////////////////////

        size_t gmakePathCapacity = setupBinDIRCapacity + 7U;

        ret = snprintf(gmakePath, gmakePathCapacity, "%s/gmake", setupBinDIR);
     
        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        gmakePathLength = ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    Package packages[6] = {
        { "gm4",      config.src_url_gm4,      config.src_sha_gm4      },
        { "perl",     config.src_url_perl,     config.src_sha_perl     },
        { "pkgconf",  config.src_url_pkgconf,  config.src_sha_pkgconf  },
        { "libtool",  config.src_url_libtool,  config.src_sha_libtool  },
        { "autoconf", config.src_url_autoconf, config.src_sha_autoconf },
        { "automake", config.src_url_automake, config.src_sha_automake }
    };

    for (int i = 0; i < 6; i++) {
        Package package = packages[i];

        if (logLevel != AutotoolsSetupLogLevel_silent) { \
            fprintf(stderr, "\n%s=>> STEP %d : installing %s%s\n", COLOR_PURPLE, stepN++, package.name, COLOR_OFF);
        }

        ret = autotools_setup_install_the_given_package(&package, downloadDIR, downloadDIRCapacity, sessionDIR, sessionDIRCapacity, setupDIR, setupDIRLength, logLevel, jobs, st, gmakePath, gmakePathLength, output2Terminal, redirectOutput2FD);

        if (ret != AUTOTOOLS_SETUP_OK) {
            return ret;
        }
    }

    //////////////////////////////////////////////////////////////////////////////

    if (chdir(setupBinDIR) != 0) {
        perror(setupBinDIR);
        return AUTOTOOLS_SETUP_ERROR;
    }

    unlink("gm4");

    if (symlink("m4", "gm4") != 0) {
        perror("m4");
        return AUTOTOOLS_SETUP_ERROR;
    }

    unlink("pkg-config");

    if (symlink("pkgconf", "pkg-config") != 0) {
        perror("pkgconf");
        return AUTOTOOLS_SETUP_ERROR;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(output2Terminal, logLevel, stepN++, "generating env.sh")

    ret = autotools_setup_write_env(envFilePath, setupBinDIR, setupAclocalDIR);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    LOG_STEP(output2Terminal, logLevel, stepN++, "generating receipt.yml")

    ret = autotools_setup_write_receipt(setupDIR, setupDIRLength, binUrlGmake, binShaGmake, config, sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "\n%s[✔] successfully setup.%s\n", COLOR_GREEN, COLOR_OFF);
    fprintf(stderr, "\n%s🔔  to use this, run %s'. %s'%s in your terminal.%s\n\n", COLOR_YELLOW, COLOR_OFF, envFilePath, COLOR_YELLOW, COLOR_OFF);

    //////////////////////////////////////////////////////////////////////////////

    return autotools_setup_rm_r(sessionDIR, logLevel >= AutotoolsSetupLogLevel_verbose);
}

int autotools_setup_setup(const char * configFilePath, const char * setupDIR, AutotoolsSetupLogLevel logLevel, unsigned int jobs) {
    SysInfo sysinfo = {0};

    if (sysinfo_make(&sysinfo) < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (jobs == 0U) {
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

    ret = autotools_setup_setup_internal(setupDIR, config, logLevel, jobs, sysinfo);

finalize:
    sysinfo_free(sysinfo);
    autotools_setup_config_free(userSpecifiedConfig);
    return ret;
}

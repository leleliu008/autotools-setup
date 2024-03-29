#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

#include "main.h"

int autotools_setup_integrate_zsh_completion(const char * outputDIR, bool verbose) {
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

    ////////////////////////////////////////////////////////////////////////////////////////

    size_t tmpFilePathCapacity = sessionDIRCapacity + 18U;
    char   tmpFilePath[tmpFilePathCapacity];

    ret = snprintf(tmpFilePath, tmpFilePathCapacity, "%s/_autotools-setup", sessionDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    const char * const url = "https://raw.githubusercontent.com/leleliu008/autotools-setup/master/autotools-setup-zsh-completion";

    ret = autotools_setup_http_fetch_to_file(url, tmpFilePath, verbose, verbose);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    size_t defaultOutputDIRCapacity = homeDIRCapacity + 26U;
    char   defaultOutputDIR[defaultOutputDIRCapacity];

    ret = snprintf(defaultOutputDIR, defaultOutputDIRCapacity, "%s/share/zsh/site-functions", homeDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t outputDIRLength;

    if (outputDIR == NULL) {
        outputDIR       = defaultOutputDIR;
        outputDIRLength = ret;
    } else {
        outputDIRLength = strlen(outputDIR);
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    ret = autotools_setup_mkdir_p(outputDIR, verbose);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    ////////////////////////////////////////////////////////////////////////////////////////

    size_t outputFilePathCapacity = outputDIRLength + 18U;
    char   outputFilePath[outputFilePathCapacity];

    ret = snprintf(outputFilePath, outputFilePathCapacity, "%s/_autotools-setup", outputDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (rename(tmpFilePath, outputFilePath) != 0) {
        if (errno == EXDEV) {
            ret = autotools_setup_copy_file(tmpFilePath, outputFilePath);

            if (ret != AUTOTOOLS_SETUP_OK) {
                return ret;
            }
        } else {
            perror(outputFilePath);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    printf("zsh completion script for autotools_setup has been written to %s\n", outputFilePath);
    return AUTOTOOLS_SETUP_OK;
}

int autotools_setup_integrate_bash_completion(const char * outputDIR, bool verbose) {
    (void)outputDIR;
    (void)verbose;
    return AUTOTOOLS_SETUP_OK;
}

int autotools_setup_integrate_fish_completion(const char * outputDIR, bool verbose) {
    (void)outputDIR;
    (void)verbose;
    return AUTOTOOLS_SETUP_OK;
}

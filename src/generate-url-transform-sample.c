#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "core/log.h"

#include "main.h"

int autotools_setup_generate_url_transform_sample() {
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

    ////////////////////////////////////////////////////////////////////////////////////////////

    size_t tmpFilePathCapacity = sessionDIRCapacity + 22U;
    char   tmpFilePath[tmpFilePathCapacity];

    ret = snprintf(tmpFilePath, tmpFilePathCapacity, "%s/url-transform.sample", sessionDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    int fd = open(tmpFilePath, O_CREAT | O_TRUNC | O_WRONLY, 0666);

    if (fd == -1) {
        perror(tmpFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    const char * p = ""
        "#!/bin/sh\n"
        "case $1 in\n"
        "    *githubusercontent.com/*)\n"
        "        printf '%s\\n' \"$1\" | sed 's|githubusercontent|gitmirror|'\n"
        "        ;;\n"
        "    https://github.com/*)\n"
        "        printf 'https://hub.gitmirror.com/%s\\n' \"$1\"\n"
        "        ;;\n"
        "    '') printf '%s\\n' \"$0 <URL>, <URL> is unspecified.\" >&2 ; exit 1 ;;\n"
        "    *)  printf '%s\\n' \"$1\"\n"
        "esac";

    size_t pSize = strlen(p);

    ssize_t writeSize = write(fd, p, pSize);

    if (writeSize == -1) {
        perror(tmpFilePath);
        close(fd);
        return AUTOTOOLS_SETUP_ERROR;
    }

    close(fd);

    if ((size_t)writeSize != pSize) {
        fprintf(stderr, "not fully written to %s\n", tmpFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    if (chmod(tmpFilePath, S_IRWXU) != 0) {
        perror(tmpFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    size_t outFilePathCapacity = homeDIRCapacity + 22U;
    char   outFilePath[outFilePathCapacity];

    ret = snprintf(outFilePath, outFilePathCapacity, "%s/url-transform.sample", homeDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (rename(tmpFilePath, outFilePath) != 0) {
        if (errno == EXDEV) {
            ret = autotools_setup_copy_file(tmpFilePath, outFilePath);

            if (ret != AUTOTOOLS_SETUP_OK) {
                return ret;
            }
        } else {
            perror(tmpFilePath);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    fprintf(stderr, "%surl-transform sample has been written into %s%s\n\n", COLOR_GREEN, outFilePath, COLOR_OFF);

    outFilePath[outFilePathCapacity - 9U] = '\0';

    fprintf(stderr, "%sYou can rename url-transform.sample to url-transform then edit it to meet your needs.\n\nTo apply this, you should run 'export AUTOTOOLS_SETUP_URL_TRANSFORM=%s' in your terminal.\n%s", COLOR_GREEN, outFilePath, COLOR_OFF);

    return autotools_setup_rm_r(sessionDIR, false);
}

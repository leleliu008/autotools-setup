#include <math.h>
#include <errno.h>
#include <string.h>

#include <limits.h>
#include <sys/stat.h>

#include "core/sysinfo.h"
#include "core/self.h"
#include "core/tar.h"
#include "core/log.h"

#include "main.h"

int autotools_setup_upgrade_self(bool verbose) {
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

    const char * githubApiUrl = "https://api.github.com/repos/leleliu008/autotools-setup/releases/latest";

    size_t githubApiResultJsonFilePathCapacity = sessionDIRCapacity + 13U;
    char   githubApiResultJsonFilePath[githubApiResultJsonFilePathCapacity];

    ret = snprintf(githubApiResultJsonFilePath, githubApiResultJsonFilePathCapacity, "%s/latest.json", sessionDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    ret = autotools_setup_http_fetch_to_file(githubApiUrl, githubApiResultJsonFilePath, verbose, verbose);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    FILE * file = fopen(githubApiResultJsonFilePath, "r");

    if (file == NULL) {
        perror(githubApiResultJsonFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    char * latestReleaseTagName = NULL;
    size_t latestReleaseTagNameLength = 0U;

    char   latestVersion[11] = {0};
    size_t latestVersionLength = 0U;

    char * p = NULL;

    char line[70];

    for (;;) {
        p = fgets(line, 70, file);

        if (p == NULL) {
            if (ferror(file)) {
                perror(githubApiResultJsonFilePath);
                goto finalize;
            } else {
                break;
            }
        }

        for (;;) {
            if (p[0] <= 32) { // non-printable ASCII characters and space
                p++;
            } else {
                break;
            }
        }

        if (strncmp(p, "\"tag_name\"", 10) == 0) {
            p += 10;

            for (;;) {
                if (p[0] == '\0') {
                    fprintf(stderr, "%s return invalid json.\n", githubApiUrl);
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (p[0] == '"') { // found left double quote
                    p++;
                    break;
                } else {
                    p++;
                }
            }

            latestReleaseTagName = p;

            size_t n = 0;

            for (;;) {
                if (p[n] == '\0') {
                    fprintf(stderr, "%s return invalid json.\n", githubApiUrl);
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (p[n] == '"') { // found right double quote
                    p[n] = '\0';
                    latestReleaseTagNameLength = n;
                    goto finalize;
                } else {
                    n++;

                    if (p[n] == '+') {
                        latestVersionLength = n > 10 ? 10 : n;
                        strncpy(latestVersion, p, latestVersionLength);
                    }
                }
            }
        }
    }

finalize:
    fclose(file);

    printf("latestReleaseTagName=%s\n", latestReleaseTagName);

    if (latestReleaseTagName == NULL) {
        fprintf(stderr, "%s return json has no tag_name key.\n", githubApiUrl);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (latestVersion[0] == '\0') {
        fprintf(stderr, "%s return invalid json.\n", githubApiUrl);
        return AUTOTOOLS_SETUP_ERROR;
    }

    char    latestVersionCopy[latestVersionLength + 1U];
    strncpy(latestVersionCopy, latestVersion, latestVersionLength + 1U);

    char * latestVersionMajorStr = strtok(latestVersionCopy, ".");
    char * latestVersionMinorStr = strtok(NULL, ".");
    char * latestVersionPatchStr = strtok(NULL, ".");

    int latestVersionMajor = 0;
    int latestVersionMinor = 0;
    int latestVersionPatch = 0;

    if (latestVersionMajorStr != NULL) {
        latestVersionMajor = atoi(latestVersionMajorStr);
    }

    if (latestVersionMinorStr != NULL) {
        latestVersionMinor = atoi(latestVersionMinorStr);
    }

    if (latestVersionPatchStr != NULL) {
        latestVersionPatch = atoi(latestVersionPatchStr);
    }

    if (latestVersionMajor == 0 && latestVersionMinor == 0 && latestVersionPatch == 0) {
        fprintf(stderr, "invalid version format: %s\n", latestVersion);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (latestVersionMajor < AUTOTOOLS_SETUP_VERSION_MAJOR) {
        LOG_SUCCESS1("this software is already the latest version.");
        return AUTOTOOLS_SETUP_OK;
    } else if (latestVersionMajor == AUTOTOOLS_SETUP_VERSION_MAJOR) {
        if (latestVersionMinor < AUTOTOOLS_SETUP_VERSION_MINOR) {
            LOG_SUCCESS1("this software is already the latest version.");
            return AUTOTOOLS_SETUP_OK;
        } else if (latestVersionMinor == AUTOTOOLS_SETUP_VERSION_MINOR) {
            if (latestVersionPatch <= AUTOTOOLS_SETUP_VERSION_PATCH) {
                LOG_SUCCESS1("this software is already the latest version.");
                return AUTOTOOLS_SETUP_OK;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    char osType[31] = {0};

    if (sysinfo_type(osType, 30) < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    char osArch[31] = {0};

    if (sysinfo_arch(osArch, 30) < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t tarballFileNameCapacity = latestVersionLength + strlen(osType) + strlen(osArch) + 26U;
    char   tarballFileName[tarballFileNameCapacity];

    ret = snprintf(tarballFileName, tarballFileNameCapacity, "autotools-setup-%s-%s-%s.tar.xz", latestVersion, osType, osArch);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t tarballUrlCapacity = tarballFileNameCapacity + strlen(latestReleaseTagName) + 66U;
    char   tarballUrl[tarballUrlCapacity];

    ret = snprintf(tarballUrl, tarballUrlCapacity, "https://github.com/leleliu008/autotools-setup/releases/download/%s/%s", latestReleaseTagName, tarballFileName);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t tarballFilePathCapacity = sessionDIRCapacity + tarballFileNameCapacity + 2U;
    char   tarballFilePath[tarballFilePathCapacity];

    ret = snprintf(tarballFilePath, tarballFilePathCapacity, "%s/%s", sessionDIR, tarballFileName);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    ret = autotools_setup_http_fetch_to_file(tarballUrl, tarballFilePath, verbose, verbose);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    //////////////////////////////////////////////////////////////////////////////////

    ret = tar_extract(sessionDIR, tarballFilePath, 0, verbose, 1);

    if (ret != 0) {
        return abs(ret) + AUTOTOOLS_SETUP_ERROR_ARCHIVE_BASE;
    }

    size_t upgradableExecutableFilePathCapacity = sessionDIRCapacity + 20U;
    char   upgradableExecutableFilePath[upgradableExecutableFilePathCapacity];

    ret = snprintf(upgradableExecutableFilePath, upgradableExecutableFilePathCapacity, "%s/bin/autotools-setup", sessionDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    char * selfRealPath = self_realpath();

    if (selfRealPath == NULL) {
        perror(NULL);
        ret = AUTOTOOLS_SETUP_ERROR;
        goto finally;
    }

    if (rename(upgradableExecutableFilePath, selfRealPath) != 0) {
        if (errno == EXDEV) {
            if (unlink(selfRealPath) != 0) {
                perror(selfRealPath);
                ret = AUTOTOOLS_SETUP_ERROR;
                goto finally;
            }

            ret = autotools_setup_copy_file(upgradableExecutableFilePath, selfRealPath);

            if (ret != AUTOTOOLS_SETUP_OK) {
                goto finally;
            }

            if (chmod(selfRealPath, S_IRWXU) != 0) {
                perror(selfRealPath);
                ret = AUTOTOOLS_SETUP_ERROR;
            }
        } else {
            perror(selfRealPath);
            ret = AUTOTOOLS_SETUP_ERROR;
            goto finally;
        }
    }

finally:
    free(selfRealPath);

    if (ret == AUTOTOOLS_SETUP_OK) {
        fprintf(stderr, "autotools-setup is up to date with version %s\n", latestVersion);
    } else {
        fprintf(stderr, "Can't upgrade self. the latest version of executable was downloaded to %s, you can manually replace the current running program with it.\n", upgradableExecutableFilePath);
    }

    return ret;
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "autotools-setup.h"

int autotools_setup_integrate_zsh_completion(const char * outputDir, bool verbose) {
    const char * url = "https://raw.githubusercontent.com/leleliu008/autotools-setup/master/autotools-setup-zsh-completion";

    const char * userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0U) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    ////////////////////////////////////////////////////////////////

    struct stat st;

    size_t   autotoolsSetupHomeDirLength = userHomeDirLength + 18U;
    char     autotoolsSetupHomeDir[autotoolsSetupHomeDirLength];
    snprintf(autotoolsSetupHomeDir, autotoolsSetupHomeDirLength, "%s/.autotools-setup", userHomeDir);

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

    ////////////////////////////////////////////////////////////////

    size_t zshCompletionDirLength = autotoolsSetupHomeDirLength + 16U;
    char   zshCompletionDir[zshCompletionDirLength];
    snprintf(zshCompletionDir, zshCompletionDirLength, "%s/zsh_completion", autotoolsSetupHomeDir);

    if (stat(zshCompletionDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", zshCompletionDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        if (mkdir(zshCompletionDir, S_IRWXU) != 0) {
            perror(zshCompletionDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    ////////////////////////////////////////////////////////////////

    size_t   zshCompletionFilePathLength = zshCompletionDirLength + 18U;
    char     zshCompletionFilePath[zshCompletionFilePathLength];
    snprintf(zshCompletionFilePath, zshCompletionFilePathLength, "%s/_autotools-setup", zshCompletionDir);

    int ret = autotools_setup_http_fetch_to_file(url, zshCompletionFilePath, verbose, verbose);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    if (outputDir == NULL) {
        return AUTOTOOLS_SETUP_OK;
    }

    if (stat(outputDir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "'%s\n' was expected to be a directory, but it was not.\n", outputDir);
            return AUTOTOOLS_SETUP_ERROR;
        }
    } else {
        fprintf(stderr, "'%s\n' directory was expected to be exist, but it was not.\n", outputDir);
        return AUTOTOOLS_SETUP_ERROR;
    }

    size_t   destFilePathLength = strlen(outputDir) + 18U;
    char     destFilePath[destFilePathLength];
    snprintf(destFilePath, destFilePathLength, "%s/_autotools-setup", outputDir);

    if (symlink(zshCompletionFilePath, destFilePath) != 0) {
        perror(destFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    } else {
        return AUTOTOOLS_SETUP_OK;
    }
}

int autotools_setup_integrate_bash_completion(const char * outputDir, bool verbose) {
    (void)outputDir;
    (void)verbose;
    return AUTOTOOLS_SETUP_OK;
}

int autotools_setup_integrate_fish_completion(const char * outputDir, bool verbose) {
    (void)outputDir;
    (void)verbose;
    return AUTOTOOLS_SETUP_OK;
}

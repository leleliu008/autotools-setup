#include <stdio.h>
#include <string.h>

#include "core/self.h"
#include "autotools-setup.h"

int autotools_setup_show_runtime_info() {
    char * userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t   autotoolsSetupHomeDirLength = userHomeDirLength + 18U;
    char     autotoolsSetupHomeDir[autotoolsSetupHomeDirLength];
    snprintf(autotoolsSetupHomeDir, autotoolsSetupHomeDirLength, "%s/.autotools-setup", userHomeDir);

    printf("\n");
    printf("autotools-setup.vers : %s\n", AUTOTOOLS_SETUP_VERSION);
    printf("autotools-setup.home : %s\n", autotoolsSetupHomeDir);

    char * selfRealPath = self_realpath();

    if (selfRealPath == NULL) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    printf("autotools-setup.path : %s\n\n", selfRealPath);

    free(selfRealPath);

    return AUTOTOOLS_SETUP_OK;
}

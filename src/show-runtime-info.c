#include <stdio.h>
#include <string.h>

#include "core/self.h"

#include "main.h"

int autotools_setup_show_runtime_info() {
    const char * const userHomeDIR = getenv("HOME");

    if (userHomeDIR == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    if (userHomeDIR[0] == '\0') {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t autotoolsSetupHomeDIRCapacity = strlen(userHomeDIR) + 18U;
    char   autotoolsSetupHomeDIR[autotoolsSetupHomeDIRCapacity];

    int ret = snprintf(autotoolsSetupHomeDIR, autotoolsSetupHomeDIRCapacity, "%s/.autotools-setup", userHomeDIR);

    if (ret < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    printf("\n");
    printf("autotools-setup.vers : %s\n", AUTOTOOLS_SETUP_VERSION);
    printf("autotools-setup.home : %s\n", autotoolsSetupHomeDIR);

    char * selfRealPath = self_realpath();

    if (selfRealPath == NULL) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    printf("autotools-setup.path : %s\n\n", selfRealPath);

    free(selfRealPath);

    return AUTOTOOLS_SETUP_OK;
}

#include <stdio.h>

#include "autotools-setup.h"

extern int autotools_setup_show_build_info();
extern int autotools_setup_show_runtime_info();

int autotools_setup_env() {
    int ret = autotools_setup_show_build_info();

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    ret = autotools_setup_sysinfo();

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    ret = autotools_setup_show_runtime_info();

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    ///////////////////////////////////////////////

    printf("default config:\n\n");

    return autotools_setup_show_default_config();
}

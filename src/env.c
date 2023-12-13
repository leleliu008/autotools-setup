#include <stdio.h>

#include "main.h"

extern int autotools_setup_buildinfo();
extern int autotools_setup_show_runtime_info();

int autotools_setup_env() {
    int ret = autotools_setup_buildinfo();

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

    return autotools_setup_show_default_config();
}

#include <stdio.h>

#include "core/sysinfo.h"
#include "autotools-setup.h"

int autotools_setup_sysinfo() {
    SysInfo sysinfo = {0};

    if (sysinfo_make(&sysinfo) < 0) {
        perror(NULL);
        return AUTOTOOLS_SETUP_ERROR;
    }

    sysinfo_dump(sysinfo);
    sysinfo_free(sysinfo);
   
    return AUTOTOOLS_SETUP_OK;
}

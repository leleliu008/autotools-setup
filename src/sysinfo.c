#include "core/sysinfo.h"
#include "autotools-setup.h"

int autotools_setup_sysinfo() {
    SysInfo sysinfo = {0};

    int ret = sysinfo_make(&sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    sysinfo_dump(sysinfo);
    sysinfo_free(sysinfo);
   
    return AUTOTOOLS_SETUP_OK;
}

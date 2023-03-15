#include <stdio.h>

#include "core/http.h"
#include "autotools-setup.h"

int autotools_setup_http_fetch_to_file(const char * url, const char * outputFilePath, bool verbose, bool showProgress) {
    int ret = http_fetch_to_file(url, outputFilePath, verbose, showProgress);

    if (ret == -1) {
        perror(outputFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    if (ret > 0) {
        return AUTOTOOLS_SETUP_ERROR_NETWORK_BASE + ret;
    }

    return AUTOTOOLS_SETUP_OK;
}

#include <stdio.h>

#include "autotools-setup.h"

int autotools_setup_show_default_config() {
    printf("src-url-autoconf: %s\n",   DEFAULT_SRC_URL_AUTOCONF);
    printf("src-sha-autoconf: %s\n\n", DEFAULT_SRC_SHA_AUTOCONF);

    printf("src-url-automake: %s\n",   DEFAULT_SRC_URL_AUTOMAKE);
    printf("src-sha-automake: %s\n\n", DEFAULT_SRC_SHA_AUTOMAKE);

    printf("src-url-libtool:  %s\n",   DEFAULT_SRC_URL_LIBTOOL);
    printf("src-sha-libtool:  %s\n\n", DEFAULT_SRC_SHA_LIBTOOL);

    printf("src-url-pkgconf:  %s\n",   DEFAULT_SRC_URL_PKGCONF);
    printf("src-sha-pkgconf:  %s\n\n", DEFAULT_SRC_SHA_PKGCONF);

    printf("src-url-perl:     %s\n",   DEFAULT_SRC_URL_PERL);
    printf("src-sha-perl:     %s\n\n", DEFAULT_SRC_SHA_PERL);

    printf("src-url-gm4:      %s\n",   DEFAULT_SRC_URL_GM4);
    printf("src-sha-gm4:      %s\n\n", DEFAULT_SRC_SHA_GM4);

    return AUTOTOOLS_SETUP_OK;
}

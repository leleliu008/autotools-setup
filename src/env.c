#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include "core/sysinfo.h"
#include "core/self.h"
#include "autotools-setup.h"

#include <zlib.h>
#include <yaml.h>
#include <archive.h>
#include <curl/curlver.h>
#include <openssl/opensslv.h>

//#define PCRE2_CODE_UNIT_WIDTH 8
//#include <pcre2.h>

int autotools_setup_env() {
    printf("build.utctime: %s\n\n", AUTOTOOLS_SETUP_BUILD_TIMESTAMP);

    //printf("pcre2   : %d.%d\n", PCRE2_MAJOR, PCRE2_MINOR);
    printf("build.libyaml: %s\n", yaml_get_version_string());
    printf("build.libcurl: %s\n", LIBCURL_VERSION);

//https://www.openssl.org/docs/man3.0/man3/OPENSSL_VERSION_BUILD_METADATA.html
//https://www.openssl.org/docs/man1.1.1/man3/OPENSSL_VERSION_TEXT.html
#ifdef OPENSSL_VERSION_STR
    printf("build.openssl: %s\n", OPENSSL_VERSION_STR);
#else
    printf("build.openssl: %s\n", OPENSSL_VERSION_TEXT);
#endif

    printf("build.archive: %s\n", ARCHIVE_VERSION_ONLY_STRING);
    printf("build.zlib:    %s\n\n", ZLIB_VERSION);

    SysInfo sysinfo = {0};

    int ret = sysinfo_make(&sysinfo);

    if (ret != AUTOTOOLS_SETUP_OK) {
        return ret;
    }

    sysinfo_dump(sysinfo);
    sysinfo_free(sysinfo);

    char * userHomeDir = getenv("HOME");

    if (userHomeDir == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t userHomeDirLength = strlen(userHomeDir);

    if (userHomeDirLength == 0) {
        return AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET;
    }

    size_t   autotoolsSetupHomeDirLength = userHomeDirLength + 18;
    char     autotoolsSetupHomeDir[autotoolsSetupHomeDirLength];
    snprintf(autotoolsSetupHomeDir, autotoolsSetupHomeDirLength, "%s/.autotools-setup", userHomeDir);

    printf("\n");
    printf("autotools-setup.vers : %s\n", AUTOTOOLS_SETUP_VERSION);
    printf("autotools-setup.home : %s\n", autotoolsSetupHomeDir);

    char * path = NULL;

    self_realpath(&path);

    printf("autotools-setup.path : %s\n\n", path == NULL ? "" : path);

    if (path != NULL) {
        free(path);
    }

    printf("default config:\n\n");

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

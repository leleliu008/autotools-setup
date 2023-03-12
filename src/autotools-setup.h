#ifndef AUTOTOOLS_SETUP_H
#define AUTOTOOLS_SETUP_H

#include <config.h>
#include <stdlib.h>
#include <stdbool.h>

 
#define AUTOTOOLS_SETUP_OK                     0
#define AUTOTOOLS_SETUP_ERROR                  1

#define AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL      2
#define AUTOTOOLS_SETUP_ERROR_ARG_IS_EMPTY     3
#define AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID   4
#define AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN   5

#define AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE  6

#define AUTOTOOLS_SETUP_ERROR_SHA256_MISMATCH  7

#define AUTOTOOLS_SETUP_ERROR_ENV_HOME_NOT_SET 8
#define AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET 9

#define AUTOTOOLS_SETUP_ERROR_EXE_NOT_FOUND    10

#define AUTOTOOLS_SETUP_ERROR_FORMULA_SYNTAX   45
#define AUTOTOOLS_SETUP_ERROR_FORMULA_SCHEME   46

// libgit's error [-35, -1]
#define AUTOTOOLS_SETUP_ERROR_LIBGIT2_BASE    70

// libarchive's error [-30, 1]
#define AUTOTOOLS_SETUP_ERROR_ARCHIVE_BASE    110

// libcurl's error [1, 99]
#define AUTOTOOLS_SETUP_ERROR_NETWORK_BASE    150

////////////////////////////////////////////////////

#define DEFAULT_SRC_URL_AUTOCONF "https://ftp.gnu.org/gnu/autoconf/autoconf-2.71.tar.gz"
#define DEFAULT_SRC_SHA_AUTOCONF "431075ad0bf529ef13cb41e9042c542381103e80015686222b8a9d4abef42a1c"

#define DEFAULT_SRC_URL_AUTOMAKE "https://ftp.gnu.org/gnu/automake/automake-1.16.5.tar.xz"
#define DEFAULT_SRC_SHA_AUTOMAKE "f01d58cd6d9d77fbdca9eb4bbd5ead1988228fdb73d6f7a201f5f8d6b118b469"

#define DEFAULT_SRC_URL_LIBTOOL  "https://ftp.gnu.org/gnu/libtool/libtool-2.4.7.tar.xz"
#define DEFAULT_SRC_SHA_LIBTOOL  "4f7f217f057ce655ff22559ad221a0fd8ef84ad1fc5fcb6990cecc333aa1635d"

#define DEFAULT_SRC_URL_PKGCONF  "http://distfiles.dereferenced.org/pkgconf/pkgconf-1.9.3.tar.xz"
#define DEFAULT_SRC_SHA_PKGCONF  "5fb355b487d54fb6d341e4f18d4e2f7e813a6622cf03a9e87affa6a40565699d"

#define DEFAULT_SRC_URL_PERL     "https://cpan.metacpan.org/authors/id/R/RJ/RJBS/perl-5.36.0.tar.xz"
#define DEFAULT_SRC_SHA_PERL     "0f386dccbee8e26286404b2cca144e1005be65477979beb9b1ba272d4819bcf0"

#define DEFAULT_SRC_URL_GM4      "https://ftp.gnu.org/gnu/m4/m4-1.4.19.tar.xz"
#define DEFAULT_SRC_SHA_GM4      "63aede5c6d33b6d9b13511cd0be2cac046f2e70fd0a07aa9573a04a82783af96"

////////////////////////////////////////////////////

typedef struct {
    char * src_url_automake;
    char * src_sha_automake;

    char * src_url_autoconf;
    char * src_sha_autoconf;

    char * src_url_libtool;
    char * src_sha_libtool;

    char * src_url_pkgconf;
    char * src_sha_pkgconf;

    char * src_url_perl;
    char * src_sha_perl;

    char * src_url_gm4;
    char * src_sha_gm4;
} AutotoolsSetupConfig;

int  autotools_setup_config_parse(const char * configFilePath, AutotoolsSetupConfig * * out);
void autotools_setup_config_free(AutotoolsSetupConfig * config);
void autotools_setup_config_dump(AutotoolsSetupConfig * config);

////////////////////////////////////////////////////

int autotools_setup_main(int argc, char* argv[]);

int autotools_setup_help();

int autotools_setup_sysinfo();

int autotools_setup_env();

int autotools_setup_show_default_config();

int autotools_setup_upgrade_self(bool verbose);

int autotools_setup_integrate_zsh_completion (const char * outputDir, bool verbose);
int autotools_setup_integrate_bash_completion(const char * outputDir, bool verbose);
int autotools_setup_integrate_fish_completion(const char * outputDir, bool verbose);

typedef enum {
    AutotoolsSetupLogLevel_silent,
    AutotoolsSetupLogLevel_normal,
    AutotoolsSetupLogLevel_verbose,
    AutotoolsSetupLogLevel_very_verbose
} AutotoolsSetupLogLevel;

int autotools_setup_setup(const char * configFilePath, const char * setupDir, AutotoolsSetupLogLevel logLevel, unsigned int jobs);

#endif

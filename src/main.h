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

#define DEFAULT_SRC_URL_PKGCONF  "http://distfiles.dereferenced.org/pkgconf/pkgconf-2.1.0.tar.xz"
#define DEFAULT_SRC_SHA_PKGCONF  "266d5861ee51c52bc710293a1d36622ae16d048d71ec56034a02eb9cf9677761"

#define DEFAULT_SRC_URL_PERL     "https://www.cpan.org/src/5.0/perl-5.38.0.tar.xz"
#define DEFAULT_SRC_SHA_PERL     "eca551caec3bc549a4e590c0015003790bdd1a604ffe19cc78ee631d51f7072e"

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

int autotools_setup_util(int argc, char* argv[]);

int autotools_setup_help();

int autotools_setup_sysinfo();

int autotools_setup_buildinfo();

int autotools_setup_env();

int autotools_setup_show_default_config();

int autotools_setup_upgrade_self(bool verbose);

int autotools_setup_integrate_zsh_completion (const char * outputDIR, bool verbose);
int autotools_setup_integrate_bash_completion(const char * outputDIR, bool verbose);
int autotools_setup_integrate_fish_completion(const char * outputDIR, bool verbose);

typedef enum {
    AutotoolsSetupLogLevel_silent,
    AutotoolsSetupLogLevel_normal,
    AutotoolsSetupLogLevel_verbose,
    AutotoolsSetupLogLevel_very_verbose
} AutotoolsSetupLogLevel;

int autotools_setup_setup(const char * configFilePath, const char * setupDIR, AutotoolsSetupLogLevel logLevel, unsigned int jobs);

int autotools_setup_http_fetch_to_file(const char * url, const char * outputFilePath, bool verbose, bool showProgress);

int autotools_setup_copy_file(const char * fromFilePath, const char * toFilePath);

int autotools_setup_mkdir_p(const char * dirPath, bool verbose);

int autotools_setup_rm_r(const char * dirPath, bool verbose);

int autotools_setup_examine_file_extension_from_url(const char * url, char buf[], size_t bufSize);

int autotools_setup_generate_url_transform_sample();

#endif

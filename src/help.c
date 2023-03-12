#include <unistd.h>
#include "core/log.h"

int autotools_setup_help() {
    if (isatty(STDOUT_FILENO)) {
        const char * str = ""
        COLOR_GREEN
        "autotools-setup is a command-line tool to setup GNU Autotools (including automake, autoconf, libtool, pkgconf, perl, gm4, gmake)\n\n"
        "autotools-setup <ACTION> [ARGUMENT...]\n\n"
        "autotools-setup --help\n"
        "autotools-setup -h\n"
        COLOR_OFF
        "    show help of this command.\n\n"
        COLOR_GREEN
        "autotools-setup --version\n"
        "autotools-setup -V\n"
        COLOR_OFF
        "    show version of this command.\n\n"
        COLOR_GREEN
        "autotools-setup sysinfo\n"
        COLOR_OFF
        "    show your system's information.\n\n"
        COLOR_GREEN
        "autotools-setup env\n"
        COLOR_OFF
        "    show your system's information and other information.\n\n"
        COLOR_GREEN
        "autotools-setup integrate zsh [-v] [--output-dir=<DIR>]\n"
        COLOR_OFF
        "    download a zsh completion script file to a approprivate location.\n\n"
        "    to apply this feature, you may need to run the command 'autoload -U compinit && compinit' in your terminal (your current running shell must be zsh).\n\n"
        COLOR_GREEN
        "autotools-setup setup [--prefix=<DIR> --jobs=N -q -v -vv --config=<FILEPATH>]\n"
        COLOR_OFF
        "    build and install GNU Autotools and relevant packages (including automake, autoconf, libtool, pkgconf, perl, gm4, gmake) to the specified directory.\n\n"
        "    if --prefix=<DIR> option is not given, then these packages will be installed into '~/.autotools-setup/autotools' directory.\n\n"
        COLOR_RED
        "    C compiler should be installed by yourself using your system's default package manager before running this command.\n\n"
        COLOR_OFF
        COLOR_GREEN
        "autotools-setup util zlib-deflate -L <LEVEL> < input/file/path\n"
        COLOR_OFF
        "    compress data using zlib deflate algorithm.\n\n"
        "    LEVEL >= 1 && LEVEL <= 9\n\n"
        "    The smaller the LEVEL, the faster the speed and the lower the compression ratio.\n\n"
        COLOR_GREEN
        "autotools-setup util zlib-inflate < input/file/path\n"
        COLOR_OFF
        "    decompress data using zlib inflate algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base16-encode <STR>\n"
        COLOR_OFF
        "    encode <STR> using base16 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base16-encode < input/file/path\n"
        COLOR_OFF
        "    encode data using base16 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base16-decode <BASE16-ENCODED-SUM>\n"
        COLOR_OFF
        "    decode <BASE16-ENCODED-SUM> using base16 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base16-decode < input/file/path\n"
        COLOR_OFF
        "    decode data using base16 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base64-encode <STR>\n"
        COLOR_OFF
        "    encode <STR> using base64 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base64-encode < input/file/path\n"
        COLOR_OFF
        "    encode data using base64 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base64-decode <BASE64-ENCODED-SUM>\n"
        COLOR_OFF
        "    decode <BASE64-ENCODED-SUM> using base64 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util base64-decode < input/file/path\n"
        COLOR_OFF
        "    decode data using base64 algorithm.\n\n"
        COLOR_GREEN
        "autotools-setup util sha256sum <input/file/path>\n"
        COLOR_OFF
        "    calculate sha256sum of file.\n\n"
        COLOR_GREEN
        "autotools-setup util sha256sum < input/file/path\n"
        COLOR_OFF
        "    calculate sha256sum of file.\n\n"
        COLOR_GREEN
        "autotools-setup util which <COMMAND-NAME> [-a]\n"
        COLOR_OFF
        "    find <COMMAND-NAME> in PATH.\n"
        ;

        printf("%s\n", str);
    } else {
        const char * str = ""
        "autotools-setup is a command-line tool to setup GNU Autotools (including automake, autoconf, libtool, pkgconf, perl, gm4, gmake)\n\n"
        "autotools-setup <ACTION> [ARGUMENT...]\n\n"
        "autotools-setup --help\n"
        "autotools-setup -h\n"
        "    show help of this command.\n\n"
        "autotools-setup --version\n"
        "autotools-setup -V\n"
        "    show version of this command.\n\n"
        "autotools-setup sysinfo\n"
        "    show your system's information.\n\n"
        "autotools-setup env\n"
        "    show your system's information and other information.\n\n"
        "autotools-setup setup [--prefix=<DIR> --jobs=N -q -v -vv --config=<FILEPATH>]\n"
        "    build and install GNU Autotools and relevant packages (including automake, autoconf, libtool, pkgconf, perl, gm4, gmake) to the specified directory.\n\n"
        "    if --prefix=<DIR> option is not given, then these packages will be installed into '~/.autotools-setup/autotools' directory.\n\n"
        "    C compiler should be installed by yourself using your system's default package manager before running this command.\n\n"
        "autotools-setup util zlib-deflate -L <LEVEL> < input/file/path\n"
        "    compress data using zlib deflate algorithm.\n\n"
        "    LEVEL >= 1 && LEVEL <= 9\n\n"
        "    The smaller the LEVEL, the faster the speed and the lower the compression ratio.\n\n"
        "autotools-setup util zlib-inflate < input/file/path\n"
        "    decompress data using zlib inflate algorithm.\n\n"
        "autotools-setup util base16-encode <STR>\n"
        "    encode <STR> using base16 algorithm.\n\n"
        "autotools-setup util base16-encode < input/file/path\n"
        "    encode data using base16 algorithm.\n\n"
        "autotools-setup util base16-decode <BASE16-ENCODED-SUM>\n"
        "    decode <BASE16-ENCODED-SUM> using base16 algorithm.\n\n"
        "autotools-setup util base16-decode < input/file/path\n"
        "    decode data using base16 algorithm.\n\n"
        "autotools-setup util base64-encode <STR>\n"
        "    encode <STR> using base64 algorithm.\n\n"
        "autotools-setup util base64-encode < input/file/path\n"
        "    encode data using base64 algorithm.\n\n"
        "autotools-setup util base64-decode <BASE64-ENCODED-SUM>\n"
        "    decode <BASE64-ENCODED-SUM> using base64 algorithm.\n\n"
        "autotools-setup util base64-decode < input/file/path\n"
        "    decode data using base64 algorithm.\n\n"
        "autotools-setup util sha256sum <input/file/path>\n"
        "    calculate sha256sum of file.\n\n"
        "autotools-setup util sha256sum < input/file/path\n"
        "    calculate sha256sum of file.\n\n"
        "autotools-setup util which <COMMAND-NAME> [-a]\n"
        "    find <COMMAND-NAME> in PATH.\n"
        ;

        printf("%s\n", str);
    }

    return 0;
}

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#include "main.h"

int autotools_setup_mkdir_p(const char * dir, bool verbose) {
    if (dir == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
    }

    if (dir[0] == '\0') {
        return AUTOTOOLS_SETUP_ERROR_ARG_IS_EMPTY;
    }

    size_t len = strlen(dir);

    char buf[len + 1U];

    memset(buf, 0, len + 1U);

    struct stat st;

    int i = 0;

    for (;;) {
        char c = dir[i];

        if (c == '/' || c == '\0') {
            if (buf[0] != '\0') {
                if (stat(buf, &st) == 0) {
                    if (!S_ISDIR(st.st_mode)) {
                        fprintf(stderr, "%s was expected to be a directory, but it was not.\n", buf);
                        return AUTOTOOLS_SETUP_ERROR;
                    }
                } else {
                    if (verbose) printf("mkdir -p %s\n", buf);

                    if (mkdir(buf, S_IRWXU) != 0) {
                        if (errno == EEXIST) {
                            if (stat(buf, &st) == 0) {
                                if (!S_ISDIR(st.st_mode)) {
                                    fprintf(stderr, "%s was expected to be a directory, but it was not.\n", buf);
                                    return AUTOTOOLS_SETUP_ERROR;
                                }
                            }
                        } else {
                            perror(buf);
                            return AUTOTOOLS_SETUP_ERROR;
                        }
                    }
                }
            }
        }

        if (c == '\0') {
            return 0;
        } else {
            buf[i] = c;
            i++;
        }
    }
}

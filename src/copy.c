#include <stdio.h>

#include <fcntl.h>
#include <unistd.h>

#include "main.h"

int autotools_setup_copy_file(const char * fromFilePath, const char * toFilePath) {
    int fromFD = open(fromFilePath, O_RDONLY);

    if (fromFD == -1) {
        perror(fromFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    int toFD = open(toFilePath, O_CREAT | O_TRUNC | O_WRONLY, 0666);

    if (toFD == -1) {
        perror(toFilePath);
        close(fromFD);
        return AUTOTOOLS_SETUP_ERROR;
    }

    unsigned char buf[1024];

    for (;;) {
        ssize_t readSize = read(fromFD, buf, 1024);

        if (readSize == -1) {
            perror(fromFilePath);
            close(fromFD);
            close(toFD);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (readSize == 0) {
            close(fromFD);
            close(toFD);
            return 0;
        }

        ssize_t writeSize = write(toFD, buf, readSize);

        if (writeSize == -1) {
            perror(toFilePath);
            close(fromFD);
            close(toFD);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (writeSize != readSize) {
            close(fromFD);
            close(toFD);
            fprintf(stderr, "not fully written to %s\n", toFilePath);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }
}

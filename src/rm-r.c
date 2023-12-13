#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "main.h"

int autotools_setup_rm_r(const char * dirPath, bool verbose) {
    if (dirPath == NULL) {
        return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
    }

    if (dirPath[0] == '\0') {
        return AUTOTOOLS_SETUP_ERROR_ARG_IS_EMPTY;
    }

    size_t dirPathLength = strlen(dirPath);

    struct stat st;

    if (lstat(dirPath, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            if (verbose) {
                printf("rm %s\n", dirPath);
            }

            if (unlink(dirPath) == 0) {
                return AUTOTOOLS_SETUP_OK;
            } else {
                perror(dirPath);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }
    } else {
        // why does this happened?
        // Suppose you have following directory structure:
        // bin
        // ├── gsed
        // └── sed -> gsed
        // if bin/gsed was removed, then bin/sed points to a non-existent file. In this case, bin/sed is known as a dangling link.

        if (verbose) {
            printf("rm %s\n", dirPath);
        }

        if (unlink(dirPath) == 0) {
            return AUTOTOOLS_SETUP_OK;
        } else {
            perror(dirPath);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    DIR * dir = opendir(dirPath);

    if (dir == NULL) {
        perror(dirPath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    for (;;) {
        errno = 0;

        struct dirent * dir_entry = readdir(dir);

        if (dir_entry == NULL) {
            if (errno == 0) {
                closedir(dir);

                if (verbose) {
                    printf("rm %s\n", dirPath);
                }

                if (rmdir(dirPath) == 0) {
                    return AUTOTOOLS_SETUP_OK;
                } else {
                    perror(dirPath);
                    return AUTOTOOLS_SETUP_ERROR;
                }
            } else {
                perror(dirPath);
                closedir(dir);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }

        if ((strcmp(dir_entry->d_name, ".") == 0) || (strcmp(dir_entry->d_name, "..") == 0)) {
            continue;
        }

        size_t filePathCapacity = dirPathLength + strlen(dir_entry->d_name) + 2U;
        char   filePath[filePathCapacity];

        int ret = snprintf(filePath, filePathCapacity, "%s/%s", dirPath, dir_entry->d_name);

        if (ret < 0) {
            perror(NULL);
            return AUTOTOOLS_SETUP_ERROR;
        }

        if (lstat(filePath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                int ret = autotools_setup_rm_r(filePath, verbose);

                if (ret != AUTOTOOLS_SETUP_OK) {
                    return ret;
                }
            } else {
                if (verbose) {
                    printf("rm %s\n", filePath);
                }

                if (unlink(filePath) != 0) {
                    perror(filePath);
                    closedir(dir);
                    return AUTOTOOLS_SETUP_ERROR;
                }
            }
        } else {
            // why does this happened?
            // Suppose you have following directory structure:
            // bin
            // ├── gsed
            // └── sed -> gsed
            // if bin/gsed was removed, then bin/sed points to a non-existent file. In this case, bin/sed is known as a dangling link.

            if (verbose) {
                printf("rm %s\n", filePath);
            }

            if (unlink(filePath) != 0) {
                perror(filePath);
                closedir(dir);
                return AUTOTOOLS_SETUP_ERROR;
            }
        }
    }
}

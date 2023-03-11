#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#if defined (__APPLE__)
#include <sys/syslimits.h>
#elif defined (__linux__) && defined (HAVE_LINUX_LIMITS_H)
#include <linux/limits.h>
#else
#include <limits.h>
#endif

#if defined (__FreeBSD__) || defined (__OpenBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include "self.h"

int self_realpath(char * * out) {
#if defined (__APPLE__)
    char buf[PATH_MAX + 1] = {0};

    uint32_t bufSize = 0U;
    _NSGetExecutablePath(NULL, &bufSize);

    char path[bufSize];
    _NSGetExecutablePath(path, &bufSize);

    if (realpath(path, buf) == NULL) {
        (*out) = NULL;
        return AUTOTOOLS_SETUP_ERROR;
    }

    (*out) = strdup(buf);

    return (*out) == NULL ? -1 : 0;
#elif defined (__FreeBSD__)
    const int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };

    size_t bufLength = 0;

    if (sysctl(mib, 4, NULL, &bufLength, NULL, 0) < 0) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    char * buf = (char*)calloc(bufLength + 1, sizeof(char));

    if (buf == NULL) {
        return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
    }

    if (sysctl(mib, 4, buf, &bufLength, NULL, 0) < 0) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    (*out) = buf;
    return AUTOTOOLS_SETUP_OK;
#elif defined (__OpenBSD__)
    const int mib[4] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV };
    size_t size;

    if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    char** argv = (char**)malloc(size);

    if (argv == NULL) {
        return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
    }

    memset(argv, 0, size);

    if (sysctl(mib, 4, argv, &size, NULL, 0) != 0) {
        return AUTOTOOLS_SETUP_ERROR;
    }

    bool isPath = false;

    char c;

    char * p = argv[0];

    for (;;) {
        c = p[0];

        if (c == '\0') {
            break;
        }

        if (c == '/') {
            isPath = true;
            break;
        }

        p++;
    }

    if (isPath) {
        (*out) = realpath(argv[0], NULL);
        return (*out) == NULL ? AUTOTOOLS_SETUP_ERROR : AUTOTOOLS_SETUP_OK;
    } else {
        char * PATH = getenv("PATH");

        if (PATH == NULL) {
            return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
        }

        size_t PATHLength = strlen(PATH);

        if (PATHLength == 0) {
            return AUTOTOOLS_SETUP_ERROR_ENV_PATH_NOT_SET;
        }

        size_t  PATH2Length = PATHLength + 1;
        char    PATH2[PATH2Length];
        strncpy(PATH2, PATH, PATH2Length);

        size_t commandNameLength = strlen(argv[0]);

        char * PATHItem = strtok(PATH2, ":");

        while (PATHItem != NULL) {
            struct stat st;

            if ((stat(PATHItem, &st) == 0) && S_ISDIR(st.st_mode)) {
                size_t  fullPathLength = strlen(PATHItem) + commandNameLength + 2;
                char    fullPath[fullPathLength];
                snprintf(fullPath, fullPathLength, "%s/%s", PATHItem, argv[0]);

                if (access(fullPath, X_OK) == 0) {
                    (*out) = strdup(fullPath);
                    return (*out) == NULL ? AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE : AUTOTOOLS_SETUP_OK;
                }
            }

            PATHItem = strtok(NULL, ":");
        }

        return AUTOTOOLS_SETUP_ERROR;
    }
#else
    char buf[PATH_MAX + 1] = {0};

    int ret = readlink("/proc/self/exe", buf, PATH_MAX);

    if (ret == -1) {
        perror("/proc/self/exe");
        return AUTOTOOLS_SETUP_ERROR;
    }

    (*out) = strdup(buf);
    return (*out) == NULL ? AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE : AUTOTOOLS_SETUP_OK;
#endif
}

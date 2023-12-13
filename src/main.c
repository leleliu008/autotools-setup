#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "core/log.h"

#include "main.h"

int autotools_setup_main(int argc, char* argv[]) {
    if (argc == 1) {
        autotools_setup_help();
        return AUTOTOOLS_SETUP_OK;
    }

    if ((strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)) {
        autotools_setup_help();
        return AUTOTOOLS_SETUP_OK;
    }

    if ((strcmp(argv[1], "-V") == 0) || (strcmp(argv[1], "--version") == 0)) {
        printf("%s\n", AUTOTOOLS_SETUP_VERSION);
        return AUTOTOOLS_SETUP_OK;
    }

    if (strcmp(argv[1], "sysinfo") == 0) {
        return autotools_setup_sysinfo();
    }

    if (strcmp(argv[1], "buildinfo") == 0) {
        return autotools_setup_buildinfo();
    }

    if (strcmp(argv[1], "env") == 0) {
        return autotools_setup_env();
    }

    if (strcmp(argv[1], "config") == 0) {
        return autotools_setup_show_default_config();
    }

    if (strcmp(argv[1], "gen-url-transform-sample") == 0) {
        return autotools_setup_generate_url_transform_sample();
    }

    if (strcmp(argv[1], "upgrade-self") == 0) {
        bool verbose = false;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-v") == 0) {
                verbose = true;
                break;
            }
        }

        return autotools_setup_upgrade_self(verbose);
    }

    if (strcmp(argv[1], "integrate") == 0) {
        bool verbose = false;

        if (argv[2] == NULL) {
            fprintf(stderr, "Usage: %s integrate <zsh|bash|fish>\n", argv[0]);
            return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
        } else if (strcmp(argv[2], "zsh") == 0) {
            char * outputDIRPath = NULL;

            for (int i = 3; i < argc; i++) {
                if (strncmp(argv[i], "--output-dir=", 13) == 0) {
                    if (argv[i][13] == '\0') {
                        fprintf(stderr, "--output-dir=VALUE, VALUE should be a non-empty string.\n");
                        return AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID;
                    } else {
                        outputDIRPath = &argv[i][13];
                    }
                } else if (strcmp(argv[i], "-v") == 0) {
                    verbose = true;
                } else {
                    fprintf(stderr, "unrecognized option: %s\n", argv[i]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN;
                }
            }

            return autotools_setup_integrate_zsh_completion (outputDIRPath, verbose);
        } else if (strcmp(argv[2], "bash") == 0) {
            return autotools_setup_integrate_bash_completion(NULL, verbose);
        } else if (strcmp(argv[2], "fish") == 0) {
            return autotools_setup_integrate_fish_completion(NULL, verbose);
        } else {
            LOG_ERROR2("unrecognized argument: ", argv[2]);
            return AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID;
        }
    }


    if (strcmp(argv[1], "setup") == 0) {
        AutotoolsSetupLogLevel logLevel = AutotoolsSetupLogLevel_normal;

        const char * configFilePath = NULL;
        const char * setupDIR = NULL;

        unsigned int jobs = 0U;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-q") == 0) {
                logLevel = AutotoolsSetupLogLevel_silent;
            } else if (strcmp(argv[i], "-v") == 0) {
                logLevel = AutotoolsSetupLogLevel_verbose;
            } else if (strcmp(argv[i], "-vv") == 0) {
                logLevel = AutotoolsSetupLogLevel_very_verbose;
            } else if (strncmp(argv[i], "--jobs=", 7) == 0) {
                if (argv[i][7] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config=<FILEPATH> --jobs=<N>], <N> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    const char * jobsStr = argv[i] + 7;
                    jobs = atoi(jobsStr);
                }
            } else if (strncmp(argv[i], "--config=", 9) == 0) {
                if (argv[i][9] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config=<FILEPATH> --jobs=<N>], <FILEPATH> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    configFilePath = argv[i] + 9;
                }
            } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
                if (argv[i][9] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config=<FILEPATH> --jobs=<N>], <DIR> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    setupDIR = argv[i] + 9;
                }
            } else {
                fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config=<FILEPATH> --jobs=<N>]\n", argv[0], argv[1]);
                return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
            }
        }

        return autotools_setup_setup(configFilePath, setupDIR, logLevel, jobs);
    }

    if (strcmp(argv[1], "util") == 0) {
        return autotools_setup_util(argc, argv);
    }

    LOG_ERROR2("unrecognized action: ", argv[1]);
    return AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN;
}

int main(int argc, char* argv[]) {
    return autotools_setup_main(argc, argv);
}

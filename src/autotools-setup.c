#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/evp.h>

#include "core/regex/regex.h"
#include "core/zlib-flate.h"
#include "core/sha256sum.h"
#include "core/base16.h"
#include "core/base64.h"
#include "core/exe.h"
#include "core/log.h"
#include "autotools-setup.h"

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

    if (strcmp(argv[1], "env") == 0) {
        return autotools_setup_env();
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
            char * outputDirPath = NULL;

            for (int i = 3; i < argc; i++) {
                if (strncmp(argv[i], "--output-dir=", 13) == 0) {
                    if (argv[i][13] == '\0') {
                        fprintf(stderr, "--output-dir=VALUE, VALUE should be a non-empty string.\n");
                        return AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID;
                    } else {
                        outputDirPath = &argv[i][13];
                    }
                } else if (strcmp(argv[i], "-v") == 0) {
                    verbose = true;
                } else {
                    fprintf(stderr, "unrecognized option: %s\n", argv[i]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN;
                }
            }

            return autotools_setup_integrate_zsh_completion (outputDirPath, verbose);
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
        AUTOTOOLSSETUPLogLevel logLevel = AUTOTOOLSSETUPLogLevel_normal;

        const char * configFilePath = NULL;
        const char * setupDir = NULL;

        unsigned int jobs = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-q") == 0) {
                logLevel = AUTOTOOLSSETUPLogLevel_silent;
            } else if (strcmp(argv[i], "-v") == 0) {
                logLevel = AUTOTOOLSSETUPLogLevel_verbose;
            } else if (strcmp(argv[i], "-vv") == 0) {
                logLevel = AUTOTOOLSSETUPLogLevel_very_verbose;
            } else if (strncmp(argv[i], "--jobs=", 7) == 0) {
                if (argv[i][7] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config-file=<FILEPATH> --jobs=<N>], <N> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    const char * jobsStr = argv[i] + 7;
                    jobs = atoi(jobsStr);
                }
            } else if (strncmp(argv[i], "--config-file=", 14) == 0) {
                if (argv[i][14] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config-file=<FILEPATH> --jobs=<N>], <FILEPATH> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    configFilePath = argv[i] + 14;
                }
            } else if (strncmp(argv[i], "--prefix=", 9) == 0) {
                if (argv[i][9] == '\0') {
                    fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix=<DIR> --config-file=<FILEPATH> --jobs=<N>], <DIR> should be a non-empty string.\n", argv[0], argv[1]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                } else {
                    setupDir = argv[i] + 9;
                }
            } else {
                fprintf(stderr, "Usage: %s %s [-q | -v | -vv | --prefix <DIR> --config-file=<FILEPATH> --jobs=<N>]\n", argv[0], argv[1]);
                return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
            }
        }

        return autotools_setup_setup(configFilePath, setupDir, logLevel, jobs);
    }

    if (strcmp(argv[1], "util") == 0) {
        if (argv[2] == NULL) {
            fprintf(stderr, "Usage: %s %s <COMMAND> , <COMMAND> is not given.\n", argv[0], argv[1]);
            return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
        }

        if (strcmp(argv[2], "base16-encode") == 0) {
            if (argv[3] == NULL) {
                unsigned char inputBuff[1024];
                unsigned int  inputSizeInBytes;

                for (;;) {
                    inputSizeInBytes = fread(inputBuff, 1, 1024, stdin);

                    if (ferror(stdin)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    size_t outputSizeInBytes = inputSizeInBytes << 1;
                    char   outputBuff[outputSizeInBytes];

                    int ret = base16_encode(outputBuff, inputBuff, inputSizeInBytes, false);

                    if (ret != AUTOTOOLS_SETUP_OK) {
                        return ret;
                    }

                    if (fwrite(outputBuff, 1, outputSizeInBytes, stdout) != outputSizeInBytes || ferror(stdout)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    if (feof(stdin)) {
                        if (isatty(STDOUT_FILENO)) {
                            printf("\n");
                        }

                        return AUTOTOOLS_SETUP_OK;
                    }
                }
            } else {
                unsigned char * inputBuff = (unsigned char *)argv[3];
                size_t          inputSizeInBytes = strlen(argv[3]);

                if (inputSizeInBytes == 0) {
                    fprintf(stderr, "Usage: %s %s %s <STR> , <STR> should be non-empty.\n", argv[0], argv[1], argv[2]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                }

                size_t outputSizeInBytes = inputSizeInBytes << 1;
                char   outputBuff[outputSizeInBytes];

                int ret = base16_encode(outputBuff, inputBuff, inputSizeInBytes, false);

                if (ret != AUTOTOOLS_SETUP_OK) {
                    return ret;
                }

                if (fwrite(outputBuff, 1, outputSizeInBytes, stdout) != outputSizeInBytes || ferror(stdout)) {
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (isatty(STDOUT_FILENO)) {
                    printf("\n");
                }

                return AUTOTOOLS_SETUP_OK;
            }
        }

        if (strcmp(argv[2], "base16-decode") == 0) {
            if (argv[3] == NULL) {
                fprintf(stderr, "Usage: %s %s %s <BASE16-DECODED-STR> , <BASE16-DECODED-STR> is not given.\n", argv[0], argv[1], argv[2]);
                return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
            }

            size_t inputSizeInBytes = strlen(argv[3]);

            if (inputSizeInBytes == 0) {
                fprintf(stderr, "Usage: %s %s %s <BASE16-DECODED-STR> , <BASE16-DECODED-STR> should be non-empty.\n", argv[0], argv[1], argv[2]);
                return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
            }

            if ((inputSizeInBytes & 1) != 0) {
                fprintf(stderr, "Usage: %s %s %s <BASE16-DECODED-STR> , <BASE16-DECODED-STR> length should be an even number.\n", argv[0], argv[1], argv[2]);
                return AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID;
            }

            size_t        outputSizeInBytes = inputSizeInBytes >> 1;
            unsigned char outputBuff[outputSizeInBytes];

            int ret = base16_decode(outputBuff, argv[3], inputSizeInBytes);

            if (ret == AUTOTOOLS_SETUP_OK) {
                if (fwrite(outputBuff, 1, outputSizeInBytes, stdout) != outputSizeInBytes || ferror(stdout)) {
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (isatty(STDOUT_FILENO)) {
                    printf("\n");
                }

                return AUTOTOOLS_SETUP_OK;
            } else if (ret == AUTOTOOLS_SETUP_ERROR_ARG_IS_INVALID) {
                fprintf(stderr, "%s is invalid base64-encoded string.\n", argv[2]);
            } else if (ret == AUTOTOOLS_SETUP_ERROR) {
                fprintf(stderr, "occurs error.\n");
            }

            return ret;
        }

        if (strcmp(argv[2], "base64-encode") == 0) {
            if (argv[3] == NULL) {
                unsigned char inputBuff[1023];
                unsigned int  inputSizeInBytes;

                for (;;) {
                    inputSizeInBytes = fread(inputBuff, 1, 1023, stdin);

                    if (ferror(stdin)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    unsigned int  x = (inputSizeInBytes % 3) == 0 ? 0 : 1;
                    unsigned int  outputSizeInBytes = (inputSizeInBytes / 3 + x) << 2;
                    unsigned char outputBuff[outputSizeInBytes];

                    int ret = EVP_EncodeBlock(outputBuff, inputBuff, inputSizeInBytes);

                    if (ret < 0) {
                        return ret;
                    }

                    if (fwrite(outputBuff, 1, outputSizeInBytes, stdout) != outputSizeInBytes || ferror(stdout)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    if (feof(stdin)) {
                        if (isatty(STDOUT_FILENO)) {
                            printf("\n");
                        }

                        return AUTOTOOLS_SETUP_OK;
                    }
                }
            } else {
                unsigned char * inputBuff = (unsigned char *)argv[3];
                unsigned int    inputSizeInBytes = strlen(argv[3]);

                if (inputSizeInBytes == 0) {
                    fprintf(stderr, "Usage: %s %s %s <STR> , <STR> should be non-empty.\n", argv[0], argv[1], argv[2]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                }

                unsigned int  x = (inputSizeInBytes % 3) == 0 ? 0 : 1;
                unsigned int  outputSizeInBytes = (inputSizeInBytes / 3 + x) << 2;
                unsigned char outputBuff[outputSizeInBytes];

                int ret = EVP_EncodeBlock(outputBuff, inputBuff, inputSizeInBytes);

                if (ret < 0) {
                    return ret;
                }

                if (fwrite(outputBuff, 1, outputSizeInBytes, stdout) != outputSizeInBytes || ferror(stdout)) {
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (isatty(STDOUT_FILENO)) {
                    printf("\n");
                }

                return AUTOTOOLS_SETUP_OK;
            }
        }

        if (strcmp(argv[2], "base64-decode") == 0) {
            if (argv[3] == NULL) {
                unsigned char inputBuff[1024];
                unsigned int  inputSizeInBytes;

                for (;;) {
                    inputSizeInBytes = fread(inputBuff, 1, 1024, stdin);

                    if (ferror(stdin)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    unsigned int  outputSizeInBytes = (inputSizeInBytes >> 2) * 3;
                    unsigned char outputBuff[outputSizeInBytes];

                    int ret = EVP_DecodeBlock(outputBuff, inputBuff, inputSizeInBytes);

                    if (ret < 0) {
                        return ret;
                    }

                    if (fwrite(outputBuff, 1, ret, stdout) != (size_t)ret || ferror(stdout)) {
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    if (feof(stdin)) {
                        if (isatty(STDOUT_FILENO)) {
                            printf("\n");
                        }

                        return AUTOTOOLS_SETUP_OK;
                    }
                }
            } else {
                unsigned char * inputBuff = (unsigned char *)argv[3];
                unsigned int    inputSizeInBytes = strlen(argv[3]);

                if (inputSizeInBytes == 0) {
                    fprintf(stderr, "Usage: %s %s %s <BASE64-DECODED-STR> , <BASE64-DECODED-STR> should be non-empty.\n", argv[0], argv[1], argv[2]);
                    return AUTOTOOLS_SETUP_ERROR_ARG_IS_NULL;
                }

                unsigned int  outputSizeInBytes = (inputSizeInBytes >> 2) * 3;
                unsigned char outputBuff[outputSizeInBytes];

                int ret = EVP_DecodeBlock(outputBuff, inputBuff, inputSizeInBytes);

                if (ret < 0) {
                    return ret;
                }

                if (fwrite(outputBuff, 1, ret, stdout) != (size_t)ret || ferror(stdout)) {
                    return AUTOTOOLS_SETUP_ERROR;
                }

                if (isatty(STDOUT_FILENO)) {
                    printf("\n");
                }

                return AUTOTOOLS_SETUP_OK;
            }
        }

        if (strcmp(argv[2], "sha256sum") == 0) {
            if (argv[3] == NULL || strcmp(argv[3], "-") == 0) {
                char outputBuff[65];
                outputBuff[64] = '\0';

                int ret = sha256sum_of_stream(outputBuff, stdin);

                if (ret != AUTOTOOLS_SETUP_OK) {
                    return ret;
                }

                printf("%s\n", outputBuff);
                return AUTOTOOLS_SETUP_OK;
            } else if (strcmp(argv[3], "-h") == 0 || strcmp(argv[3], "--help") == 0) {
                fprintf(stderr, "Usage: %s %s %s [FILEPATH]\n", argv[0], argv[1], argv[2]);
                return AUTOTOOLS_SETUP_OK;
            } else {
                char outputBuff[65];
                outputBuff[64] = '\0';

                int ret = sha256sum_of_file(outputBuff, argv[3]);

                if (ret != AUTOTOOLS_SETUP_OK) {
                    return ret;
                }

                printf("%s\n", outputBuff);
                return AUTOTOOLS_SETUP_OK;
            }
        }

        if (strcmp(argv[2], "zlib-deflate") == 0) {
            int level = 1;

            for (int i = 3; i < argc; i++) {
                if (strcmp(argv[i], "-L") == 0) {
                    char * p = argv[i + 1];

                    if (p == NULL) {
                        fprintf(stderr, "Usage: %s %s %s [-L N] (N>=1 && N <=9) , The smaller the N, the faster the speed and the lower the compression ratio.\n", argv[0], argv[1], argv[2]);
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    if (strlen(p) != 1) {
                        fprintf(stderr, "Usage: %s %s %s [-L N] (N>=1 && N <=9) , The smaller the N, the faster the speed and the lower the compression ratio.\n", argv[0], argv[1], argv[2]);
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    if (p[0] < '1' || p[0] > '9') {
                        fprintf(stderr, "Usage: %s %s %s [-L N] (N>=1 && N <=9) , The smaller the N, the faster the speed and the lower the compression ratio.\n", argv[0], argv[1], argv[2]);
                        return AUTOTOOLS_SETUP_ERROR;
                    }

                    level = atoi(p);

                    i++;
                } else {
                    fprintf(stderr, "unrecognized option: %s", argv[i]);
                    fprintf(stderr, "Usage: %s %s %s [-L N] (N>=1 && N <=9) , The smaller the N, the faster the speed and the lower the compression ratio.\n", argv[0], argv[1], argv[2]);
                    return AUTOTOOLS_SETUP_ERROR;
                }
            }

            return zlib_deflate_file_to_file(stdin, stdout, level);
        }

        if (strcmp(argv[2], "zlib-inflate") == 0) {
            return zlib_inflate_file_to_file(stdin, stdout);
        }

        if (strcmp(argv[2], "which") == 0) {
            if (argv[3] == NULL) {
                fprintf(stderr, "USAGE: %s %s %s <COMMAND-NAME> [-a]\n", argv[0], argv[1], argv[2]);
                return 1;
            }

            bool findAll = false;

            for (int i = 4; i < argc; i++) {
                if (strcmp(argv[i], "-a") == 0) {
                    findAll = true;
                } else {
                    fprintf(stderr, "unrecognized argument: %s\n", argv[i]);
                    fprintf(stderr, "USAGE: %s %s %s <COMMAND-NAME> [-a]\n", argv[0], argv[1], argv[2]);
                    return 1;
                }
            }

            char ** pathList = NULL;
            size_t  pathListSize;

            int ret = exe_search(argv[3], &pathList, &pathListSize, findAll);

            if (ret == 0) {
                for (size_t i = 0; i < pathListSize; i++) {
                    printf("%s\n", pathList[i]);

                    free(pathList[i]);
                    pathList[i] = NULL;
                }

                free(pathList);
                pathList = NULL;
            }

            return ret;
        }

        LOG_ERROR2("unrecognized command: ", argv[2]);
        return AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN;
    }


    LOG_ERROR2("unrecognized action: ", argv[1]);
    return AUTOTOOLS_SETUP_ERROR_ARG_IS_UNKNOWN;
}

int main(int argc, char* argv[]) {
    return autotools_setup_main(argc, argv);
}

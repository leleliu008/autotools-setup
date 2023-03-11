#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <yaml.h>

#include "autotools-setup.h"
#include "core/regex/regex.h"

typedef enum {
    FORMULA_KEY_CODE_unknown,

    FORMULA_KEY_CODE_src_url_autoconf,
    FORMULA_KEY_CODE_src_sha_autoconf,

    FORMULA_KEY_CODE_src_url_automake,
    FORMULA_KEY_CODE_src_sha_automake,

    FORMULA_KEY_CODE_src_url_libtool,
    FORMULA_KEY_CODE_src_sha_libtool,

    FORMULA_KEY_CODE_src_url_pkgconf,
    FORMULA_KEY_CODE_src_sha_pkgconf,

    FORMULA_KEY_CODE_src_url_perl,
    FORMULA_KEY_CODE_src_sha_perl,

    FORMULA_KEY_CODE_src_url_gm4,
    FORMULA_KEY_CODE_src_sha_gm4,
} AUTOTOOLSSETUPFormulaKeyCode;

void autotools_setup_config_dump(AUTOTOOLSSETUPFormula * config) {
    if (config == NULL) {
        return;
    }

    printf("src-url-autoconf: %s\n", config->src_url_autoconf);
    printf("src-sha-autoconf: %s\n", config->src_sha_autoconf);

    printf("src-url-automake: %s\n", config->src_url_automake);
    printf("src-sha-automake: %s\n", config->src_sha_automake);

    printf("src-url-libtool:  %s\n", config->src_url_libtool);
    printf("src-sha-libtool:  %s\n", config->src_sha_libtool);

    printf("src-url-pkgconf:  %s\n", config->src_url_pkgconf);
    printf("src-sha-pkgconf:  %s\n", config->src_sha_pkgconf);

    printf("src-url-perl:     %s\n", config->src_url_perl);
    printf("src-sha-perl:     %s\n", config->src_sha_perl);

    printf("src-url-gm4:      %s\n", config->src_url_gm4);
    printf("src-sha-gm4:      %s\n", config->src_sha_gm4);
}

void autotools_setup_config_free(AUTOTOOLSSETUPFormula * config) {
    if (config == NULL) {
        return;
    }

    ///////////////////////////////

    if (config->src_url_perl != NULL) {
        free(config->src_url_perl);
        config->src_url_perl = NULL;
    }

    if (config->src_sha_perl != NULL) {
        free(config->src_sha_perl);
        config->src_sha_perl = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_gm4 != NULL) {
        free(config->src_url_gm4);
        config->src_url_gm4 = NULL;
    }

    if (config->src_sha_gm4 != NULL) {
        free(config->src_sha_gm4);
        config->src_sha_gm4 = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_pkgconf != NULL) {
        free(config->src_url_pkgconf);
        config->src_url_pkgconf = NULL;
    }

    if (config->src_sha_pkgconf != NULL) {
        free(config->src_sha_pkgconf);
        config->src_sha_pkgconf = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_libtool != NULL) {
        free(config->src_url_libtool);
        config->src_url_libtool = NULL;
    }

    if (config->src_sha_libtool != NULL) {
        free(config->src_sha_libtool);
        config->src_sha_libtool = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_automake != NULL) {
        free(config->src_url_automake);
        config->src_url_automake = NULL;
    }

    if (config->src_sha_automake != NULL) {
        free(config->src_sha_automake);
        config->src_sha_automake = NULL;
    }

    //////////////////////////////////////

    if (config->src_url_autoconf != NULL) {
        free(config->src_url_autoconf);
        config->src_url_autoconf = NULL;
    }

    if (config->src_sha_autoconf != NULL) {
        free(config->src_sha_autoconf);
        config->src_sha_autoconf = NULL;
    }

    free(config);
}

static AUTOTOOLSSETUPFormulaKeyCode autotools_setup_config_key_code_from_key_name(char * key) {
           if (strcmp(key, "src-url-perl") == 0) {
        return FORMULA_KEY_CODE_src_url_perl;
    } else if (strcmp(key, "src-sha-perl") == 0) {
        return FORMULA_KEY_CODE_src_sha_perl;
    } else if (strcmp(key, "src-url-gm4") == 0) {
        return FORMULA_KEY_CODE_src_url_gm4;
    } else if (strcmp(key, "src-sha-gm4") == 0) {
        return FORMULA_KEY_CODE_src_sha_gm4;
    } else if (strcmp(key, "src-url-pkgconf") == 0) {
        return FORMULA_KEY_CODE_src_url_pkgconf;
    } else if (strcmp(key, "src-sha-pkgconf") == 0) {
        return FORMULA_KEY_CODE_src_sha_pkgconf;
    } else if (strcmp(key, "src-url-libtool") == 0) {
        return FORMULA_KEY_CODE_src_url_libtool;
    } else if (strcmp(key, "src-sha-libtool") == 0) {
        return FORMULA_KEY_CODE_src_sha_libtool;
    } else if (strcmp(key, "src-url-automake") == 0) {
        return FORMULA_KEY_CODE_src_url_automake;
    } else if (strcmp(key, "src-sha-automake") == 0) {
        return FORMULA_KEY_CODE_src_sha_automake;
    } else if (strcmp(key, "src-url-autoconf") == 0) {
        return FORMULA_KEY_CODE_src_url_autoconf;
    } else if (strcmp(key, "src-sha-autoconf") == 0) {
        return FORMULA_KEY_CODE_src_sha_autoconf;
    } else {
        return FORMULA_KEY_CODE_unknown;
    }
}

static void autotools_setup_config_set_value(AUTOTOOLSSETUPFormulaKeyCode keyCode, char * value, AUTOTOOLSSETUPFormula * config) {
    if (keyCode == FORMULA_KEY_CODE_unknown) {
        return;
    }

    char c;

    for (;;) {
        c = value[0];

        if (c == '\0') {
            return;
        }

        // non-printable ASCII characters and space
        if (c <= 32) {
            value = &value[1];
        } else {
            break;
        }
    }

    switch (keyCode) {
        case FORMULA_KEY_CODE_src_url_perl: if (config->src_url_perl != NULL) free(config->src_url_perl); config->src_url_perl = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_perl: if (config->src_sha_perl != NULL) free(config->src_sha_perl); config->src_sha_perl = strdup(value); break;

        case FORMULA_KEY_CODE_src_url_gm4: if (config->src_url_gm4 != NULL) free(config->src_url_gm4); config->src_url_gm4 = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_gm4: if (config->src_sha_gm4 != NULL) free(config->src_sha_gm4); config->src_sha_gm4 = strdup(value); break;

        case FORMULA_KEY_CODE_src_url_pkgconf: if (config->src_url_pkgconf != NULL) free(config->src_url_pkgconf); config->src_url_pkgconf = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_pkgconf: if (config->src_sha_pkgconf != NULL) free(config->src_sha_pkgconf); config->src_sha_pkgconf = strdup(value); break;

        case FORMULA_KEY_CODE_src_url_libtool: if (config->src_url_libtool != NULL) free(config->src_url_libtool); config->src_url_libtool = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_libtool: if (config->src_sha_libtool != NULL) free(config->src_sha_libtool); config->src_sha_libtool = strdup(value); break;

        case FORMULA_KEY_CODE_src_url_automake: if (config->src_url_automake != NULL) free(config->src_url_automake); config->src_url_automake = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_automake: if (config->src_sha_automake != NULL) free(config->src_sha_automake); config->src_sha_automake = strdup(value); break;

        case FORMULA_KEY_CODE_src_url_autoconf: if (config->src_url_autoconf != NULL) free(config->src_url_autoconf); config->src_url_autoconf = strdup(value); break;
        case FORMULA_KEY_CODE_src_sha_autoconf: if (config->src_sha_autoconf != NULL) free(config->src_sha_autoconf); config->src_sha_autoconf = strdup(value); break;
        case FORMULA_KEY_CODE_unknown: break;
    } 
}

static int autotools_setup_config_check(AUTOTOOLSSETUPFormula * config) {
    if (config->src_url_gm4 == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_GM4);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_gm4 = p;
    }

    if (config->src_sha_gm4 == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_GM4);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_gm4 = p;
    }

    ////////////////////////////////////////////////////////

    if (config->src_url_pkgconf == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_PKGCONF);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_pkgconf = p;
    }

    if (config->src_sha_pkgconf == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_PKGCONF);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_pkgconf = p;
    }

    ////////////////////////////////////////////////////////

    if (config->src_url_libtool == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_LIBTOOL);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_libtool = p;
    }

    if (config->src_sha_libtool == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_LIBTOOL);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_libtool = p;
    }

    ////////////////////////////////////////////////////////

    if (config->src_url_automake == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_AUTOMAKE);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_automake = p;
    }

    if (config->src_sha_automake == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_AUTOMAKE);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_automake = p;
    }

    ////////////////////////////////////////////////////////

    if (config->src_url_autoconf == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_AUTOCONF);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_autoconf = p;
    }

    if (config->src_sha_autoconf == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_AUTOCONF);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_autoconf = p;
    }

    ////////////////////////////////////////////////////////

    if (config->src_url_perl == NULL) {
        char * p = strdup(DEFAULT_SRC_URL_PERL);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_url_perl = p;
    }

    if (config->src_sha_perl == NULL) {
        char * p = strdup(DEFAULT_SRC_SHA_PERL);

        if (p == NULL) {
            autotools_setup_config_free(config);
            return AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
        }

        config->src_sha_perl = p;
    }
}

int autotools_setup_config_parse(const char * configFilePath, AUTOTOOLSSETUPFormula * * out) {
    FILE * file = fopen(configFilePath, "r");

    if (file == NULL) {
        perror(configFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    yaml_parser_t parser;
    yaml_token_t  token;

    // https://libyaml.docsforge.com/master/api/yaml_parser_initialize/
    if (yaml_parser_initialize(&parser) == 0) {
        perror("Failed to initialize yaml parser");
        return AUTOTOOLS_SETUP_ERROR;
    }

    yaml_parser_set_input_file(&parser, file);

    AUTOTOOLSSETUPFormulaKeyCode configKeyCode = FORMULA_KEY_CODE_unknown;

    AUTOTOOLSSETUPFormula * config = NULL;

    int lastTokenType = 0;

    int ret = AUTOTOOLS_SETUP_OK;

    do {
        // https://libyaml.docsforge.com/master/api/yaml_parser_scan/
        if (yaml_parser_scan(&parser, &token) == 0) {
            fprintf(stderr, "syntax error in config file: %s\n", configFilePath);
            ret = AUTOTOOLS_SETUP_ERROR_FORMULA_SYNTAX;
            goto finalize;
        }

        switch(token.type) {
            case YAML_KEY_TOKEN:
                lastTokenType = 1;
                break;
            case YAML_VALUE_TOKEN:
                lastTokenType = 2;
                break;
            case YAML_SCALAR_TOKEN:
                if (lastTokenType == 1) {
                    configKeyCode = autotools_setup_config_key_code_from_key_name((char*)token.data.scalar.value);
                } else if (lastTokenType == 2) {
                    if (config == NULL) {
                        config = (AUTOTOOLSSETUPFormula*)calloc(1, sizeof(AUTOTOOLSSETUPFormula));

                        if (config == NULL) {
                            ret = AUTOTOOLS_SETUP_ERROR_MEMORY_ALLOCATE;
                            goto finalize;
                        }
                    }

                    autotools_setup_config_set_value(configKeyCode, (char*)token.data.scalar.value, config);
                }
                break;
            default: 
                lastTokenType = 0;
                break;
        }

        if (token.type != YAML_STREAM_END_TOKEN) {
            yaml_token_delete(&token);
        }
    } while(token.type != YAML_STREAM_END_TOKEN);

finalize:
    yaml_token_delete(&token);

    yaml_parser_delete(&parser);

    fclose(file);

    if (ret == AUTOTOOLS_SETUP_OK) {
        ret = autotools_setup_config_check(config);

        if (ret == AUTOTOOLS_SETUP_OK) {
            (*out) = config;
            return AUTOTOOLS_SETUP_OK;
        }
    }

    autotools_setup_config_free(config);
    return ret;
}

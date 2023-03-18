#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <libgen.h>
#include <time.h>
#include <yaml.h>

#include "autotools-setup.h"

typedef enum {
    AutotoolsSetupConfigKeyCode_unknown,

    AutotoolsSetupConfigKeyCode_src_url_automake,
    AutotoolsSetupConfigKeyCode_src_sha_automake,

    AutotoolsSetupConfigKeyCode_src_url_autoconf,
    AutotoolsSetupConfigKeyCode_src_sha_autoconf,

    AutotoolsSetupConfigKeyCode_src_url_libtool,
    AutotoolsSetupConfigKeyCode_src_sha_libtool,

    AutotoolsSetupConfigKeyCode_src_url_pkgconf,
    AutotoolsSetupConfigKeyCode_src_sha_pkgconf,

    AutotoolsSetupConfigKeyCode_src_url_perl,
    AutotoolsSetupConfigKeyCode_src_sha_perl,

    AutotoolsSetupConfigKeyCode_src_url_gm4,
    AutotoolsSetupConfigKeyCode_src_sha_gm4,
} AutotoolsSetupConfigKeyCode;

void autotools_setup_config_dump(AutotoolsSetupConfig * config) {
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

void autotools_setup_config_free(AutotoolsSetupConfig * config) {
    if (config == NULL) {
        return;
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

    if (config->src_url_pkgconf != NULL) {
        free(config->src_url_pkgconf);
        config->src_url_pkgconf = NULL;
    }

    if (config->src_sha_pkgconf != NULL) {
        free(config->src_sha_pkgconf);
        config->src_sha_pkgconf = NULL;
    }

    //////////////////////////////////////

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

    free(config);
}

static AutotoolsSetupConfigKeyCode autotools_setup_config_key_code_from_key_name(char * key) {
           if (strcmp(key, "src-url-perl") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_perl;
    } else if (strcmp(key, "src-sha-perl") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_perl;
    } else if (strcmp(key, "src-url-gm4") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_gm4;
    } else if (strcmp(key, "src-sha-gm4") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_gm4;
    } else if (strcmp(key, "src-url-pkgconf") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_pkgconf;
    } else if (strcmp(key, "src-sha-pkgconf") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_pkgconf;
    } else if (strcmp(key, "src-url-libtool") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_libtool;
    } else if (strcmp(key, "src-sha-libtool") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_libtool;
    } else if (strcmp(key, "src-url-automake") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_automake;
    } else if (strcmp(key, "src-sha-automake") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_automake;
    } else if (strcmp(key, "src-url-autoconf") == 0) {
        return AutotoolsSetupConfigKeyCode_src_url_autoconf;
    } else if (strcmp(key, "src-sha-autoconf") == 0) {
        return AutotoolsSetupConfigKeyCode_src_sha_autoconf;
    } else {
        return AutotoolsSetupConfigKeyCode_unknown;
    }
}

static void autotools_setup_config_set_value(AutotoolsSetupConfigKeyCode keyCode, char * value, AutotoolsSetupConfig * config) {
    if (keyCode == AutotoolsSetupConfigKeyCode_unknown) {
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
        case AutotoolsSetupConfigKeyCode_src_url_perl: if (config->src_url_perl != NULL) free(config->src_url_perl); config->src_url_perl = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_perl: if (config->src_sha_perl != NULL) free(config->src_sha_perl); config->src_sha_perl = strdup(value); break;

        case AutotoolsSetupConfigKeyCode_src_url_gm4: if (config->src_url_gm4 != NULL) free(config->src_url_gm4); config->src_url_gm4 = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_gm4: if (config->src_sha_gm4 != NULL) free(config->src_sha_gm4); config->src_sha_gm4 = strdup(value); break;

        case AutotoolsSetupConfigKeyCode_src_url_pkgconf: if (config->src_url_pkgconf != NULL) free(config->src_url_pkgconf); config->src_url_pkgconf = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_pkgconf: if (config->src_sha_pkgconf != NULL) free(config->src_sha_pkgconf); config->src_sha_pkgconf = strdup(value); break;

        case AutotoolsSetupConfigKeyCode_src_url_libtool: if (config->src_url_libtool != NULL) free(config->src_url_libtool); config->src_url_libtool = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_libtool: if (config->src_sha_libtool != NULL) free(config->src_sha_libtool); config->src_sha_libtool = strdup(value); break;

        case AutotoolsSetupConfigKeyCode_src_url_automake: if (config->src_url_automake != NULL) free(config->src_url_automake); config->src_url_automake = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_automake: if (config->src_sha_automake != NULL) free(config->src_sha_automake); config->src_sha_automake = strdup(value); break;

        case AutotoolsSetupConfigKeyCode_src_url_autoconf: if (config->src_url_autoconf != NULL) free(config->src_url_autoconf); config->src_url_autoconf = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_src_sha_autoconf: if (config->src_sha_autoconf != NULL) free(config->src_sha_autoconf); config->src_sha_autoconf = strdup(value); break;
        case AutotoolsSetupConfigKeyCode_unknown: break;
    } 
}

int autotools_setup_config_parse(const char * configFilePath, AutotoolsSetupConfig * * out) {
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
        fclose(file);
        return AUTOTOOLS_SETUP_ERROR;
    }

    yaml_parser_set_input_file(&parser, file);

    AutotoolsSetupConfigKeyCode configKeyCode = AutotoolsSetupConfigKeyCode_unknown;

    AutotoolsSetupConfig * config = NULL;

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
                        config = (AutotoolsSetupConfig*)calloc(1, sizeof(AutotoolsSetupConfig));

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
        //autotools_setup_config_dump(config);
        (*out) = config;
    } else {
        autotools_setup_config_free(config);
    }

    return ret;
}

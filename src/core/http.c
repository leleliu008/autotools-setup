#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <stdbool.h>
#include <curl/curl.h>
#include <curl/curlver.h>

#include "http.h"

static size_t write_callback(void * ptr, size_t size, size_t nmemb, void * stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int http_fetch_to_stream(const char * url, FILE * outputFile, bool verbose, bool showProgress) {
    if (outputFile == NULL) {
        size_t  urlCopyLength = strlen(url) + 1;
        char    urlCopy[urlCopyLength];
        strncpy(urlCopy, url, urlCopyLength);

        const char * filename = basename(urlCopy);

        outputFile = fopen(filename, "wb");

        if (outputFile == NULL) {
            perror(filename);
            return AUTOTOOLS_SETUP_ERROR;
        }
    }

    curl_global_init(CURL_GLOBAL_ALL);

    CURL * curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, outputFile);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // https://curl.se/libcurl/c/CURLOPT_VERBOSE.html
    if (verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    } else {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
    }

    // https://curl.se/libcurl/c/CURLOPT_NOPROGRESS.html
    if (showProgress) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    } else {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    }

    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // https://www.openssl.org/docs/man1.1.1/man3/SSL_CTX_set_default_verify_paths.html
    const char * SSL_CERT_FILE = getenv("SSL_CERT_FILE");

    if (SSL_CERT_FILE != NULL) {
        // https://curl.se/libcurl/c/CURLOPT_CAINFO.html
        curl_easy_setopt(curl, CURLOPT_CAINFO, SSL_CERT_FILE);
    }

    const char * SSL_CERT_DIR = getenv("SSL_CERT_DIR");

    if (SSL_CERT_DIR != NULL) {
        // https://curl.se/libcurl/c/CURLOPT_CAPATH.html
        curl_easy_setopt(curl, CURLOPT_CAPATH, SSL_CERT_DIR);
    }

    char    userAgent[50];
    snprintf(userAgent, 50, "User-Agent: libcurl-%s", LIBCURL_VERSION);

    struct curl_slist *list = NULL;

    //list = curl_slist_append(list, "Accept: *");
    list = curl_slist_append(list, userAgent);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    CURLcode curlcode = curl_easy_perform(curl);
    
    // https://curl.se/libcurl/c/libcurl-errors.html
    if (curlcode != CURLE_OK) {
        fprintf(stderr, "%s\n", curl_easy_strerror(curlcode));
    }

    curl_slist_free_all(list);

    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return curlcode == CURLE_OK ? AUTOTOOLS_SETUP_OK : AUTOTOOLS_SETUP_ERROR_NETWORK_BASE + curlcode;
}

int http_fetch_to_file(const char * url, const char * outputFilePath, bool verbose, bool showProgress) {
    FILE * file = fopen(outputFilePath, "wb");

    if (file == NULL) {
        perror(outputFilePath);
        return AUTOTOOLS_SETUP_ERROR;
    }

    int ret = http_fetch_to_stream(url, file, verbose, showProgress);

    fclose(file);

    return ret;
}

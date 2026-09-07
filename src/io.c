#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>

    #include <io.h>
    #define F_OK 0
    #define access _access
#endif
#ifndef _WIN32
    #include <unistd.h>
#endif

#define DIR_PATH "./data"
#define SUCCESS 1
#define FAIL 0

/**
 * Appends the given file name to the path of the data directory.
 * Allocates memory for the file path and returns the address.
 */
char *getFilePath(const char *fileName) {
    size_t filePathLen = strlen(DIR_PATH) + strlen(fileName) + 2;
    char *filePath = (char *)malloc(filePathLen);
    if (!filePath) {
        return NULL;
    }

    snprintf(filePath, filePathLen, "%s/%s", DIR_PATH, fileName);

    return filePath;
}

int mk_exists(const char *fileName) {
    int exists = 0;
    char *filePath = getFilePath(fileName);

    if (access(filePath, F_OK) == 0) {
        exists = 1;
    }

    free(filePath);
    return exists;
}

/**
 * Reads bytes from the start of a file.
 * File is assumed to be in the system's data directory.
 */
int mk_read(const char *fileName, void *data, size_t dataLen) {
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return FAIL;
    }

    FILE *file = fopen(filePath, "r");
    if (!file) {
        free(filePath);
        return FAIL;
    }

    fread(data, sizeof(char), dataLen, file);

    fclose(file);
    free(filePath);
    return SUCCESS;
}

/**
 * Writes bytes from the start of a new file. Can overwrite another file.
 * File is assumed to be in the system's data directory.
 */
int mk_write(void *data, size_t dataLen, const char *fileName) {
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return FAIL;
    }

    FILE *file = fopen(filePath, "w");
    if (!file) {
        free(filePath);
        return FAIL;
    }

    fwrite(data, sizeof(char), dataLen, file);

    fclose(file);
    free(filePath);
    return SUCCESS;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef WIN32
    #include <io.h>
    #define F_OK 0
    #define access _access
#endif

#define DIR_PATH "./data"
#define SUCCESS 1
#define FAIL 0

char *getFilePath(const char *fileName) {
    int filePathLen = strlen(DIR_PATH) + strlen(fileName) + 2;
    char *filePath = (char *)malloc(filePathLen);
    snprintf(filePath, filePathLen, "%s/%s", DIR_PATH, fileName);
    return filePath;
}

int mk_read(const char *fileName, BYTE *data, ULONG dataLen) {
    // open file
    char *filePath = getFilePath(fileName);
    FILE *file = fopen(filePath, "r");
    if (!file) {
        // Error opening file
        free(filePath);
        return FAIL;
    }

    // read from file
    fread(data, sizeof(BYTE), dataLen, file);

    // close file
    fclose(file);
    free(filePath);
    return SUCCESS;
}

int mk_write(BYTE *data, ULONG dataLen, const char *fileName) {
    // open file
    char *filePath = getFilePath(fileName);
    FILE *file = fopen(filePath, "w");
    if (!file) {
        perror("Error opening file");
        free(filePath);
        return FAIL;
    }

    // write to file
    fwrite(data, sizeof(BYTE), dataLen, file);

    // close file
    fclose(file);
    free(filePath);
    return SUCCESS;
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
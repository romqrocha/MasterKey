#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define DIR_PATH "./data"

char *getFilePath(const char *fileName) {
    int filePathLen = strlen(DIR_PATH) + strlen(fileName) + 2;
    char *filePath = (char *)malloc(filePathLen);
    snprintf(filePath, filePathLen, "%s/%s", DIR_PATH, fileName);
    return filePath;
}

int read(const char *fileName, BYTE *data, ULONG dataLen) {
    // open file
    char *filePath = getFilePath(fileName);
    FILE *file = fopen(filePath, "r");
    if (!file) {
        // Error opening file
        free(filePath);
        return 0;
    }

    // read from file
    fread(data, sizeof(BYTE), dataLen, file);

    // close file
    fclose(file);
    free(filePath);
    return 1;
}

int write(BYTE *data, ULONG dataLen, const char *fileName) {
    // open file
    char *filePath = getFilePath(fileName);
    FILE *file = fopen(filePath, "w");
    if (!file) {
        perror("Error opening file");
        free(filePath);
        return 0;
    }

    // write to file
    fwrite(data, sizeof(BYTE), dataLen, file);

    // close file
    fclose(file);
    free(filePath);
    return 1;
}
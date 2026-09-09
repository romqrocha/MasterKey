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
    #include <dirent.h>
#endif

#define DIR_PATH "./data"
#define SUCCESS 1
#define FAIL 0

/**
 * Gets the number of bytes required to read a file.
 */
long getFileLength(FILE *fp) {
    fseek(fp, 0, SEEK_END); 
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    return length;
}

/**
 * Appends the given file name to the path of the data directory.
 * Allocates memory for the file path and returns the address.
 */
char *getFilePath(const char *fileName) {
    // remember to free later
    char *filePath = NULL;
    
    size_t filePathLen = strlen(DIR_PATH) + strlen(fileName) + 2;
    filePath = (char *)malloc(filePathLen);
    if (!filePath) {
        return NULL;
    }

    snprintf(filePath, filePathLen, "%s/%s", DIR_PATH, fileName);

    return filePath;
}

/**
 * Returns 1 if the file exists, 0 otherwise.
 */
int mk_exists(const char *fileName) {
    // remember to free at the end
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return FAIL;
    }

    int exists = 0;
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
    // remember to free at the end
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return FAIL;
    }

    FILE *file = fopen(filePath, "rb");
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
 * Reads the text contents of an entire file.
 * Writes the file length to fileLen.
 * Allocates memory for the contents and returns the address.
 */
void *mk_readAll(const char *fileName, long *fileLen) {
    // remember to free at the end
    void *content = NULL;
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return NULL;
    }

    FILE *file = fopen(filePath, "rb");
    if (!file) {
        free(filePath);
        return NULL;
    }

    *fileLen = getFileLength(file);
    content = malloc(*fileLen);
    if (!content) {
        return NULL;
    }

    fread(content, sizeof(char), *fileLen, file);

    fclose(file);
    free(filePath);
    return content;
}

/**
 * Writes bytes from the start of a new file. Can overwrite another file.
 * File is assumed to be in the system's data directory.
 */
int mk_write(void *data, size_t dataLen, const char *fileName) {
    // remember to free at the end
    char *filePath = getFilePath(fileName);
    if (!filePath) {
        return FAIL;
    }

    FILE *file = fopen(filePath, "wb");
    if (!file) {
        free(filePath);
        return FAIL;
    }

    fwrite(data, sizeof(char), dataLen, file);

    fclose(file);
    free(filePath);
    return SUCCESS;
}

/**
 * Finds the next file in the given directory.
 * Everything is relative to the system's data directory.
 * Allocates memory for the file name and returns the address.
 */
char *mk_nextFileInDir(const char *dirName, void **currFile) {
    // remember to free later
    char *fileName = NULL;
    char *dirPath = getFilePath(dirName);
    if (!dirPath) {
        return NULL;
    }; 

#ifdef _WIN32
    WIN32_FIND_DATA fileData;
    size_t fileNameLen = 0;
    int success = 0;

    if (!*currFile) {
        *currFile = FindFirstFile(dirPath, &fileData);
        success = *currFile != INVALID_HANDLE_VALUE;
    } else {
        success = FindNextFile(*currFile, &fileData);
    }

    // skip directories
    while (success && (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        success = FindNextFile(*currFile, &fileData);
    }

    if (!success) {
        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_FILES) {
            perror("Error at mk_nextFileInDir()\n");
        }
        FindClose(*currFile);
        fileName = NULL;
    } else {
        fileNameLen = strlen(fileData.cFileName) + 1; // +1 for '\0'
        fileName = malloc(fileNameLen);
        memcpy(fileName, fileData.cFileName, fileNameLen);
    }
#else
    // non-windows solution using dirent.h
#endif

    free(dirPath);
    return fileName;
}

/**
 * Counts the number of files in the given directory.
 * Everything is relative to the system's data directory.
 */
size_t mk_countFilesInDir(const char *dirName) {
    // remember to free later
    char *dirPath = getFilePath(dirName);
    if (!dirPath) {
        return 0;
    }

    size_t count = 0;
    
#ifdef _WIN32
    // remember to free later
    void *hFile = NULL;

    WIN32_FIND_DATA fileData;
    hFile = FindFirstFile(dirPath, &fileData);
    
    int success = hFile != INVALID_HANDLE_VALUE;
    while (success) {
        if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            count++;
        } 
        success = FindNextFile(hFile, &fileData);
    }

    if (!success) {
        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_FILES) {
            perror("Error at mk_countFilesInDir()\n");
            count = 0;
        }
    }

    FindClose(hFile);
#else
    // non-windows solution using dirent.h
#endif

    free(dirPath);
    return count;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

#define OK 0
#define ERR -1
#define ERR_NO_MEM -2
#define ERR_SYS_CALL -3
#define ERR_EARLY_EOF -10
#define ERR_STREAM -11
#define ERR_FINDNEXTFILE -12

/**
 * Appends the given file name to the path of the data directory.
 * Allocates memory for the file path and returns the address.
 */
char *getFilePath(const char *fileName) {
    // remember to free later
    char *filePath = NULL;
    
    size_t filePathLen = strlen(DIR_PATH) + strlen(fileName) + 2; // +2 for '/' and '\0'
    filePath = (char *)malloc(filePathLen);
    if (!filePath) {
        return NULL;
    }

    snprintf(filePath, filePathLen, "%s/%s", DIR_PATH, fileName);

    return filePath;
}

/**
 * Boilerplate remover
 */
FILE *openFile(const char *fileName, const char *mode, int *err) {
    char *filePath = NULL;
    FILE *file = NULL;
    
    filePath = getFilePath(fileName);
    if (!filePath) {
        *err = ERR_NO_MEM;
        goto cleanup;
    }

    do {
        file = fopen(filePath, mode);
    } while (!file && errno == EINTR);
    if (!file) {
        *err = ERR_SYS_CALL;
        goto cleanup;
    }

cleanup:
    free(filePath);
    return file;
}

/**
 * Gets the number of bytes required to read a file.
 * >= 0: Number of bytes to read the file
 * <  0: Error
 */
long mk_fileLength(const char *fileName) {
    long length = 0;
    int err = ERR;
    
    FILE *file = openFile(fileName, "rb", &err);
    if (!file) {
        goto cleanup;
    }

    fseek(file, 0, SEEK_END); 

    do {
        length = ftell(file);
    } while (length == ERR && errno == EINTR);

    err = OK;
cleanup:
    if (file) {
        fclose(file); // ! could error
    }
    return err ? (long)err : length;
}

/**
 * = 1: File exists
 * = 0: File doesn't exist
 * < 0: Error
 */
int mk_exists(const char *fileName) {
    int exists = 0;

    char *filePath = NULL;
    
    filePath = getFilePath(fileName);
    if (!filePath) {
        exists = ERR_NO_MEM;
        goto cleanup;
    }

    if (access(filePath, F_OK) == 0) {
        exists = 1;
    }

cleanup:
    free(filePath);
    return exists;
}

/**
 * Reads bytes from the start of a file.
 * File is assumed to be in the system's data directory.
 * = 0: OK
 * < 0: Error
 */
int mk_read(const char *fileName, void *data, size_t dataLen) {
    int err = ERR;

    FILE *file = openFile(fileName, "rb", &err);
    if (!file) {
        goto cleanup;
    }

    size_t bytesRead = fread(data, sizeof(char), dataLen, file);
    if (bytesRead != dataLen) {
        err = feof(file) ? ERR_EARLY_EOF : ERR_STREAM;
        goto cleanup;
    }

    err = OK;
cleanup:
    if (file) {
        fclose(file); // ! could error
    }
    return err;
}

/**
 * Writes bytes from the start of a new file. Can overwrite another file.
 * File is assumed to be in the system's data directory.
 * = 0: OK
 * < 0: Error
 */
int mk_write(const void *data, size_t dataLen, const char *fileName) {
    int err = ERR;

    FILE *file = openFile(fileName, "wb", &err);
    if (!file) {
        goto cleanup;
    }

    fwrite(data, sizeof(char), dataLen, file);

    err = OK;
cleanup:
    if (file) {
        fclose(file); // ! could error
    }
    return err;
}

/**
 * Finds the next file in the given directory.
 * Everything is relative to the system's data directory.
 * Allocates memory for the file name and returns the address.
 */
char *mk_nextFileInDir(const char *dirName, void **currFile, int *err) {
    // remember to free later
    char *fileName = NULL;
    char *dirPath = getFilePath(dirName);
    if (!dirPath) {
        *err = ERR_NO_MEM;
        goto cleanup;
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
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            *err = ERR_FINDNEXTFILE;
            goto cleanup;
        }
        FindClose(*currFile);
        fileName = NULL;
    } else {
        fileNameLen = strlen(fileData.cFileName) + 1; // +1 for '\0'
        fileName = malloc(fileNameLen);
        if (!fileName) {
            *err = ERR_NO_MEM;
            goto cleanup;
        }
        memcpy(fileName, fileData.cFileName, fileNameLen);
    }
#else
    // non-windows solution using dirent.h
#endif

cleanup:
    if (*err) {
        if (currFile && *currFile) {
#ifdef _WIN32
            FindClose(*currFile);
#endif
        }
        free(fileName);
        fileName = NULL;
    }
    free(dirPath);
    return fileName;
}

/**
 * Counts the number of files in the given directory.
 * Everything is relative to the system's data directory.
 * >= 0: Number of files in directory
 * <=-1: Error
 */
size_t mk_countFilesInDir(const char *dirName) {
    size_t count = 0;

    // remember to free later
    char *dirPath = getFilePath(dirName);
    if (!dirPath) {
        count = ERR_NO_MEM;
        goto cleanup;
    }
    
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
        if (GetLastError() != ERROR_NO_MORE_FILES) {
            count = ERR_FINDNEXTFILE;
            goto cleanup;
        }
    }
#else
    // non-windows solution using dirent.h
#endif

cleanup:
    if (hFile) {
#ifdef _WIN32
        FindClose(hFile);
#endif
    }
    free(dirPath);
    return count;
}

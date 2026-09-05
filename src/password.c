#include <stdlib.h>
#include <stdio.h>
#include "crypto.h"
#include "io.h"
#include "password.h"

#define SALT_LEN 8
#define KEY_LEN 64
#define ITERATIONS 15042


int setPassword(char *pw) {
    int success = 0;
    NTSTATUS status = NTE_FAIL;
    BYTE *saltAndKey = NULL;

    char trashCan[1] = "";
    int fileExists = mk_read(HASHED_PASSWORD_FILE, (BYTE *)trashCan, 0);
    if (fileExists) {
        printf("Error: tried setting password but one already exists.\n");
        return 0;
    }

    do {
        saltAndKey = (BYTE *)malloc(SALT_LEN + KEY_LEN);
        if (!saltAndKey) {
            status = NTE_NO_MEMORY;
            break;
        }

        status = genRandom(saltAndKey, SALT_LEN);
        if (status != NO_ERROR) {
            break;
        }

        status = deriveKey((BYTE *)pw, strlen(pw), saltAndKey, SALT_LEN, ITERATIONS, saltAndKey + SALT_LEN, KEY_LEN);
        if (status != NO_ERROR) {
            break;
        }

        success = mk_write(saltAndKey, SALT_LEN + KEY_LEN, HASHED_PASSWORD_FILE);
    } while (0);

    free(saltAndKey);
    return success;
}

int testPassword(char *pw) {
    int success = 0;
    NTSTATUS status = NTE_FAIL;
    BYTE *saltAndKey = NULL;
    BYTE *fileContents = NULL;

    do {
        fileContents = (BYTE *)malloc(SALT_LEN + KEY_LEN);
        if (!fileContents) {
            status = NTE_NO_MEMORY;
            break;
        }
        
        int readSuccessfully = mk_read(HASHED_PASSWORD_FILE, fileContents, SALT_LEN + KEY_LEN);
        if (!readSuccessfully) {
            printf("Unable to read %s. File may be missing.\n", HASHED_PASSWORD_FILE);
            break;
        }

        saltAndKey = (BYTE *)malloc(SALT_LEN + KEY_LEN);
        if (!saltAndKey) {
            status = NTE_NO_MEMORY;
            break;
        }
        memcpy(saltAndKey, fileContents, SALT_LEN);

        status = deriveKey((BYTE *)pw, strlen(pw), saltAndKey, SALT_LEN, ITERATIONS, saltAndKey + SALT_LEN, KEY_LEN);
        if (status != NO_ERROR) {
            break;
        }

        int isMatching = 1;
        for (int i = 0; i < SALT_LEN + KEY_LEN; i++) {
            if (fileContents[i] != saltAndKey[i]) {
                isMatching = 0;
                break;
            }
        }
        success = isMatching;
    } while (0);

    free(saltAndKey);
    return success;
}

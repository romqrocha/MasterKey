#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include "wincrypto.h" 
#else
    // it would be really nice if i had a cross platform solution...
#endif

#include "io.h"
#include "password.h"

#define SALT_LEN 8
#define KEY_LEN 64
#define ITERATIONS 50042
#define SUCCESS 1
#define FAIL 0

typedef unsigned char byte;

/**
 * Sets bytes at the pointer to 0.
 * Guaranteed to not be optimized away by compilers.
 */
void mk_clearMemory(void *pointer, size_t bytes) 
{
    #ifdef _WIN32
        SecureZeroMemory(pointer, bytes);
    #else
        //help
    #endif
}

/**
 * Sets the master password for the system.
 * Returns 0 for fail and 1 for success.
 */
int setPassword(char *pw) {
    // remember to free later
    byte *saltAndKey = (byte *)malloc(SALT_LEN + KEY_LEN);
    if (!saltAndKey) {
        return FAIL;
    }

    int success = FAIL;
    do {
        int fileExists = mk_exists(HASHED_PASSWORD_FILE);
        if (fileExists) {
            printf("Error: tried setting password but one already exists.\n");
            break;
        }

#ifdef _WIN32
        if (!genRandom(saltAndKey, SALT_LEN)) {
            break;
        }
        if (!deriveKey((byte *)pw, strlen(pw) + 1, saltAndKey, SALT_LEN, ITERATIONS, saltAndKey + SALT_LEN, KEY_LEN)) {
            break;
        }
#else
        // future non windows cryptography library...
#endif

        if (!mk_write(saltAndKey, SALT_LEN + KEY_LEN, HASHED_PASSWORD_FILE)) {
            break;
        }

        success = SUCCESS;
    } while (0);

    free(saltAndKey);
    return success;
}

/**
 * Tests the given password against the master password.
 * Returns 0 for fail and 1 for success.
 */
int testPassword(char *pw) {
    // remember to free later
    byte *saltAndKey = (byte *)malloc(SALT_LEN + KEY_LEN);
    byte *fileContents = (byte *)malloc(SALT_LEN + KEY_LEN);
    if (!saltAndKey && !fileContents) {
        perror("Out of memory in testPassword()\n");
        free(saltAndKey);
        free(fileContents);
        return FAIL;
    }

    int isMatching = 0;
    do {
        if (!mk_read(HASHED_PASSWORD_FILE, fileContents, SALT_LEN + KEY_LEN)) {
            printf("Unable to read %s. File may be missing.\n", HASHED_PASSWORD_FILE);
            break;
        }
        
        memcpy(saltAndKey, fileContents, SALT_LEN);

#ifdef _WIN32
        if (!deriveKey((byte *)pw, strlen(pw) + 1, saltAndKey, SALT_LEN, ITERATIONS, saltAndKey + SALT_LEN, KEY_LEN)) {
            break;
        }
#else
        // future non windows cryptography library...
#endif

        isMatching = 1;
        for (int i = 0; i < SALT_LEN + KEY_LEN; i++) {
            if (fileContents[i] != saltAndKey[i]) {
                isMatching = 0;
                break;
            }
        }
    } while (0);

    free(saltAndKey);
    free(fileContents);
    return isMatching;
}

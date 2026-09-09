#ifdef _WIN32 // This library relies on the Win32 API

#include <stdio.h>
#include <string.h>
#include <wchar.h>

// win32 dependent
#include <windows.h>
#include <bcrypt.h>

#include "wincrypto.h"
#include "io.h"
#include "password.h"

#define SALT_FILE "salt.bin"
#define SALT_LEN 8
#define KEY_LEN 64
#define NUM_ITERATIONS 20000
#define BREAK_IF_ERR(var) if (var != NO_ERROR) {break;}

/**
 * Fills the buffer with random bytes.
 */
int genRandom(BYTE *buffer, ULONG bufferLen)
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;

    do {
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0);
        BREAK_IF_ERR(status);

        status = BCryptGenRandom(hAlg, buffer, bufferLen, 0);
    } while (0);

    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (status != NO_ERROR) {
        printf("Error in genRandom() (%ld)", status);
    }
    return status == NO_ERROR;
}

/**
 * Gets the salt required by some of the functions in this library.
 * A new salt will be generated and stored if no salt file already exists; 
 * otherwise the stored salt will be retrieved.
 * Allocates memory and writes the address to <salt>.
 */
NTSTATUS getSalt(BYTE **salt, int forceNewSalt) 
{
    // remember to free later
    *salt = (BYTE *)malloc(SALT_LEN);
    if (!*salt) {
        return NTE_NO_MEMORY;
    }

    NTSTATUS status = NTE_FAIL;

    // first check if salt already exists
    if (!forceNewSalt) {
        int exists = mk_exists(SALT_FILE);
        forceNewSalt = !exists;
    }

    do {
        // if no salt file, make new salt
        if (forceNewSalt) {
            status = genRandom(*salt, SALT_LEN) ? NO_ERROR : NTE_FAIL;
            BREAK_IF_ERR(status);

            status = mk_write(*salt, SALT_LEN, SALT_FILE) ? NO_ERROR : NTE_FAIL;
        } else {
            status = mk_read(SALT_FILE, *salt, SALT_LEN) ? NO_ERROR : NTE_FAIL;
        }
    } while (0);

    if (status != NO_ERROR) {
        free(*salt);
    }
    return status;
}

/**
 * Finds the length (in bytes) of the initialization vector of the given algorithm.
 */
NTSTATUS getIvLen(BCRYPT_HANDLE hAlg, ULONG *ivLen) 
{
    ULONG _ = 0;
    return BCryptGetProperty(hAlg, BCRYPT_BLOCK_LENGTH, (PUCHAR)ivLen, sizeof(*ivLen), &_, 0);
}

/**
 * Generates a key from a password and a salt by using the PBKDF2 algorithm with SHA-256.
 * More iterations means it takes longer to generate but also to brute force.
 */
int deriveKey(BYTE *pw, ULONG pwLen, BYTE *salt, ULONG saltLen, 
    const ULONG iterations, BYTE *derivedKey, ULONG derivedKeyLen) 
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    
    do {
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
        BREAK_IF_ERR(status);

        status = BCryptDeriveKeyPBKDF2(hAlg, pw, pwLen, salt, saltLen, iterations, derivedKey, derivedKeyLen, 0);
    } while (0);

    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (status != NO_ERROR) {
        printf("Error in deriveKey() (%ld)\n", status);
    }
    return status == NO_ERROR;
}

/**
 * Encrypts data while handling the memory allocation.
 * Since the encryption uses a random IV, the IV is prepended to the ciphertext.
 * Allocates memory and writes the address to <encryptedData>.
 */
NTSTATUS doEncrypt(BYTE *key, ULONG keyLen, BYTE *plaintxt, ULONG plaintxtLen, 
    BYTE **encryptedData, ULONG *encryptedDataLen)
{
    // remember to free later
    BYTE *iv = NULL;
    BYTE *ivCopy = NULL;

    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    do {
        // set up the symmetric encryption algorithm and IV
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        BREAK_IF_ERR(status);
        
        const wchar_t mode[] = BCRYPT_CHAIN_MODE_CBC;
        ULONG modeLen = (ULONG)sizeof(mode);
        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)mode, modeLen, 0);
        BREAK_IF_ERR(status);

        ULONG ivLen = 0;
        status = getIvLen(hAlg, &ivLen);
        BREAK_IF_ERR(status);

        iv = (BYTE *)malloc(ivLen);
        ivCopy = (BYTE *)malloc(ivLen);
        status = (iv && ivCopy) ? NO_ERROR : NTE_NO_MEMORY; 
        BREAK_IF_ERR(status);

        status = genRandom(iv, ivLen) ? NO_ERROR : NTE_FAIL;
        BREAK_IF_ERR(status);
        memcpy(ivCopy, iv, ivLen);

        // get symmetric encryption key from our hashed/derived key
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, keyLen, 0);
        BREAK_IF_ERR(status);

        // find how big the ciphertext is
        ULONG ciphertxtLen = 0;
        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, NULL, 0, &ciphertxtLen, BCRYPT_BLOCK_PADDING);
        BREAK_IF_ERR(status);

        *encryptedDataLen = ivLen + ciphertxtLen;
        *encryptedData = (BYTE *)malloc(*encryptedDataLen);
        status = encryptedData ? NO_ERROR : NTE_NO_MEMORY;
        BREAK_IF_ERR(status);

        // prepend encryptedData with the IV
        memcpy(*encryptedData, iv, ivLen);
        BYTE *ciphertxt = *encryptedData + ivLen;

        // encrypt
        ULONG _ = 0;
        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, ciphertxt, ciphertxtLen, &_, BCRYPT_BLOCK_PADDING);
    } while (0);

    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    free(iv);
    free(ivCopy);
    return status;
}

/**
 * Decrypts data while handling the memory allocation.
 * Since the encryption used a random IV, that IV is acquired from the encrypted data.
 * Allocates memory and writes the address to <plaintxt>.
 */
NTSTATUS doDecrypt(BYTE *key, ULONG keyLen, BYTE *encryptedData, 
    ULONG encryptedDataLen, BYTE **plaintxt, ULONG *plaintxtLen)
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;

    // remember to free later
    BYTE *iv = NULL;

    do {
        // set up the symmetric key algorithm and IV
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        BREAK_IF_ERR(status);
        
        const wchar_t mode[] = BCRYPT_CHAIN_MODE_CBC;
        ULONG modeLen = (ULONG)sizeof(mode);
        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)mode, modeLen, 0);
        BREAK_IF_ERR(status);
        
        ULONG ivLen = 0;
        status = getIvLen(hAlg, &ivLen);
        BREAK_IF_ERR(status);

        iv = (BYTE *)malloc(ivLen);
        status = iv ? NO_ERROR : NTE_NO_MEMORY;
        BREAK_IF_ERR(status);
        memcpy(iv, encryptedData, ivLen);

        // get symmetric encryption key from our hashed/derived key
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, keyLen, 0);
        BREAK_IF_ERR(status);

        // find how big the plaintext is
        BYTE *ciphertxt = encryptedData + ivLen;
        ULONG ciphertxtLen = encryptedDataLen - ivLen;
        status = BCryptDecrypt(hKey, ciphertxt, ciphertxtLen, NULL, iv, ivLen, NULL, 0, plaintxtLen, BCRYPT_BLOCK_PADDING);
        BREAK_IF_ERR(status);

        *plaintxt = (BYTE *)malloc(*plaintxtLen);
        status = *plaintxt ? NO_ERROR : NTE_NO_MEMORY;
        BREAK_IF_ERR(status);
        
        // decrypt
        ULONG _ = 0;
        status = BCryptDecrypt(hKey, ciphertxt, ciphertxtLen, NULL, iv, ivLen, *plaintxt, *plaintxtLen, &_, BCRYPT_BLOCK_PADDING);
    } while (0);

    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    free(iv);
    return status;
}

/**
 * Encrypts data (plaintext) with a password.
 * Allocates memory and writes the address to <encryptedData>.
 */
NTSTATUS encrypt(BYTE *pw, ULONG pwLen, BYTE *plaintxt, ULONG plaintxtLen, 
    BYTE **encryptedData, ULONG *encryptedDataLen) 
{
    NTSTATUS status = NTE_FAIL;

    // remember to free
    BYTE *salt = NULL;
    BYTE *key = (BYTE *)malloc(KEY_LEN);
    if (key == NULL) {
        return NTE_NO_MEMORY;
    }

    do {
        status = getSalt(&salt, 0);
        BREAK_IF_ERR(status);

        status = deriveKey(pw, pwLen, salt, SALT_LEN, NUM_ITERATIONS, key, KEY_LEN) ? NO_ERROR : NTE_FAIL;
        BREAK_IF_ERR(status);

        status = doEncrypt(key, KEY_LEN, plaintxt, plaintxtLen, encryptedData, encryptedDataLen);
    } while (0);
    
    mk_clearMemory(key, KEY_LEN);
    free(key);
    free(salt);
    return status;
}

/**
 * Decrypts data (ciphertext) with a password.
 * Allocates memory and writes the address to <plaintxt>.
 */
NTSTATUS decrypt(BYTE *pw, ULONG pwLen, BYTE *ciphertxt, ULONG ciphertxtLen, 
    BYTE **plaintxt, ULONG *plaintxtLen) 
{
    NTSTATUS status = NTE_FAIL;

    // remember to free at the end
    BYTE *salt = NULL;
    BYTE *key = (BYTE *)malloc(KEY_LEN);
    if (key == NULL) {
        return NTE_NO_MEMORY;
    }

    do {
        status = getSalt(&salt, 0);
        BREAK_IF_ERR(status);

        status = deriveKey(pw, pwLen, salt, SALT_LEN, NUM_ITERATIONS, key, KEY_LEN) ? NO_ERROR : NTE_FAIL;
        BREAK_IF_ERR(status);

        status = doDecrypt(key, KEY_LEN, ciphertxt, ciphertxtLen, plaintxt, plaintxtLen);
    } while (0);

    mk_clearMemory(key, KEY_LEN);
    free(key);
    free(salt);
    return status;
}


/**
 * Encrypts text data with a text password.
 * Allocates memory and writes the address to <ciphertxt>.
 */
int encryptText(char *pw, char *text, BYTE **ciphertxt, ULONG *ciphertxtLen) {
    NTSTATUS status = encrypt((BYTE *)pw, (ULONG)(strlen(pw) + 1), 
        (BYTE *)text, (ULONG)(strlen(text) + 1), ciphertxt, ciphertxtLen
    );

    if (status != NO_ERROR) {
        printf("Error in encryptText() (%ld)\n", status);
    }
    return status == NO_ERROR;
}

/**
 * Decrypts text data with a text password.
 * Allocates memory and writes the address to <text>.
 */
int decryptText(char *pw, BYTE *ciphertxt, ULONG ciphertxtLen, char **text) {
    ULONG _ = 0;
    NTSTATUS status = decrypt((BYTE *) pw, (ULONG)(strlen(pw) + 1),
        ciphertxt, ciphertxtLen, (BYTE **)text, &_
    );

    int success = status == NO_ERROR;
    if (!success) {
        printf("Error in decryptText() (%ld)\n", status);
    }
    return success;
}

#endif
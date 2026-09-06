#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <wchar.h>

#include "crypto.h"
#include "io.h"

#ifdef _WIN32
    #include <bcrypt.h>
#endif

#define SALT_FILE "salt.bin"
#define SALT_LEN 8
#define KEY_LEN 64
#define NUM_ITERATIONS 20000

void mk_clearMemory(void *pointer, size_t bytes) {
    SecureZeroMemory(pointer, bytes);
}

NTSTATUS genRandom(BYTE *buffer, ULONG bufferLen) {
    BCRYPT_ALG_HANDLE hAlg;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, NULL, 0);
    if (status != NO_ERROR) {
        return status;
    }
    status = BCryptGenRandom(hAlg, buffer, bufferLen, 0);
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return status;
}

NTSTATUS getSalt(BYTE *salt, int forceNewSalt) {
    salt = (BYTE *)malloc(SALT_LEN);
    if (!salt) {
        return NTE_NO_MEMORY;
    }

    // first check if salt already exists
    if (!forceNewSalt) {
        int exists = mk_read(SALT_FILE, salt, SALT_LEN);
        printf("Reading salt file (Exists: %s)", exists ? "Y" : "N");
        forceNewSalt = exists;
    }

    if (forceNewSalt) {
        // if no salt file, make new salt
        NTSTATUS status = genRandom(salt, SALT_LEN);
        if (status != NO_ERROR) {
            free(salt);
            return status;
        }

        int success = mk_write(salt, SALT_LEN, SALT_FILE);
        printf("Writing salt file (Success: %s)", success ? "Y" : "N");
        if (!success) {
            free(salt);
            return NTE_FAIL;
        }
    }

    return NO_ERROR;
}

NTSTATUS getIvLen(BCRYPT_HANDLE hAlg, ULONG *ivLen) {
    ULONG _ = 0;
    return BCryptGetProperty(hAlg, BCRYPT_BLOCK_LENGTH, (PUCHAR)ivLen, sizeof(*ivLen), &_, 0);
}

NTSTATUS deriveKey(BYTE *pw, ULONG pwLen, BYTE *salt, ULONG saltLen, 
    const ULONG iterations, BYTE *derivedKey, ULONG derivedKeyLen) 
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    if (status == NO_ERROR) {
        status = BCryptDeriveKeyPBKDF2(hAlg, pw, pwLen, salt, saltLen, iterations, derivedKey, derivedKeyLen, 0);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return status;
}

NTSTATUS doEncrypt(BYTE *key, ULONG keyLen, BYTE *plaintxt, ULONG plaintxtLen, 
    BYTE **encryptedData, ULONG *encryptedDataLen)
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    BYTE *iv = NULL;
    BYTE *ivCopy = NULL;
    do {
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (status != NO_ERROR) {
            break;
        }
        
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, keyLen, 0);
        if (status != NO_ERROR) {
            break;
        }

        const wchar_t mode[] = BCRYPT_CHAIN_MODE_CBC;
        ULONG modeLen = (ULONG)sizeof(mode);
        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)mode, modeLen, 0);
        if (status != NO_ERROR) {
            break;
        }

        ULONG ivLen = 0;
        status = getIvLen(hAlg, &ivLen);
        if (status != NO_ERROR) {
            break;
        }

        iv = (BYTE *)malloc(ivLen);
        ivCopy = (BYTE *)malloc(ivLen);
        if (iv == NULL || ivCopy == NULL) {
            status = NTE_NO_MEMORY;
            break;
        }
        status = genRandom(iv, ivLen);
        if (status != NO_ERROR) {
            break;
        }
        memcpy(ivCopy, iv, ivLen);

        ULONG ciphertxtLen = 0;
        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, NULL, 0, &ciphertxtLen, BCRYPT_BLOCK_PADDING);
        if (status != NO_ERROR) {
            break;
        }

        *encryptedDataLen = ivLen + ciphertxtLen;

        *encryptedData = (BYTE *)malloc(*encryptedDataLen);
        if (*encryptedData == NULL) {
            status = NTE_NO_MEMORY;
            break;
        }
        memcpy(*encryptedData, iv, ivLen);
        
        BYTE *ciphertxt = *encryptedData + ivLen;
        ULONG _ = 0;
        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, ciphertxt, ciphertxtLen, &_, BCRYPT_BLOCK_PADDING);
    } while (0);

    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (iv) {
        free(iv);
    }
    if (ivCopy) {
        free(ivCopy);
    }
    return status;
}

NTSTATUS doDecrypt(BYTE *key, ULONG keyLen, 
    BYTE *encryptedData, ULONG encryptedDataLen, 
    BYTE **plaintxt, ULONG *plaintxtLen)
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    BYTE *iv = NULL;
    do {
        status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, NULL, 0);
        if (status != NO_ERROR) {
            break;
        }
        
        status = BCryptGenerateSymmetricKey(hAlg, &hKey, NULL, 0, key, keyLen, 0);
        if (status != NO_ERROR) {
            break;
        }

        const wchar_t mode[] = BCRYPT_CHAIN_MODE_CBC;
        ULONG modeLen = (ULONG)sizeof(mode);
        status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PUCHAR)mode, modeLen, 0);
        if (status != NO_ERROR) {
            break;
        }
        
        ULONG ivLen = 0;
        status = getIvLen(hAlg, &ivLen);
        if (status != NO_ERROR) {
            break;
        }

        iv = (BYTE *)malloc(ivLen);
        if (iv == NULL) {
            status = NTE_NO_MEMORY;
            break;
        }
        memcpy(iv, encryptedData, ivLen);

        BYTE *ciphertxt = encryptedData + ivLen;
        ULONG ciphertxtLen = encryptedDataLen - ivLen;
        status = BCryptDecrypt(hKey, ciphertxt, ciphertxtLen, NULL, iv, ivLen, NULL, 0, plaintxtLen, BCRYPT_BLOCK_PADDING);
        if (status != NO_ERROR) {
            break;
        }
        *plaintxt = (BYTE *)malloc(*plaintxtLen);
        if (*plaintxt == NULL) {
            status = NTE_NO_MEMORY;
            break;
        }
        
        ULONG _ = 0;
        status = BCryptDecrypt(hKey, ciphertxt, ciphertxtLen, NULL, iv, ivLen, *plaintxt, *plaintxtLen, &_, BCRYPT_BLOCK_PADDING);
    } while (0);

    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (iv) {
        free(iv);
    }
    return status;
}

NTSTATUS encrypt(BYTE *pw, ULONG pwLen, BYTE *plaintxt, ULONG plaintxtLen, 
    BYTE **encryptedData, ULONG *encryptedDataLen) 
{
    NTSTATUS status = NTE_FAIL;
    BYTE *salt = NULL;
    BYTE *key = (BYTE *)malloc(KEY_LEN);
    if (key == NULL) {
        free(key);
        return NTE_NO_MEMORY;
    }
    do {
        status = getSalt(salt, 0);
        if (status != NO_ERROR) {
            break;
        }

        status = deriveKey(pw, pwLen, salt, SALT_LEN, NUM_ITERATIONS, key, KEY_LEN);
        if (status != NO_ERROR) {
            break;
        }

        status = doEncrypt(key, KEY_LEN, plaintxt, plaintxtLen, encryptedData, encryptedDataLen);
    } while (0);
    mk_clearMemory(key, KEY_LEN);
    free(key);
    free(salt);
    return status;
}

NTSTATUS decrypt(BYTE *pw, ULONG pwLen, BYTE *ciphertxt, ULONG ciphertxtLen, 
    BYTE **plaintxt, ULONG *plaintxtLen) 
{
    NTSTATUS status = NTE_FAIL;
    BYTE *salt = NULL;
    BYTE *key = (BYTE *)malloc(KEY_LEN);
    if (key == NULL) {
        free(key);
        return NTE_NO_MEMORY;
    }

    do {
        status = getSalt(salt, 0);
        if (status != NO_ERROR) {
            break;
        }

        status = deriveKey(pw, pwLen, salt, SALT_LEN, NUM_ITERATIONS, key, KEY_LEN);
        if (status != NO_ERROR) {
            break;
        }

        status = doDecrypt(key, KEY_LEN, ciphertxt, ciphertxtLen, plaintxt, plaintxtLen);
        if (status != NO_ERROR) {
            break;
        }
    } while (0);
    mk_clearMemory(key, KEY_LEN);
    free(key);
    free(salt);
    return status;
}

int encryptText(char *pw, char *text, BYTE **ciphertxt, ULONG *ciphertxtLen) {
    NTSTATUS status = encrypt((BYTE *)pw, (ULONG)(strlen(pw) + 1), 
        (BYTE *)text, (ULONG)(strlen(text) + 1), ciphertxt, ciphertxtLen
    );
    printf("Text encrypted (Status: %ld)\n", status);
    return status == NO_ERROR;
}

int decryptText(char *pw, BYTE *ciphertxt, ULONG ciphertxtLen, char **text) {
    ULONG _ = 0;
    NTSTATUS status = decrypt((BYTE *) pw, (ULONG)(strlen(pw) + 1),
        ciphertxt, ciphertxtLen, (BYTE **)text, &_
    );
    printf("Text decrypted (Status: %ld)\n", status);
    return status == NO_ERROR;
}

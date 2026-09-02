#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <wchar.h>

#include "crypto.h"

#ifdef _WIN32
    #include <bcrypt.h>
#endif

static NTSTATUS genRandom(BYTE *buffer, ULONG bufferLen) {
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

static NTSTATUS derive_key(BYTE *pw, ULONG pwLen, BYTE *salt, ULONG saltLen, const ULONG iterations, BYTE *derivedKey, ULONG derivedKeyLen) {
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

static NTSTATUS do_encrypt(BYTE *key, ULONG keyLen,
    BYTE *plaintxt, ULONG plaintxtLen, BYTE *iv, ULONG *ivLen, 
    BYTE *ciphertxt, ULONG *ciphertxtLen)
{
    NTSTATUS status = NTE_FAIL;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
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

        ULONG result = 0;
        status = BCryptGetProperty(hAlg, BCRYPT_BLOCK_LENGTH, (PUCHAR)&ivLen, sizeof(ivLen), &result, 0);
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

        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, NULL, 0, ciphertxtLen, BCRYPT_BLOCK_PADDING);
        if (status != NO_ERROR) {
            break;
        }
        ciphertxt = (BYTE *)malloc(*ciphertxtLen);
        if (ciphertxt == NULL) {
            status = NTE_NO_MEMORY;
            break;
        }

        status = BCryptEncrypt(hKey, plaintxt, plaintxtLen, NULL, ivCopy, ivLen, ciphertxt, *ciphertxtLen, ciphertxtLen, BCRYPT_BLOCK_PADDING);
    } while (0);

    if (hKey) {
        BCryptDestroyKey(hKey);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    if (ivCopy) {
        free(ivCopy);
    }
    return status;
}

int encrypt(const char* fileName) {
    // BCRYPT_KEY_HANDLE hKey;
    // NTSTATUS status = BCryptGenerateSymmetricKey(NULL, NULL, NULL, 0, NULL, 0, BCRYPT_AES_ALGORITHM);

    // FILE *filePtr = fopen(fileName, "wb");
    // if (filePtr == NULL) {
    //     printf("Error opening file for writing.\n");
    //     return -1;
    // }
    
}
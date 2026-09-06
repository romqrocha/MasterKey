#ifndef CRYPTO_H
#define CRYPTO_H

#ifdef _WIN32
    #include <windows.h>
#endif

void mk_clearMemory(void *pointer, size_t bytes);
NTSTATUS genRandom(BYTE *buffer, ULONG bufferLen);
NTSTATUS deriveKey(BYTE *pw, ULONG pwLen, BYTE *salt, ULONG saltLen, const ULONG iterations, BYTE *derivedKey, ULONG derivedKeyLen);
int encryptText(char *pw, char *text, BYTE **ciphertxt, ULONG *ciphertxtLen);
int decryptText(char *pw, BYTE *ciphertxt, ULONG ciphertxtLen, char **text);

#endif
#ifndef CRYPTO_H
#define CRYPTO_H

#ifdef _WIN32
    #include <windows.h>
#endif

int encryptText(char *pw, char *text, BYTE *ciphertxt, ULONG *ciphertxtLen);
int decryptText(char *pw, BYTE *ciphertxt, ULONG ciphertxtLen, char *text);

#endif
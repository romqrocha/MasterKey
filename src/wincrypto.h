#ifndef WIN_CRYPTO_H
#define WIN_CRYPTO_H

typedef unsigned char BYTE;
typedef unsigned long ULONG;

int genRandom(BYTE *buffer, ULONG bufferLen);
int deriveKey(BYTE *pw, ULONG pwLen, BYTE *salt, ULONG saltLen, const ULONG iterations, BYTE *derivedKey, ULONG derivedKeyLen);
int encryptText(char *pw, char *text, BYTE **ciphertxt, ULONG *ciphertxtLen);
int decryptText(char *pw, BYTE *ciphertxt, ULONG ciphertxtLen, char **text);

#endif
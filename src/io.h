#ifndef IO_H
#define IO_H

int read(const char *fileName, BYTE *data, ULONG dataLen);
int write(BYTE *data, ULONG dataLen, const char *fileName);

#endif
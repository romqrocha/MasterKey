#ifndef IO_H
#define IO_H

int mk_read(const char *fileName, BYTE *data, ULONG dataLen);
int mk_write(BYTE *data, ULONG dataLen, const char *fileName);
int mk_exists(const char *fileName);

#endif
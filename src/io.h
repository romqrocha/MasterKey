#ifndef IO_H
#define IO_H

long mk_fileLength(const char *fileName);
int mk_exists(const char *fileName);
int mk_read(const char *fileName, void *data, size_t dataLen);
int mk_write(const void *data, size_t dataLen, const char *fileName);
char *mk_nextFileInDir(const char *dirName, void **currFile, int *err);
size_t mk_countFilesInDir(const char *dirName);

#endif
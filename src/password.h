#ifndef PASSWORD_H
#define PASSWORD_H

#define HASHED_PASSWORD_FILE "hashed.key"
#define MASTER_PASSWORD_MAX_LEN 64
#define MASTER_PASSWORD_MIN_LEN 8

void mk_clearMemory(void *pointer, size_t bytes);
int setPassword(char *pw);
int testPassword(char *pw);

#endif
#ifndef PASSWORD_H
#define PASSWORD_H

#define HASHED_PASSWORD_FILE "hashed.key"
#define MASTER_PASSWORD_MAX_LEN 64

int setPassword(char *pw);
int testPassword(char *pw);

#endif
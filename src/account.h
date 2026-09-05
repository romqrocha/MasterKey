#ifndef ACCOUNT_H
#define ACCOUNT_H

#define ACCOUNT_MAX_NAME_LEN 64
#define ACCOUNT_MAX_EMAIL_LEN 254
#define ACCOUNT_MAX_PW_LEN 64
#define ACCOUNT_MAX_KEY_LEN 128

typedef struct AccountKVP {
    char key[ACCOUNT_MAX_KEY_LEN];
    char *value;
    struct AccountKVP *next;
} AccountKVP;

typedef struct Account {
    char name[ACCOUNT_MAX_NAME_LEN];
    char email[ACCOUNT_MAX_EMAIL_LEN];
    char password[ACCOUNT_MAX_PW_LEN];
    char oauthProvider[ACCOUNT_MAX_NAME_LEN];
    AccountKVP *additionalInfo;
} Account;

char *serialize(Account *account);
Account *deserialize(char *serializedAccount, char *name);
void destroy(Account *account);

#endif
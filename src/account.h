#ifndef ACCOUNT_H
#define ACCOUNT_H

#define ACCOUNT_MAX_NAME_LEN 64
#define ACCOUNT_MAX_EMAIL_LEN 255
#define ACCOUNT_MAX_PW_LEN 128
#define ACCOUNT_MAX_KEY_LEN 128
#define ACCOUNT_DIR "accounts/"
#define ACCOUNT_DIR_FILES "accounts/*"
#define ACCOUNT_FILE_EXT ".bin"
#define ACCOUNT_MAX_FILENAME_LEN ACCOUNT_MAX_NAME_LEN + 9 + 4

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
void getAccountFileName(Account *account, char buffer[ACCOUNT_MAX_FILENAME_LEN]);

#endif
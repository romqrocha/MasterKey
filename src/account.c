#include <stdlib.h>

#define MAX_NAME_LEN 64
#define MAX_PW_LEN 64
#define MAX_KEY_LEN 128

typedef struct {
    char key[MAX_KEY_LEN];
    char *value;
    AccountKVP *next;
} AccountKVP;

typedef struct {
    char name[MAX_NAME_LEN];
    char password[MAX_PW_LEN];
    char oauthProvider[MAX_NAME_LEN];
    AccountKVP *additionalInfo;
} Account;

size_t getSerializedLength(Account *account) {
    // size_t totalLen = MAX_NAME_LEN + MAX_PW_LEN + MAX_NAME_LEN;
    // AccountKVP *kvp = account->additionalInfo;
    // while (kvp != NULL) {
    //     totalLen += MAX_KEY_LEN;
    // }
}

char *serialize(Account *account) {
    size_t totalLen = getSerializedLength(account);
    char *serializedAccount = (char *)malloc(totalLen);
    if (!serializedAccount) {
        return NULL;   
    }

    return serializedAccount;
}

Account *deserialize(char *serializedAccount) {

}

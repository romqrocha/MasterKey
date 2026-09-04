#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_NAME_LEN 64
#define MAX_PW_LEN 64
#define MAX_KEY_LEN 128

typedef struct AccountKVP {
    char key[MAX_KEY_LEN];
    char *value;
    AccountKVP *next;
} AccountKVP;

typedef struct Account {
    char name[MAX_NAME_LEN];
    char password[MAX_PW_LEN];
    char oauthProvider[MAX_NAME_LEN];
    AccountKVP *additionalInfo;
} Account;

size_t getSerializedLength(Account *account) {
    size_t totalLen = 0;
    totalLen += strlen(account->password) + 1; // +1 for \n
    totalLen += strlen(account->oauthProvider) + 1;
    AccountKVP *kvp = account->additionalInfo;
    while (kvp != NULL) {
        totalLen += strlen(kvp->key) + 1;
        totalLen += strlen(kvp->value) + 1;
        kvp = kvp->next;
    }
    return totalLen + 1; // +1 for \0
}

void *appendLine(char *buffer, unsigned long long *currIndex, char *contents) {
    size_t contentsLen = strlen(contents);
    memcpy(buffer + *currIndex, contents, contentsLen);
    *currIndex += contentsLen;
    buffer[(*currIndex)++] = '\n';
}

char *serialize(Account *account) {
    size_t totalLen = getSerializedLength(account);
    char *serializedAccount = (char *)malloc(totalLen);
    if (!serializedAccount) {
        return NULL;   
    }

    unsigned long long charCount = 0;
    appendLine(serializedAccount, &charCount, account->password);
    appendLine(serializedAccount, &charCount, account->oauthProvider);
    AccountKVP *kvp = account->additionalInfo;
    while (kvp != NULL) {
        appendLine(serializedAccount, &charCount, kvp->key);
        appendLine(serializedAccount, &charCount, kvp->value);
        kvp = kvp->next;
    }
    serializedAccount[charCount] = '\0'; 

    return serializedAccount;
}

Account *deserialize(char *serializedAccount, char *name) {
    Account *account = calloc(1, sizeof(*account));
    if (!account) {
        return NULL;
    }

    snprintf(account->name, sizeof(account->name), "%s", name);

    // deserialize here

    return account;
}

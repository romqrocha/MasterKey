#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "account.h"

size_t getSerializedLength(Account *account) {
    size_t totalLen = 0;
    totalLen += strlen(account->email) + 1; // +1 for \n
    totalLen += strlen(account->password) + 1;
    totalLen += strlen(account->oauthProvider) + 1;
    AccountKVP *kvp = account->additionalInfo;
    while (kvp != NULL) {
        totalLen += strlen(kvp->key) + 1;
        totalLen += strlen(kvp->value) + 1;
        kvp = kvp->next;
    }
    return totalLen + 1; // +1 for \0
}

void appendLine(char *buffer, unsigned long long *currIndex, char *contents) {
    size_t contentsLen = strlen(contents);
    memcpy(buffer + *currIndex, contents, contentsLen);
    *currIndex += contentsLen;
    buffer[(*currIndex)++] = '\n';
}

AccountKVP *deserializeKVP(char *buffer, unsigned long long *currIndex) {
    AccountKVP *kvp = calloc(1, sizeof(*kvp));

    for (int i = 0; buffer[*currIndex] != '\0' ; i++) {
        if (buffer[(*currIndex)++] == '\n') {
            break;
        } else {
            kvp->key[i] = buffer[(*currIndex)++];
        }
    }
    if (kvp->key[0] == '\0') {
        return NULL;
    }

    size_t valueLen = 0;
    while (buffer[*currIndex + valueLen] != '\n') {
        valueLen++;
    }
    valueLen++; // +1 for /0

    kvp->value = (char *)malloc(valueLen);
    if (kvp->value == NULL) {
        return NULL;
    }

    for (unsigned long long i = 0; buffer[*currIndex] != '\0' ; i++) {
        if (buffer[(*currIndex)++] == '\n') {
            break;
        } else {
            kvp->value[i] = buffer[(*currIndex)++];
        }
    }

    return kvp;
}

char *serialize(Account *account) {
    size_t totalLen = getSerializedLength(account);
    char *serializedAccount = (char *)malloc(totalLen);
    if (!serializedAccount) {
        return NULL;   
    }

    unsigned long long charCount = 0;
    appendLine(serializedAccount, &charCount, account->email);
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

    if (strlen(name) >= ACCOUNT_MAX_NAME_LEN) {
        name[ACCOUNT_MAX_NAME_LEN - 1] = '\0';
    }
    snprintf(account->name, sizeof(account->name), "%s", name);

    unsigned long long charCount = 0;
    
    char email[ACCOUNT_MAX_EMAIL_LEN];
    int emailLen = 0;
    while (serializedAccount[charCount + emailLen] != '\n') {
        account->password[emailLen] = serializedAccount[charCount + emailLen];
        emailLen++;
    }
    charCount += emailLen + 1; // +1 for \n
    memcpy(account->email, email, emailLen);

    char pw[ACCOUNT_MAX_PW_LEN];
    int pwLen = 0;
    while (serializedAccount[charCount + pwLen] != '\n') {
        account->password[pwLen] = serializedAccount[charCount + pwLen];
        pwLen++;
    }
    charCount += pwLen + 1; // +1 for \n
    memcpy(account->password, pw, pwLen);

    char oauth[ACCOUNT_MAX_NAME_LEN];
    int oauthLen = 0;
    while (serializedAccount[charCount + oauthLen] != '\n') {
        account->oauthProvider[oauthLen] = serializedAccount[charCount + oauthLen];
        oauthLen++;
    }
    charCount += oauthLen + 1; // +1 for \n
    memcpy(account->oauthProvider, oauth, oauthLen);

    AccountKVP *nextKVP = deserializeKVP(serializedAccount, &charCount);
    account->additionalInfo = nextKVP;
    while (nextKVP != NULL) {
        nextKVP->next = deserializeKVP(serializedAccount, &charCount);
        nextKVP = nextKVP->next;
    }

    return account;
}

void destroyRecursively(AccountKVP *kvp) {
    if (kvp == NULL) {
        return;
    }

    destroyRecursively(kvp->next);

    free(kvp->value);
    free(kvp);
}

void destroy(Account *account) {
    destroyRecursively(account->additionalInfo);

    free(account);
}
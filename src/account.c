#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "account.h"
#include "password.h"

#ifdef _WIN32
    #include <windows.h>
    #include "wincrypto.h"
#endif
#ifndef _WIN32
    // future non windows cryptography library...
#endif

/**
 * Returns the number of bytes required to serialize the given account struct.
 */
size_t getSerializedLength(Account *account) {
    size_t totalLen = 0;
    AccountKVP *kvp = account->additionalInfo;

    totalLen += strlen(account->email) + 1; // +1 for \n
    totalLen += strlen(account->password) + 1;
    totalLen += strlen(account->oauthProvider) + 1;
    while (kvp != NULL) {
        totalLen += strlen(kvp->key) + 1;
        totalLen += strlen(kvp->value) + 1;
        kvp = kvp->next;
    }

    return totalLen + 1; // +1 for \0
}

/**
 * Appends a string and a newline to a buffer at the given index.
 * The index is incremented accordingly.
 */
void appendLine(char *buffer, unsigned long long *currIndex, char *contents) {
    size_t contentsLen = strlen(contents);

    memcpy(buffer + *currIndex, contents, contentsLen);
    *currIndex += contentsLen;
    buffer[(*currIndex)++] = '\n';
}

/**
 * Creates an AccountKVP struct with the information serialized in a buffer at 
 * the given index.
 * The index is incremented accordingly.
 * Allocates memory for the struct and returns the address.
 */
AccountKVP *deserializeKVP(char *buffer, unsigned long long *currIndex) {
    // remember to free later
    AccountKVP *kvp = calloc(1, sizeof(*kvp));
    if (!kvp) {
        return NULL;
    }

    // deserialize key
    for (int i = 0; buffer[*currIndex] != '\0'; i++) {
        if (buffer[(*currIndex)++] == '\n') {
            break;
        } else {
            kvp->key[i] = buffer[(*currIndex)++];
        }
    }
    if (kvp->key[0] == '\0') {
        free(kvp);
        return NULL;
    }

    // find number of bytes to deserialize value
    size_t valueLen = 0;
    while (buffer[*currIndex + valueLen] != '\n') {
        valueLen++;
    }
    valueLen++; // +1 for '\0'

    kvp->value = (char *)malloc(valueLen);
    if (kvp->value == NULL) {
        free(kvp);
        return NULL;
    }

    // deserialize value
    for (unsigned long long i = 0; buffer[*currIndex] != '\0' ; i++) {
        if (buffer[(*currIndex)++] == '\n') {
            break;
        } else {
            kvp->value[i] = buffer[(*currIndex)++];
        }
    }

    return kvp;
}

/**
 * Creates an Account struct with the information serialized in the given buffer.
 * Allocates memory for the struct and returns the address.
 */
Account *deserialize(char *serializedAccount, char *name) {
    // remember to free later
    Account *account = calloc(1, sizeof(*account));
    if (!account) {
        return NULL;
    }

    name[ACCOUNT_MAX_NAME_LEN - 1] = '\0'; // guarantee no overflow
    memcpy(account->name, name, strlen(name));

    unsigned long long charCount = 0;
    
    char email[ACCOUNT_MAX_EMAIL_LEN];
    int emailLen = 0;
    while (serializedAccount[charCount + emailLen] != '\n') {
        account->email[emailLen] = serializedAccount[charCount + emailLen];
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

/**
 * Serializes an account struct to a string.
 * Allocates memory for the string and returns the address.
 */
char *serialize(Account *account) {
    size_t totalLen = getSerializedLength(account);

    // remember to free later
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

/**
 * Frees memory held by the given AccountKVP and all of the ones linked to it.
 */
void destroyRecursively(AccountKVP *kvp) {
    if (kvp == NULL) {
        return;
    }

    destroyRecursively(kvp->next);

    free(kvp->value);
    free(kvp);
}

/**
 * Frees memory held by the given Account and all of its AccountKVPs.
 * Makes sure to clear the password from memory.
 */
void destroy(Account *account) {
    destroyRecursively(account->additionalInfo);

    mk_clearMemory(account->password, ACCOUNT_MAX_PW_LEN);

    free(account);
}

/**
 * Writes the account's file name to the buffer in the form:
 * "<ACCOUNT_DIR>/<Name>.<ACCOUNT_FILE_EXT>"
 */
void getAccountFileName(Account *account, char buffer[ACCOUNT_MAX_FILENAME_LEN]) {
    char *ptr = buffer;

    memcpy(ptr, ACCOUNT_DIR, strlen(ACCOUNT_DIR));
    ptr += strlen(ACCOUNT_DIR);

    memcpy(ptr, account->name, strlen(account->name));
    ptr += strlen(account->name);

    memcpy(ptr, ACCOUNT_FILE_EXT, strlen(ACCOUNT_FILE_EXT) + 1); // copy the \0
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
    #include <conio.h>
    #include "wincrypto.h"
    #include "winterminal.h"
#else
    // ummm
#endif

#include "io.h"
#include "account.h"
#include "password.h"

#define OUT_OF_MEMORY "Out of memory. Please try again later.\n"
#define MAX_USER_INPUT_LEN 32768
#define MAX_VIEWABLE 9
#define ASCII_TO_INT -48

typedef unsigned char byte;

void outputIntro() {
    printf("Program initialized.\n");
}

int setup() {
    char *pw1 = calloc(MASTER_PASSWORD_MAX_LEN, sizeof(*pw1));
    char *pw2 = calloc(MASTER_PASSWORD_MAX_LEN, sizeof(*pw2));
    if (!pw1 || !pw2) {
        printf("ok i get it\n");
        printf("RAM is expensive these days\n");
        printf("but you're telling me you couldn't spare even ONE Kb.\n");
        printf("...\n");
        printf("... this is unrecoverable\n");
        printf("program terminated.\n");
        exit(1);
    }

    printf("You'll need a master password to continue.\n");
    printf("It must be 8-63 characters long.\n");

    const char prompt2[] = "Enter your master password again to confirm: ";
    const char prompt1[] = "Enter your master password: ";
    const char underLimitError[] = "The password must have at least 8 characters. Please try again.\n";
    const char matchingError[] = "The passwords did not match. Please try again.\n";
    const char cancelledMsg[] = "Escape key pressed. Cancelling setup.\n";
    const char resetMsg[] = "Escape key pressed. Restarting process.\n";

    while (1) {
        // make sure to clear pw1 and pw2 between iterations
        mk_clearMemory(pw1, MASTER_PASSWORD_MAX_LEN);
        mk_clearMemory(pw2, MASTER_PASSWORD_MAX_LEN);

        inputLine(prompt1, pw1, MASTER_PASSWORD_MAX_LEN, "", 1);
        if (pw1[0] == ESC_KEY) {
            printf(cancelledMsg);
            exit(0);
        }
        if (strlen(pw1) < MASTER_PASSWORD_MIN_LEN) {
            printf(underLimitError);
            continue;
        }

        inputLine(prompt2, pw2, MASTER_PASSWORD_MAX_LEN, "", 1);
        if (pw2[0] == ESC_KEY) {
            printf(resetMsg);
            continue;
        }
        if (strcmp(pw1, pw2) != 0) {
            printf(matchingError);
            continue;
        }

        break;
    }

    printf("Passwords OK. This master password will be hashed and saved.\n");
    
    int success = setPassword(pw1);
    if (!success) {
        printf("The password could not be saved.\n");
        return 0;
    }

    int sanityCheck = testPassword(pw2);
    
    mk_clearMemory(pw1, MASTER_PASSWORD_MAX_LEN);
    mk_clearMemory(pw2, MASTER_PASSWORD_MAX_LEN);
    free(pw1);
    free(pw2);
    if (sanityCheck) {
        printf("Setup complete.\n");
        return 1;
    } else {
        printf("Sanity check failed. Setup failed.\n");
        return 0;
    }
}

void strToUpper(char **str) {
    int i = 0;
    for (char c = (*str)[i]; c != '\0'; c = (*str)[++i]) {
        (*str)[i] = (char)toupper((*str)[i]);
    }
}

void whileViewing(char *fileName) {
    printf("You want to view %s\n", fileName);
}

void whileBrowsing() {
    // remember to free at the end
    char **fileNames = NULL;
    char **searchableNames = NULL;

    size_t numOfAccounts = mk_countFilesInDir(ACCOUNT_DIR_FILES);
    if (numOfAccounts == 0) {
        printf("No accounts have been added yet.\n");
        return;
    }
    
    searchableNames = calloc(numOfAccounts, sizeof(*searchableNames));
    if (!searchableNames) {
        perror(OUT_OF_MEMORY);
        goto cleanup;
    }
    for (int i = 0; i < numOfAccounts; i++) {
        searchableNames[i] = malloc(ACCOUNT_MAX_NAME_LEN);
        if (!searchableNames[i]) {
            perror(OUT_OF_MEMORY);
            goto cleanup;
        }
    }

    fileNames = calloc(numOfAccounts, sizeof(*fileNames));
    if (!fileNames) {
        perror(OUT_OF_MEMORY);
        goto cleanup;
    }

    void *currFile = NULL;
    for (int i = 0; i < numOfAccounts; i++) {
        fileNames[i] = mk_nextFileInDir(ACCOUNT_DIR_FILES, &currFile);
        if (fileNames[i] == NULL) {
            perror(OUT_OF_MEMORY);
            goto cleanup;
        } else if (currFile == NULL) {
            perror("Unexpect null while iterating through account files.\n");
            goto cleanup;
        }
        int searchableNameLen = strlen(fileNames[i]) - strlen(ACCOUNT_FILE_EXT);
        memcpy(searchableNames[i], fileNames[i], searchableNameLen);
        searchableNames[i][searchableNameLen] = '\0';
        strToUpper(&(searchableNames[i]));
    }

    char filter[ACCOUNT_MAX_NAME_LEN] = "";
    char filterToUpper[ACCOUNT_MAX_NAME_LEN] = "";
    int filterLen = 0;
    char *names[MAX_VIEWABLE];
    int matchesFound = 0;
    char curName[ACCOUNT_MAX_NAME_LEN];
    int curNameLen = 0;
    int keyPressed;
    int savedIndex = 0;

    while (1) {
        matchesFound = 0;

        // filter
        for (int i = savedIndex; i < numOfAccounts && matchesFound < MAX_VIEWABLE; savedIndex = ++i) {
            if (strstr(searchableNames[i], filterToUpper)) {
                names[matchesFound++] = fileNames[i];
            }
        }

        // print filtered options
        for (int i = 0; i < matchesFound; i++) {
            curNameLen = strlen(names[i]) - strlen(ACCOUNT_FILE_EXT) + 1;
            memcpy(curName, names[i], curNameLen);
            curName[curNameLen - 1] = '\0';

            printf("%d. %s\n", i + 1, curName);
        }
        if (matchesFound == 0) {
            printf("No matches found.\n");
        }

        // wait for user input
        printf("Filter by name: %s", filter);
        keyPressed = inputKey();
        printf("\n");

        // update terminal according to user input
        if (keyPressed == ESC_KEY) {
            savedIndex -= (MAX_VIEWABLE + matchesFound);
            if (savedIndex < 0) {
                break;
            }
        } else if (keyPressed == BACKSPACE_KEY) {
            savedIndex = 0;

            if (filterLen > 0) {
                filterLen--;
                filter[filterLen] = '\0';
                filterToUpper[filterLen] = '\0';
                printf("\b \b");
            }
        } else if (keyPressed == ENTER_KEY) {
            if (savedIndex >= numOfAccounts) {
                savedIndex -= matchesFound;
            }
        } else if (keyPressed >= '1' && keyPressed <= '9') {
            savedIndex -= matchesFound;
            
            int keyAsInt = keyPressed + ASCII_TO_INT;
            whileViewing(names[keyAsInt - 1]);
        } else if (filterLen < ACCOUNT_MAX_NAME_LEN - 1) { // reserve 1 for '\0'
            savedIndex = 0;

            filter[filterLen] = (char)keyPressed;
            filterToUpper[filterLen] = (char)toupper(keyPressed);
            filterLen++;
            filter[filterLen] = '\0';
            filterToUpper[filterLen] = '\0';
        }
    }

cleanup:
    for (int i = 0; i < numOfAccounts; i++) {
        if (fileNames) {
            free(fileNames[i]);
        }
        if (searchableNames) {
            free(searchableNames[i]);
        }
    }
    free(fileNames);
    free(searchableNames);

    printf("\n");
}

/**
 * Creates a new AccountKVP from terminal input.
 * Allocates memory for the AccountKVP struct and returns the address.
 */
AccountKVP *addKvp() {
    // remember to free later
    AccountKVP *newKvp = calloc(1, sizeof(*newKvp));
    if (!newKvp) {
        printf(OUT_OF_MEMORY);
        return NULL;
    }
    char *newValue = NULL;
    size_t valueLen = MAX_USER_INPUT_LEN;
    while (!newValue) {
        // Reducing allocation size if needed
        if (valueLen < ACCOUNT_MAX_KEY_LEN) {
            printf(OUT_OF_MEMORY);
            free(newKvp);
            return NULL;
        }
        newValue = malloc(valueLen);
        if (!newValue) {
            valueLen /= 4;
        }
    }

    // input key
    char newKey[ACCOUNT_MAX_KEY_LEN];
    inputLineSimply("Key: ", newKey, ACCOUNT_MAX_KEY_LEN);
    if (strlen(newKey) == 0 || newKey[0] == ESC_KEY) {
        free(newValue);
        free(newKvp);
        return NULL;
    }
    memcpy(newKvp->key, newKey, strlen(newKey) + 1);

    // input value
    inputLineSimply("Value: ", newValue, valueLen);
    if (newValue[0] == ESC_KEY) {
        free(newValue);
        free(newKvp);
        return NULL;
    }
    valueLen = strlen(newValue) + 1;
    
    // reallocate newValue to minimum size
    newKvp->value = malloc(valueLen);
    if (newKvp->value == NULL) {
        printf(OUT_OF_MEMORY);
        free(newValue);
        free(newKvp);
        return NULL;
    }
    memcpy(newKvp->value, newValue, valueLen);
    
    // set next pointer to null
    newKvp->next = NULL;

    free(newValue);
    return newKvp;
}

void whileAdding(char *pw) {
    // remember to free later
    unsigned char *ciphertxt = NULL;
    char *serializedAccount = NULL;
    Account *account = account = calloc(1, sizeof(*account));
    if (!account) {
        printf(OUT_OF_MEMORY);
        return;
    }

    const char oauthMsg[] = " * Leave the Email field empty to specify an OAuth provider instead.\n";
    const char additionalInfoMsg[] = "Now you can specify additional account information, saved in the format \"<Key>: <Value>\".\n";
    const char emptyKeyMsg[] = " * When you are done, press ESC to exit.\n";
    const char encryptionError[] = "An error occurred while encrypting the account information. Please try again later.\n";
    const char allDone[] = "All account information was saved successfully.\n";
    const char cancelled[] = "ESC key pressed. Cancelling process.\n";
    const char invalidName[] = "Invalid account name. Cancelling process.\n";

    inputLine("Account Name: ", account->name, ACCOUNT_MAX_NAME_LEN, DISALLOWED_CHARS, 0);
    int charCount = strlen(account->name);
    if (charCount == 0 || account->name[0] == ESC_KEY) {
        free(account);
        printf(cancelled);
        return;
    }
    while(strchr(". ", account->name[--charCount]) != NULL) {
        if (charCount == 0) {
            free(account);
            printf(invalidName);
            return;
        }
        account->name[charCount] = '\0';
    }

    // get account email or leave empty for oauth
    printf(oauthMsg);
    inputLineSimply("Email: ", account->email, ACCOUNT_MAX_EMAIL_LEN);

    if (strlen(account->email) == 0 || account->email[0] == ESC_KEY) {
        // no email/password, just oauth provider
        inputLineSimply("Name of OAuth Provider: ", account->oauthProvider, ACCOUNT_MAX_NAME_LEN);
    } else {
        inputLineSimply("Password: ", account->password, ACCOUNT_MAX_PW_LEN);
    }

    printf(additionalInfoMsg);
    printf(emptyKeyMsg);
    
    AccountKVP *additionalInfo = addKvp();
    account->additionalInfo = additionalInfo;
    while (additionalInfo != NULL) {
        additionalInfo->next = addKvp();
        additionalInfo = additionalInfo->next;
    }

    serializedAccount = serialize(account);
    char fileName[ACCOUNT_MAX_FILENAME_LEN];
    getAccountFileName(account, fileName);

    unsigned long ciphertxtLen = 0;
    if (!encryptText(pw, serializedAccount, &ciphertxt, &ciphertxtLen)) {
        printf(encryptionError);
    } else {
        if (!mk_write((byte *)serializedAccount, strlen(serializedAccount), fileName)) {
            printf(encryptionError);
        } else {
            printf(allDone);
        }
    }

    free(ciphertxt);
    free(serializedAccount);
    destroy(account);
}

void whileSyncing() {
    // maybe really cool peer-to-peer syncing in the future???
}

void whileUnlocked(char *pw) {
    const char options[] = "1. View accounts\n2. Add new account\n3. Sync\n";

    char choice;
    do {
        printf("Options:\n");
        choice = inputChoice(options);
        if (choice == '1') {
            printf("Viewing accounts...\n");
            whileBrowsing();
        } else if (choice == '2') {
            printf("Adding new account...\n");
            whileAdding(pw);
        } else if (choice == '3') {
            printf("Syncing...\n");
            whileSyncing();
        } else if (choice == ESC_KEY){
            printf("Locking...\n");
        } else {
            printf("Invalid choice.\n");
        }
    } while (choice != ESC_KEY);
}

void whileLocked() {
    // remember to free later
    char *pw = calloc(MASTER_PASSWORD_MAX_LEN, sizeof(*pw));

    const char prompt[] = "Enter the password: ";
    const char wrongPwError[] = "Incorrect password. Please try again.\n";
    const char rightPw[] = "Password OK. Unlocking password manager.\n";
    const char cancelled[] = "Escape key pressed. Exiting program.\n";

    while (1) {
        inputLine(prompt, pw, MASTER_PASSWORD_MAX_LEN, "", 1);
        if (pw[0] == ESC_KEY) {
            printf(cancelled);
            break;
        }

        int authorized = testPassword(pw);
        if (!authorized) {
            printf(wrongPwError);
        } else {
            printf(rightPw);
            whileUnlocked(pw);
        }
        mk_clearMemory(pw, MASTER_PASSWORD_MAX_LEN);
    }

    mk_clearMemory(pw, MASTER_PASSWORD_MAX_LEN);
    free(pw);
}

int main() {
    outputIntro();
    
    int passwordExists = mk_exists(HASHED_PASSWORD_FILE);
    if (!passwordExists) { 
        if (!setup()) {
            exit(1);
        }
    }

    whileLocked();
    
    return 0;
}

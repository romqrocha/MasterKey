#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <conio.h>
#endif

#include "crypto.h"
#include "io.h"
#include "account.h"
#include "password.h"

#define ESC_KEY '\x1B'
#define ENTER_KEY '\xD'
#define BACKSPACE_KEY '\x8'
#define HIDDEN_CHAR '#'
#define OUT_OF_MEMORY "Out of memory. Please try again later.\n"
#define MAX_USER_INPUT_LEN 32768

void outputIntro() {
    printf("Program initialized.\n");
}

char inputChoice(const char *prompt) {
    printf(prompt);
    
    char choice;
    // duuuude i wanted to code this for both windows and unix at first but man it's way too time consuming
    #ifdef _WIN32
        printf("Choose with your keyboard.\n");
        choice = _getch();
        printf("\n");
    #else
        printf("Enter your choice: ");
        choice = fgetc(stdin);
        while (fgetc(stdin) != '\n');
    #endif
    
    return choice;
}

int inputLine(const char *prompt, char *buffer, int charLimit) {
    int overLimit = 0;
    
    printf(prompt);

    fgets(buffer, charLimit, stdin);
    if (buffer[strlen(buffer) - 1] == '\n') {
        // characters entered < limit
        buffer[strlen(buffer) - 1] = '\0';
    } else if (fgetc(stdin) != '\n') {
        // characters entered > limit
        while (fgetc(stdin) != '\n');
        overLimit = 1;
    }

    return overLimit;
}

void inputLineSecretly(const char *prompt, char *buffer, int bufferLen) {
    // backspace to delete, enter to submit
    printf(prompt);
    char ch;
    int charCount = 0;
    do {
        ch = _getch();
        if (ch == ESC_KEY) {
            buffer[0] = ESC_KEY;
            break;
        } else if (ch == ENTER_KEY) {
            ch = '\0';
            buffer[charCount] = ch;
        } else if (ch == BACKSPACE_KEY) {
            if (charCount > 0) {
                buffer[--charCount] = '\0';
                printf("\b \b");
            }
        } else if (charCount < bufferLen) {
            buffer[charCount++] = ch;
            printf("%c", HIDDEN_CHAR);
        }
    } while (ch != '\0');

    printf("\n");
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

    do {
        // make sure to clear pw1 and pw2 between iterations
        mk_clearMemory(pw1, MASTER_PASSWORD_MAX_LEN);
        mk_clearMemory(pw2, MASTER_PASSWORD_MAX_LEN);

        inputLineSecretly(prompt1, pw1, MASTER_PASSWORD_MAX_LEN - 1); // -1 for '/0'
        if (pw1[0] == ESC_KEY) {
            printf(cancelledMsg);
            exit(0);
        }
        if (strlen(pw1) < MASTER_PASSWORD_MIN_LEN) {
            printf(underLimitError);
            continue;
        }

        inputLineSecretly(prompt2, pw2, MASTER_PASSWORD_MAX_LEN - 1);
        if (pw2[0] == ESC_KEY) {
            printf(resetMsg);
            continue;
        }
        if (strcmp(pw1, pw2) != 0) {
            printf(matchingError);
            continue;
        }

        break;
    } while (1);

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

void whileViewing() {
    // show all account names by checking file names in data dir
}

AccountKVP *addKvp() {
    const char nameError[] = "(Name truncated to %s)\n";

    char newKey[ACCOUNT_MAX_KEY_LEN]; 
    if (inputLine("Key: ", newKey, ACCOUNT_MAX_KEY_LEN)) {
        printf(nameError, newKey);
    }

    if (strlen(newKey) == 0) {
        return NULL;
    }

    size_t bytes = MAX_USER_INPUT_LEN;
    char *newValue = NULL;
    while (!newValue) {
        if (bytes < ACCOUNT_MAX_KEY_LEN) {
            printf(OUT_OF_MEMORY);
            return NULL;
        }
        newValue = malloc(bytes);
        bytes /= 4;
    }

    if (inputLine("Value: ", newValue, MAX_USER_INPUT_LEN)) {
        printf("Input exceeded %lld bytes. Value truncated.\n", bytes);
    }
    bytes = strlen(newValue) + 1;
    
    AccountKVP *newKvp = malloc(sizeof(*newKvp));
    memcpy(newKvp->key, newKey, strlen(newKey) + 1);
    newKvp->value = malloc(bytes);
    if (!newKvp->value) {
        printf(OUT_OF_MEMORY);
        free(newValue);
        free(newKvp);
        return NULL;
    }
    memcpy(newKvp->value, newValue, bytes);
    newKvp->next = NULL;

    free(newValue);
    return newKvp;
}

void whileAdding(char *pw) {
    // remember to free later
    Account *account = NULL;
    unsigned char *ciphertxt = NULL;

    account = calloc(1, sizeof(*account));
    if (!account) {
        printf(OUT_OF_MEMORY);
        return;
    }

    const char nameError[] = "(Name truncated to %s)\n";
    const char oauthMsg[] = "* Leave the Email field empty to specify an OAuth provider instead.\n";
    const char emailError[] = "Email addresses cannot be longer than 254 characters. Please try again.\n";
    const char pwError[] = "Passwords cannot be longer than %d characters. Please try again.\n";
    const char additionalInfoMsg[] = "Now you can specify additional account information, saved in the format \"<Key>: <Value>\".\n";
    const char emptyKeyMsg[] = "When you are done, leave the Key field empty to exit.\n";
    const char encryptionError[] = "An error occurred while encrypting the account information. Please try again later.\n";
    const char allDone[] = "All account information was saved successfully.\n";

    if (inputLine("Account Name: ", account->name, ACCOUNT_MAX_NAME_LEN)) {
        printf(nameError, account->name);
    }

    // get account email or leave empty for oauth
    printf(oauthMsg);
    while (inputLine("Email: ", account->email, ACCOUNT_MAX_EMAIL_LEN)) {
        printf(emailError);
    }

    if (strlen(account->email) == 0) {
        // no email/password, just oauth provider
        if (inputLine("Name of OAuth Provider: ", account->oauthProvider, ACCOUNT_MAX_NAME_LEN)) {
            printf(nameError, account->oauthProvider);
        }
    } else {
        while (inputLine("Password: ", account->password, ACCOUNT_MAX_PW_LEN)) {
            printf(pwError, ACCOUNT_MAX_PW_LEN);
        }
    }

    printf(additionalInfoMsg);
    printf(emptyKeyMsg);
    
    AccountKVP *additionalInfo = addKvp();
    account->additionalInfo = additionalInfo;
    while (additionalInfo != NULL) {
        additionalInfo->next = addKvp();
        additionalInfo = additionalInfo->next;
    }

    char *serializedAccount = serialize(account);
    char fileName[ACCOUNT_MAX_NAME_LEN + 4];
    getAccountFileName(account, fileName);

    unsigned long ciphertxtLen = 0;
    int success = encryptText(pw, serializedAccount, &ciphertxt, &ciphertxtLen);
    if (!success) {
        printf(encryptionError);
    } else {
        if (!mk_write((BYTE *)serializedAccount, strlen(serializedAccount), fileName)) {
            printf(encryptionError);
        } else {
            printf(allDone);
        }
    }

    destroy(account);
    free(ciphertxt);
}

void whileSyncing() {

}

void whileUnlocked(char *pw) {
    const char *options = "1. View accounts\n2. Add new account\n3. Sync\n";
    char choice;
    do {
        printf("Options:\n");
        choice = inputChoice(options);
        if (choice == '1') {
            printf("Viewing accounts...\n");
            whileViewing();
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
    char pw[MASTER_PASSWORD_MAX_LEN];

    const char prompt[] = "Enter the password: ";
    const char wrongPwError[] = "Incorrect password. Please try again.\n";
    const char rightPw[] = "Password OK. Unlocking password manager.\n";
    const char cancelled[] = "Escape key pressed. Exiting program.\n";

    while (1) {
        inputLineSecretly(prompt, pw, MASTER_PASSWORD_MAX_LEN);
        if (pw[0] == ESC_KEY) {
            printf(cancelled);
            mk_clearMemory(pw, MASTER_PASSWORD_MAX_LEN);
            return;
        }

        int authorized = testPassword(pw);
        if (!authorized) {
            printf(wrongPwError);
            mk_clearMemory(pw, MASTER_PASSWORD_MAX_LEN);
        } else {
            printf(rightPw);
            whileUnlocked(pw);
        }
    }
}

int main() {
    outputIntro();
    
    int passwordExists = mk_exists(HASHED_PASSWORD_FILE);
    if (!passwordExists) {
        int success = setup();
        if (!success) {
            exit(1);
        }
    }

    whileLocked();
    
    return 0;
}

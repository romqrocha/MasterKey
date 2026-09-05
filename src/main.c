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

void outputIntro() {
    printf("Program initialized.\n");
}

char inputChoice(const char *prompt) {
    printf(prompt);
    
    char choice;
    // duuuude i wanted to code this for both windows and unix at first but man it's way too time consuming
    #ifdef _WIN32
        printf("Use your keyboard to choose.\n");
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
    size_t lineLength = strlen(buffer);

    if (buffer[lineLength - 1] == '\n') {
        // characters entered < limit
        buffer[lineLength - 1] = '\0';
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
            ch = '\n';
        } else if (ch == BACKSPACE_KEY) {
            if (charCount > 0) {
                buffer[--charCount] = '\0';
                printf("\b \b");
            }
        } else if (charCount < bufferLen) {
            buffer[charCount++] = ch;
            // printf("%c", HIDDEN_CHAR);
            printf("%c", ch);
        }
    } while (ch != '\n');

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
        inputLineSecretly(prompt1, pw1, MASTER_PASSWORD_MAX_LEN - 1); // -1 for '/0'
        if (pw1[0] == ESC_KEY) {
            printf(cancelledMsg);
            exit(0);
        }
        if (strlen(pw1) < 8) {
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
    } while (0);

    printf("Passwords OK. This master password will be hashed and saved.\n");
    
    int success = setPassword(pw1);
    if (!success) {
        printf("The password could not be saved.\n");
        return 0;
    }

    int sanityCheck = testPassword(pw2);
    
    SecureZeroMemory(pw1, MASTER_PASSWORD_MAX_LEN);
    SecureZeroMemory(pw2, MASTER_PASSWORD_MAX_LEN);
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

}

void whileAdding() {

}

void whileUnlocked() {
    const char *options = "1. View accounts\n2. Add new account\n3. Sync";
    char choice;
    do {
        choice = inputChoice(options);
        if (choice == '1') {
            printf("Viewing accounts...\n");
            whileViewing();
        } else if (choice == '2') {
            printf("Adding new account...\n");
            whileAdding();
        } else if (choice == '3') {
            printf("Syncing...\n");  
        } else if (choice == ESC_KEY){
            printf("Locking...\n");
        } else {
            printf("Invalid choice.\n");
        }
    } while (choice != ESC_KEY);
}

void whileLocked() {
    int authorized = 0;

    const char prompt[] = "Enter the password: ";
    char pw[ACCOUNT_MAX_PW_LEN];

    while (!authorized) {
        inputLine(prompt, pw, ACCOUNT_MAX_PW_LEN);
        authorized = testPassword(pw);
    }
    
    whileUnlocked();
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

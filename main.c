#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <conio.h>
#endif

#define ESC_KEY '\x1B'

void outputIntro() {
    printf("Program initialized.\n");
}

char inputChoice(const char *prompt) {
    printf(prompt);
    
    char choice;
    #ifdef _WIN32
        printf("Use your keyboard to choose.\n");
        choice = getch();
        printf("\n");
    #else
        printf("Enter your choice: ");
        choice = fgetc(stdin);
        while (fgetc(stdin) != '\n');
    #endif
    
    return choice;
}

char *inputLine(const char *prompt, char *buffer, int charLimit) {
    printf(prompt);
    fgets(buffer, charLimit, stdin);
    size_t lineLength = strlen(buffer);
    if (buffer[lineLength - 1] == '\n') {
        buffer[lineLength - 1] = '\0';
    } else {
        while (fgetc(stdin) != '\n');
    }
    return buffer;
}

int testKey(char *key) {
    return key[0] == '\0';
}

int unlock() {
    const char *prompt = "Enter the password: ";
    const int MAX_PW_LENGTH = 31;
    char key[MAX_PW_LENGTH + 1];

    inputLine(prompt, key, MAX_PW_LENGTH + 1);
    
    return testKey(key);
}

void whileViewing() {

}

void whileAdding() {

}

void whileUnlocked() {
    const char *options = "1. View accounts\n2. Add new account\n";
    char choice;
    do {
        choice = inputChoice(options);
        if (choice == '1') {
            printf("Viewing accounts...\n");
            whileViewing();
        } else if (choice == '2') {
            printf("Adding new account...\n");
            whileAdding();
        } else if (choice == ESC_KEY){
            printf("Locking...\n");
        } else {
            printf("Invalid choice.\n");
        }
    } while (choice != ESC_KEY);
}

void whileLocked() {
    const char *options = "1. Unlock\n2. Add new account\n";
    char choice;
    do {
        choice = inputChoice(options);
        if (choice == '1') {
            unlock() ? whileUnlocked() : printf("Key failed.\n");
        } else if (choice == '2') {
            printf("Adding new account...\n");
            whileAdding();
        } else if (choice == ESC_KEY){
            printf("Quitting...\n");
        } else {
            printf("Invalid choice.\n");
        }
    } while (choice != ESC_KEY);
}

int main() {
    outputIntro();

    whileLocked();
    
    return 0;
}

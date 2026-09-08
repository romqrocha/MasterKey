#ifdef _WIN32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// win32 dependent
#include <conio.h>
#include <windows.h>

#include "winterminal.h"

int isAllowed(int ch, char *disallowed) {
    int isAllowed = 1;

    if (strchr(disallowed, ch) != NULL) {
        isAllowed = 0;
    } else if (ch > MAX_CHAR || ch < -MAX_CHAR) {
        isAllowed = 0;
    } else if (ch >= 0 && ch <= 31) {
        isAllowed = 0;
    }

    return isAllowed;
}

char inputChoice(const char *prompt)
{
    printf(prompt);
    
    char choice;
    printf("* Choose with your keyboard.\n");
    choice = _getch();
    
    return choice;
}

int inputKey()
{   
    return _getch();
}

void inputLine(const char *prompt, char *buffer, int bufferLen, 
    char *disallowed, int isSecret) 
{
    printf(prompt);

    int ch;
    int charCount = 0;

    do {
        ch = _getch();
        if (ch == ESC_KEY) {
            buffer[0] = ESC_KEY;
            break;
        } else if (ch == BACKSPACE_KEY) {
            if (charCount > 0) {
                buffer[--charCount] = '\0';
                printf("\b \b");
            }
        } else if (ch == ENTER_KEY) {
            ch = '\0';
            buffer[charCount] = '\0';
        } else if (charCount < bufferLen - 1 && isAllowed(ch, disallowed)) { // reserve 1 for '\0'
            buffer[charCount++] = (char)ch;
            printf("%c", isSecret ? HIDDEN_CHAR : (char)ch);
        }
    } while (ch != '\0');

    printf("\n");
}

void inputLineSimply(const char *prompt, char *buffer, int bufferLen) {
    inputLine(prompt, buffer, bufferLen, "", 0);
}

#endif
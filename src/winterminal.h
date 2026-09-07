#ifndef WINTERMINAL_H
#define WINTERMINAL_H

#define ESC_KEY '\x1B'
#define ENTER_KEY '\xD'
#define BACKSPACE_KEY '\x8'
#define HIDDEN_CHAR '#'
#define MAX_CHAR 127
#define DISALLOWED_CHARS "<>:\"/\\|?*"

char inputChoice(const char *prompt);
void inputLine(const char *prompt, char *buffer, int bufferLen, char *disallowed, int isSecret);
void inputLineSimply(const char *prompt, char *buffer, int bufferLen);

#endif
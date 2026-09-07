CC = gcc
DEBUGGABLE = -ggdb3
CFLAGS = -Wall $(DEBUGGABLE)
TARGET = MasterKey.exe
OBJECTS = main.o io.o wincrypto.o account.o password.o winterminal.o
LDLIBS = -lbcrypt

D: $(TARGET)
	gdb $(TARGET)

T: $(TARGET)
	-del /Q $(OBJECTS) 2>NUL
	$(TARGET)

build: $(TARGET)
	-del /Q $(OBJECTS) 2>NUL

clean:
	-del /Q $(OBJECTS) $(TARGET) 2>NUL

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDLIBS)

main.o: src/main.c src/wincrypto.h src/io.h src/account.h src/password.h src/winterminal.h
	$(CC) $(CFLAGS) -c src/main.c -o main.o

io.o: src/io.c src/io.h
	$(CC) $(CFLAGS) -c src/io.c -o io.o

wincrypto.o: src/wincrypto.c src/wincrypto.h src/password.h src/io.h
	$(CC) $(CFLAGS) -c src/wincrypto.c -o wincrypto.o

account.o: src/account.c src/account.h src/password.h src/wincrypto.h
	$(CC) $(CFLAGS) -c src/account.c -o account.o

password.o: src/password.c src/password.h src/io.h src/wincrypto.h
	$(CC) $(CFLAGS) -c src/password.c -o password.o

winterminal.o: src/winterminal.c src/winterminal.h
	$(CC) $(CFLAGS) -c src/winterminal.c -o winterminal.o

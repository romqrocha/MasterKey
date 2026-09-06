CC = gcc
DEBUGGABLE =
CFLAGS = -Wall $(DEBUGGABLE)
TARGET = MasterKey.exe
OBJECTS = main.o io.o crypto.o account.o password.o
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

main.o: src/main.c src/crypto.h src/io.h src/account.h src/password.h
	$(CC) $(CFLAGS) -c src/main.c -o main.o

io.o: src/io.c src/io.h
	$(CC) $(CFLAGS) -c src/io.c -o io.o

crypto.o: src/crypto.c src/crypto.h src/io.h
	$(CC) $(CFLAGS) -c src/crypto.c -o crypto.o

account.o: src/account.c src/account.h src/crypto.h
	$(CC) $(CFLAGS) -c src/account.c -o account.o

password.o: src/password.c src/password.h src/crypto.h src/io.h
	$(CC) $(CFLAGS) -c src/password.c -o password.o

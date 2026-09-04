CC = gcc
CFLAGS = -Wall
TARGET = MasterKey.exe
OBJECTS = main.o io.o crypto.o
LDLIBS = -lbcrypt

T: $(TARGET)
	-del /Q $(OBJECTS) 2>NUL
	$(TARGET)

build: $(TARGET)
	-del /Q $(OBJECTS) 2>NUL

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDLIBS)

main.o: src/main.c src/crypto.h src/io.h
	$(CC) $(CFLAGS) -c src/main.c -o main.o

io.o: src/io.c src/io.h
	$(CC) $(CFLAGS) -c src/io.c -o io.o

crypto.o: src/crypto.c src/crypto.h src/io.h
	$(CC) $(CFLAGS) -c src/crypto.c -o crypto.o

CC = gcc
CFLAGS = -Wall
TARGET = MasterKey.exe
OBJECT = main.o

T: .
	$(TARGET)

.: $(OBJECT)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECT)
	-del /Q $(OBJECT) 2>NUL

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $(OBJECT)

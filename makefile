CC = gcc
CFLAGS = -g -Wall
OBJS = buf.o testcase.o disk.o fs_internal.o fs.o
TARGET = hw3

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

buf.o: buf.h buf.c
testcase.o: fs.h disk.h testcase.c
disk.o: disk.h disk.c
fs_internal.o: buf.h fs.h fs_internal.c
fs.o: buf.h fs.h fs.c

clean:
	rm -f *.o
	rm -f $(TARGET)
	rm MY_DISK

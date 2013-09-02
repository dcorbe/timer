CC=gcc
CFLAGS=-ggdb
LDFLAGS=-ggdb

timer: timer.o

clean:
	rm -rf timer *.o *.core

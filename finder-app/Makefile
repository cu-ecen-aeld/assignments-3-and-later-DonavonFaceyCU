#Cross Compiler is prefixed to default build target
CC = $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

all: default

default: writer.c
	rm -f writer
	$(CC) $(CFLAGS) writer.c -o writer

clean:
	rm -f writer.o writer

#Cross Compiler is prefixed to default build target
CC = $(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

all: default

default: aesdsocket.c
	rm -f aesdsocket
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

clean:
	rm -f aesdsocket.o aesdsocket

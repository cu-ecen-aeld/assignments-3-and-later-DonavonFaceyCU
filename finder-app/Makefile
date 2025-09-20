#Cross Compiler is prefixed to default build target
CROSS_COMPILE = 

all: default

default: writer.c
	$(CROSS_COMPILE)gcc writer.c -o writer

clean:
	rm -f writer.o writer

# Makefile
#

SH = bash
CFLAGS = -g
LDFLAGS = -lm

all: sender.c receiver.c
	gcc sender.c -o sender
	gcc receiver.c -o receiver


.PHONY: clean 

clean:
	rm -rf sender
	rm -rf receiver

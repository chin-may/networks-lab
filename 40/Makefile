# Makefile
#

SH = bash
CFLAGS = -g
LDFLAGS = -lm

all: sender receiver

sender: sender.o
	gcc -c $(CFLAGS) $< -o $@

receiver: receiver.o
	gcc -c $(CFLAGS) $< -o $@

sender.o: sender.c
	gcc -c $(CFLAGS) $< -o $@

receiver.o: receiver.c
	gcc -c $(CFLAGS) $< -o $@

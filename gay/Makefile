CC=gcc

all: libgay.a

libgay.a: dtoa.o
	ar rcs $@ $<

dtoa.o: dtoa.c Makefile
	$(CC) -c $< -o $@

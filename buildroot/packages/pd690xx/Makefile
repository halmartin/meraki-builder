CC=gcc
PWD=$(shell pwd)

all: *.c
	$(CC) -c -Wall -Werror -fpic libpd690xx.c
	$(CC) -shared -o libpd690xx.so libpd690xx.o
	$(CC) -L$(PWD) -Wall -o pd690xx pd690xx.c -lpd690xx
	#$(CC) -Os *.c -o pd690xx
clean:
	rm -rf a.out pd690xx
	rm -f *.o

CC=gcc

all: *.c
	$(CC) -I$(PWD)/package/pd690xx -I$(PWD)/package/postmerkos -Os -o status -ljson-c '$<' \
	$(PWD)/package/pd690xx/libpd690xx.c \
	$(PWD)/package/postmerkos/libpostmerkos.c
clean:
	rm -rf a.out status
	rm -f *.o
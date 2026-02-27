CC=gcc
CFLAGS=-I$(PWD)/package/pd690xx -I$(PWD)/package/postmerkos -Os
LDFLAGS=-ljson-c -lwebsockets

SRCS=main.c click_port.c click_global.c status.c websocket.c json_util.c config_file.c
LIBS=$(PWD)/package/pd690xx/libpd690xx.c $(PWD)/package/postmerkos/libpostmerkos.c

all: $(SRCS)
	$(CC) $(CFLAGS) -o configd $(SRCS) $(LIBS) $(LDFLAGS)

clean:
	rm -f configd *.o

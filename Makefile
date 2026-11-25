CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
AR      = ar
ARFLAGS = rcs

all: libmonitor.a server client

bd.o: bd.c bd.h
	$(CC) $(CFLAGS) -c bd.c -o bd.o

formato.o: formato.c formato.h bd.h
	$(CC) $(CFLAGS) -c formato.c -o formato.o

libmonitor.a: bd.o formato.o
	$(AR) $(ARFLAGS) libmonitor.a bd.o formato.o

server: server.c libmonitor.a bd.h formato.h
	$(CC) $(CFLAGS) server.c -L. -lmonitor -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f *.o libmonitor.a server client

.PHONY: all clean
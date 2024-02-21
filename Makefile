CC = gcc
CFLAGS = -Wall -Wextra -g

all: client server

client: client/main.o client/network.o
	$(CC) $(CFLAGS) -o client/client $^
	rm -f client/*.o

client/main.o: client/main.c client/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

client/network.o: client/network.c client/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

server: server/main.o server/network.o
	$(CC) $(CFLAGS) -o server/server $^
	rm -f server/*.o

server/main.o: server/main.c server/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

server/network.o: server/network.c server/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f server/server client/client client/*.o server/*.o

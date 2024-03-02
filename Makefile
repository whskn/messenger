CC = gcc
CFLAGS = -Wall -Wextra -g


all: client server


client: src/client/main.o src/client/network.o 
	$(CC) $(CFLAGS) -o client $^ 
	rm -f src/client/*.o

src/client/main.o: src/client/main.c src/client/network.h 
	$(CC) $(CFLAGS) -c -o $@ $<

src/client/network.o: src/client/network.c src/client/network.h src/flags.h 
	$(CC) $(CFLAGS) -c -o $@ $<


server: src/server/main.o src/server/network.o 
	$(CC) $(CFLAGS) -o server $^
	rm -f src/server/*.o

src/server/main.o: src/server/main.c src/server/network.h
	$(CC) $(CFLAGS) -c -o $@ $<

src/server/network.o: src/server/network.c src/server/network.h src/flags.h
	$(CC) $(CFLAGS) -c -o $@ $<


clean:
	rm -f server client src/client/*.o src/server/*.o

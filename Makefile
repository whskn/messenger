CC = gcc
CFLAGS = -Wall -Wextra -g


all: client server
	rm -f src/history/*.o


# Client
client: cli_main cli_network ui
	$(CC) $(CFLAGS) -o $@ src/client/main.o \
						  src/client/network.o \
						  src/client/ui.o
	rm -f src/client/*.o

cli_main: 
	$(CC) $(CFLAGS) -c -o src/client/main.o src/client/main.c 

ui: 
	$(CC) $(CFLAGS) -c -o src/client/ui.o src/client/ui.c 

cli_network: 
	$(CC) $(CFLAGS) -c -o src/client/network.o src/client/network.c


# Server
server: srv_main serv srv_network logger history sqlite3
	$(CC) $(CFLAGS) -o server src/server/main.o \
							  src/server/network.o \
							  src/server/serv.o \
							  src/server/logger.o \
							  src/history/history.o \
							  src/sqlite/sqlite3.o
	rm -f src/server/*.o

srv_main: 
	$(CC) $(CFLAGS) -c -o src/server/main.o src/server/main.c 

serv:
	$(CC) $(CFLAGS) -c -o src/server/serv.o src/server/serv.c

srv_network:
	$(CC) $(CFLAGS) -c -o src/server/network.o src/server/network.c

logger:
	$(CC) $(CFLAGS) -c -o src/server/logger.o src/server/logger.c


# Database 
history: 
	$(CC) $(CFLAGS) -c -o src/history/history.o src/history/history.c

sqlite3: 
	if [ ! -f src/sqlite/sqlite3.o ]; then \
		$(CC) -g -c -o src/sqlite/sqlite3.o src/sqlite/sqlite3.c; \
    fi



clean:
	rm -f server client src/client/*.o src/server/*.o src/history/*.o

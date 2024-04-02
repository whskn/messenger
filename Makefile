CC = gcc
CFLAGS = -Wall -Wextra -g
CURSES = -lncurses


all: clean sqlite3 history misc client server
	rm -f src/history/*.o src/misc/*.o 


# Client
client: sqlite_funcs cli_main cli_network render ui app
	$(CC) $(CFLAGS) -o $@ src/client/main.o \
						  src/client/network.o \
						  src/client/app.o \
						  \
						  src/client/interface/render.o \
						  src/client/interface/ui.o \
						  \
						  src/history/sqlite/sqlite3.o \
						  src/history/history.o \
						  src/history/sqlite_funcs.o \
						  \
						  src/misc/blocking_read.o \
						  src/misc/validate.o \
						  $(CURSES)
	rm -f src/client/*.o src/client/interface/*.o

cli_main: 
	$(CC) $(CFLAGS) -c -o src/client/main.o src/client/main.c 


cli_network: 
	$(CC) $(CFLAGS) -c -o src/client/network.o src/client/network.c

app:
	$(CC) $(CFLAGS) -c -o src/client/app.o src/client/app.c

# Interface
# interface: ui render
# 	ld -r -o src/client/interface/interface.o src/client/interface/ui.o \ 
# 			 								  src/client/interface/render.o

ui: 
	$(CC) $(CFLAGS) -c -o src/client/interface/ui.o src/client/interface/ui.c 

render: 
	$(CC) $(CFLAGS) -c -o src/client/interface/render.o \
						  src/client/interface/render.c 


# Server
server: srv_main serv srv_network logger
	$(CC) $(CFLAGS) -o server src/server/main.o \
							  src/server/network.o \
							  src/server/serv.o \
							  src/server/logger.o \
							  \
							  src/history/history.o \
							  src/history/sqlite/sqlite3.o \
							  src/history/sqlite_funcs.o \
							  \
							  src/misc/blocking_read.o \
							  src/misc/validate.o
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
	$(CC) $(CFLAGS) -c -o src/history/history.o \
						  src/history/history.c

sqlite_funcs:
	$(CC) $(CFLAGS) -c -o src/history/sqlite_funcs.o src/history/sqlite_funcs.c

sqlite3:
	if [ ! -f src/history/sqlite/sqlite3.o ]; then \
		$(CC) -g -c -o src/history/sqlite/sqlite3.o \
					   src/history/sqlite/sqlite3.c; \
	fi

# misc
misc:
	$(CC) $(CFLAGS) -c -o src/misc/blocking_read.o src/misc/blocking_read.c
	$(CC) $(CFLAGS) -c -o src/misc/validate.o src/misc/validate.c


clean:
	rm -f server client src/client/*.o src/server/*.o src/history/*.o \
		  src/misc/*.o src/client/interface/*.o

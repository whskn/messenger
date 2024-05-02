CC = gcc
CFLAGS = -Wall -Wextra -g
CURSES = -lncurses
SQLITE = -lsqlite3
ICU = -licuuc


all: misc server client test_env
	rm -f src/misc/*.o server client

test_env:
	mkdir -p client1 client2 client3 client4 serv
	rm -f src/misc/*.o
	cp client client1/client
	cp client client2/client
	cp client client3/client
	cp client client4/client
	cp server serv/server
	rm -f client server
	


# Client
client: cli_main cli_network render ui app handlers cli_db
	$(CC) $(CFLAGS) -o $@ src/client/main.o \
						  src/client/network.o \
						  src/client/app.o \
						  src/client/render.o \
						  src/client/ui.o \
						  src/client/handlers.o \
						  src/client/db.o \
						  src/misc/validate.o \
						  $(CURSES) $(SQLITE) $(ICU)
	rm -f src/client/*.o

cli_main: 
	$(CC) $(CFLAGS) -c -o src/client/main.o src/client/main.c 


cli_network: 
	$(CC) $(CFLAGS) -c -o src/client/network.o src/client/network.c

app:
	$(CC) $(CFLAGS) -c -o src/client/app.o src/client/app.c

handlers:
	$(CC) $(CFLAGS) -c -o src/client/handlers.o src/client/handlers.c 

ui: 
	$(CC) $(CFLAGS) -c -o src/client/ui.o src/client/ui.c 

render: 
	$(CC) $(CFLAGS) -c -o src/client/render.o src/client/render.c 

cli_db:
	$(CC) $(CFLAGS) -c -o src/client/db.o src/client/db.c



# Server
server: srv_main serv_app srv_network logger srv_db
	$(CC) $(CFLAGS) -o $@ src/server/main.o \
							  src/server/network.o \
							  src/server/serv.o \
							  src/server/logger.o \
							  src/server/db.o \
							  src/misc/validate.o \
							  $(SQLITE) $(ICU)
	rm -f src/server/*.o

srv_main: 
	$(CC) $(CFLAGS) -c -o src/server/main.o src/server/main.c 

serv_app:
	$(CC) $(CFLAGS) -c -o src/server/serv.o src/server/serv.c

srv_network:
	$(CC) $(CFLAGS) -c -o src/server/network.o src/server/network.c

logger:
	$(CC) $(CFLAGS) -c -o src/server/logger.o src/server/logger.c

srv_db:
	$(CC) $(CFLAGS) -c -o src/server/db.o src/server/db.c

	
	
	

sqlite3:
	if [ ! -f src/sqlite3/sqlite3.o ]; then \
		$(CC) -g -c -o src/sqlite3/sqlite3.o src/misc/sqlite3.c; \
	fi



# misc
misc:
	$(CC) $(CFLAGS) -c -o src/misc/validate.o src/misc/validate.c

	

purge: clean
	rm -r -f serv client1 client2 client3 client4

clean:
	rm -f client server src/client/*.o src/server/*.o \
		  src/misc/*.o src/client/*.o \
		  client1/client client2/client serv/server
		
rmdbs:
	rm -f client1/database.db client2/database.db serv/database.db
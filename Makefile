CC = gcc
CFLAGS = -Wall -Wextra -g
CURSES = -lncurses
SQLITE = -lsqlite3
ICU = -licuuc


all: bin/server bin/client


# Client
bin/client: bin_dir cli_misc cli_main cli_network render ui app handlers cli_db
	$(CC) $(CFLAGS) -o $@ src/client/main.o \
						  src/client/network.o \
						  src/client/app.o \
						  src/client/render.o \
						  src/client/ui.o \
						  src/client/handlers.o \
						  src/client/db.o \
						  src/misc/validate.o \
						  $(CURSES) $(SQLITE) $(ICU)
	rm -f src/client/*.o src/misc/*.o

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

cli_misc:
	$(CC) $(CFLAGS) -c -o src/misc/validate.o src/misc/validate.c



# Server
bin/server: bin_dir srv_misc srv_main ser_app srv_network logger srv_db
	$(CC) $(CFLAGS) -o $@ src/server/main.o \
							  src/server/network.o \
							  src/server/serv.o \
							  src/server/logger.o \
							  src/server/db.o \
							  src/misc/validate.o \
							  $(SQLITE) $(ICU)
	rm -f src/server/*.o src/misc/*.o

srv_main: 
	$(CC) $(CFLAGS) -c -o src/server/main.o src/server/main.c 

ser_app:
	$(CC) $(CFLAGS) -c -o src/server/serv.o src/server/serv.c

srv_network:
	$(CC) $(CFLAGS) -c -o src/server/network.o src/server/network.c

logger:
	$(CC) $(CFLAGS) -c -o src/server/logger.o src/server/logger.c

srv_db:
	$(CC) $(CFLAGS) -c -o src/server/db.o src/server/db.c

srv_misc:
	$(CC) $(CFLAGS) -c -o src/misc/validate.o src/misc/validate.c


bin_dir:
	mkdir -p bin

	

clean:
	rm -f src/client/*.o src/server/*.o src/misc/*.o
	rm -fr bin
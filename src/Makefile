all: server client

server: server.c msg.h
	$(CC) server.c -o $@ -Wall -std=c99 -pedantic

client: client.c msg.h
	$(CC) client.c -o $@ -Wall -std=c99 -pedantic

clean:
	rm -f client server


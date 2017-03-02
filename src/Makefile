
FLAGS= -I../deps/sds/ 
FLAGS+= -I../deps/adlist/
FLAGS+= -ladlist

all: server client reg

server: server.c msg.h
	$(CC) -g server.c -o $@ -Wall -std=c99 -pedantic $(FLAGS)

client: client.c msg.h
	$(CC) -g client.c -o $@ -Wall -std=c99 -pedantic

reg: reg.c
	$(CC) -g reg.c -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm -f client server reg


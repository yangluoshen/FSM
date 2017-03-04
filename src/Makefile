
FLAGS= -I../deps/sds/ -I../deps/adlist/ -I../deps/clog/
FLAGS+= -L../deps/adlist/
FLAGS+= -ladlist

all: server client reg

server: server.c fsm.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

client: client.c fsm.c
	$(CC) -g $^ -o $@ -Wall -std=c99 -pedantic

reg: reg.c
	$(CC) -g reg.c -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm -f client server reg


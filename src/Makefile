
FLAGS= -I../deps/sds/ -I../deps/adlist/ -I../deps/clog/
FLAGS+= -L../deps/adlist/
FLAGS+= -ladlist

all: server client_base reg

server: server.c fsm.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

client_base: client_base.c fsm.c
	$(CC) -g $^ -o $@ -Wall -std=c99 -pedantic

reg: reg.c
	$(CC) -g reg.c -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm -f client server reg


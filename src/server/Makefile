
FLAGS= -I../../deps/sds/ -I../../deps/adlist/ -I../../deps/clog/
FLAGS+= -I../
FLAGS+= -L../../deps/adlist/
FLAGS+= -ladlist

all: server 

server: server.c ../fsm.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm -f server


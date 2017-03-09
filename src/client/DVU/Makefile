
FLAGS= -I../../ -I../ 
FLAGS+= -I../../../deps/adlist/
FLAGS+= -L../../../deps/adlist/
FLAGS+= -ladlist

dvu: dvu.c dvu_base.c ../../fsm.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm dvu


FLAGS= -I../../ -I../ 
FLAGS+= -I../../../deps/adlist/
FLAGS+= -L../../../deps/adlist/
FLAGS+= -ladlist
FLAGS+= -DDVU_MDL

dvu: dvu.c client_config.c ../client_base.c ../../fsm.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm dvu

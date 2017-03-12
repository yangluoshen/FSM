
FLAGS= -I../../ -I../ 
FLAGS+= -I../../../deps/adlist/
FLAGS+= -L../../../deps/adlist/
FLAGS+= -ladlist
FLAGS+= -DYAU_MDL

yau: client_config.c ../client_base.c ../../fsm.c yau.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm yau

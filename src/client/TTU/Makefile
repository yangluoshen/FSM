ROOT_DIR = ../../..
FLAGS= -I$(ROOT_DIR)/src/  -I$(ROOT_DIR)/src/client/ 
FLAGS+= -I$(ROOT_DIR)/deps/adlist/ -I$(ROOT_DIR)/deps/clog/
FLAGS+= -L$(ROOT_DIR)/deps/adlist/
FLAGS+= -ladlist
FLAGS+= -DTTU_MDL

SRC=
SRC+= client_config.c $(ROOT_DIR)/src/debug.c $(ROOT_DIR)/src/fsm_base.c $(ROOT_DIR)/src/client/client_base.c ttu.c

ttu: $(SRC)
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm ttu

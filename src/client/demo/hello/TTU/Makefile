ROOT_DIR = ../../../../../
FLAGS= -I$(ROOT_DIR)/src/  -I$(ROOT_DIR)/src/client/ -I../
FLAGS+= -I$(ROOT_DIR)/deps/adlist/ -I$(ROOT_DIR)/deps/clog/
FLAGS+= -L$(ROOT_DIR)/deps/adlist/
FLAGS+= -ladlist
FLAGS+= -DTTU_MDL

BASE_SRC= $(ROOT_DIR)/src/fsm_base.c $(ROOT_DIR)/src/client/fsm.c  $(ROOT_DIR)/src/client/main.c 
DEPS_SRC= $(ROOT_DIR)/src/fdict.c $(ROOT_DIR)/src/flist.c $(ROOT_DIR)/src/debug.c
SRC=
SRC+= $(BASE_SRC) $(DEPS_SRC) client_config.c $(ROOT_DIR)/src/client/demo/hello/cachefsm.c  ttu.c

ttu: $(SRC)
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm ttu

ROOT_DIR = ../../../../../
FLAGS= -I$(ROOT_DIR)/src/  -I$(ROOT_DIR)/src/client/ -I../
FLAGS+= -I$(ROOT_DIR)/deps/adlist/ -I$(ROOT_DIR)/deps/clog/
FLAGS+= -L$(ROOT_DIR)/deps/adlist/
FLAGS+= -ladlist
FLAGS+= -DYAU_MDL

BASE_SRC= $(ROOT_DIR)/src/client/fsm.c  $(ROOT_DIR)/src/client/main.c 
DEPS_SRC= $(ROOT_DIR)/src/dict.c $(ROOT_DIR)/src/debug.c $(ROOT_DIR)/src/client/ae.c $(ROOT_DIR)/src/client/anet.c
SRC=
SRC+= $(BASE_SRC) $(DEPS_SRC)  client_config.c  yau.c

yau: $(SRC)
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm yau


FLAGS= -I../

yau: client_base.c fsm.c yau.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm yau

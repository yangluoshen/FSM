
FLAGS= -I../

dvu: client_base.c fsm.c dvu.c
	$(CC) -g $^  -o $@ -Wall -std=c99 -pedantic $(FLAGS)

clean:
	rm dvu

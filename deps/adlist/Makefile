
libadlist.a: adlist.h adlist.c
	$(CC) -c adlist.c -Wall -std=c99 -pedantic -O2 -o adlist.o
	ar rcs $@ adlist.o
	rm adlist.o

clean:
	rm -f libadlist.a adlist.o

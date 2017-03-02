
libadlist.so: adlist.h adlist.c
	$(CC) -fPIC -shared adlist.c -Wall -std=c99 -pedantic -O2 -o $@

clean:
	rm -f libadlist.so

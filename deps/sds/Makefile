
sds: sds.c sds.h
	$(CC) -c sds.c -Wall -std=c99 -pedantic -O2 -o sds.o
	ar rcs libsds.a sds.o
	rm -f sds.o

sds-test: sds.c sds.h
	$(CC) -o sds-test sds.c -Wall -std=c99 -pedantic -O2 -DSDS_TEST_MAIN
	@echo ">>> Type ./sds-test to run the sds.c unit tests."

clean:
	rm -f sds-test libsds.a sds.o

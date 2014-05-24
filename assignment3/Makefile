CC =	gcc

CFLAGS = -Wall -W -Wstrict-prototypes -O2 -ansi -g -DDebug
DFLAGS = -D_POSIX_C_SOURCE=200809L

LIBS = -lm

all: isam_bench isam_test

isam_bench:	isam_bench.o isam.o index.o mt19937ar.o
	$(CC) $(CFLAGS) -o isam_bench isam_bench.o isam.o index.o mt19937ar.o $(LIBS)

isam_test:	isam_test.o isam.o index.o
	$(CC) $(CFLAGS) -o isam_test isam_test.o isam.o index.o $(LIBS)

isam_bench.o:	isam_bench.c isam.h
	$(CC) $(CFLAGS) -c isam_bench.c

isam_test.o:	isam_test.c isam.h
	$(CC) $(CFLAGS) -c isam_test.c

isam.o:	isam.c isam.h index.h
	$(CC) $(CFLAGS) $(DFLAGS) -c isam.c

index.o:	index.c index.h
	$(CC) $(CFLAGS) -c index.c

mt19937ar.o:	mt19937ar.c mt19937.h
	$(CC) $(CFLAGS) -c mt19937ar.c

clean:
	rm -f *.o *~ isam_bench isam_test core *.isam

bench: isam_bench
	rm -f klant.isam
	./isam_bench namen initialen.txt titels.txt

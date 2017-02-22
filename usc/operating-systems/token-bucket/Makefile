warmup2: tokenbucket.o my402list.o
	gcc -o warmup2 -g tokenbucket.o my402list.o -pthread -lm -lrt

tokenbucket.o: tokenbucket.c tokenbucket.h my402list.h
	gcc -g -c -Wall tokenbucket.c -D_POSIX_PTHREAD_SEMANTICS

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o warmup2

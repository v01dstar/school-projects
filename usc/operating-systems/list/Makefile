warmup1: my402trans.o my402list.o
	gcc -o warmup1 -g my402trans.o my402list.o

my402trans.o: my402trans.c my402trans.h my402list.h
	gcc -g -c -Wall my402trans.c

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c

clean:
	rm -f *.o warmup1

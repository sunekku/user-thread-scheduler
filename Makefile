all: main

main: main.o libthread.a
        gcc -lm -o main main.o -L. -lthread

main.o: main.c
        gcc -O -c main.c main.h

scheduler.o: scheduler.c scheduler.h thread.h
        gcc -O -c scheduler.c

thread.o: thread.c scheduler.h thread.h
        gcc -O -c thread.c

libthread.a: thread.o scheduler.o
        ar rcs libthread.a thread.o scheduler.o

libs: libmylib.a

clean:
        rm -f main *.o *.a *.gch
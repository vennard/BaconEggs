FLAGS_L = -c -fpic
FLAGS_S = -shared -o 
FLAGS_R = -lmem -L. -o
FLAG_SHOW_ERROR = -Wall -Werror

all: mem.o libmem.so myprog

mem.o: mem.c $(HEADER)
	gcc $(FLAGS_L) mem.c $(FLAG_SHOW_ERROR) 

libmem.so: mem.o
	gcc $(FLAGS_S) libmem.so mem.o

myprog: mem_tester.c
	gcc $(FLAGS_R) myprog mem_tester.c $(FLAG_SHOW_ERROR)

clean:
	-rm -f mem.o myprog libmem.so

run: myprog
	./myprog

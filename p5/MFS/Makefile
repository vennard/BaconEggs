FLAGS_L = -c -fpic
FLAGS_S = -shared -o 
FLAGS_R = -lmfs -L. -o
#FLAG_SHOW_ERROR = -Wall -Werror

HEADERS = mfs.h udp.h

all: mfs.o libmfs.so myprog

mfs.o: mfs.c $(HEADER)
	gcc $(FLAGS_L) mfs.c $(FLAG_SHOW_ERROR)

libmfs.so: mfs.o
	gcc $(FLAGS_S) libmfs.so mfs.o 

myprog: test.c
	gcc $(FLAGS_R) myprog test.c $(FLAG_SHOW_ERROR)

clean:
	-rm -f mfs.o myprog libmfs.so

run: myprog
	./myprog

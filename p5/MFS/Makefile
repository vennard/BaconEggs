FLAGS_L = -c -fpic
FLAGS_S = -shared -o 
FLAGS_R = -lmem -L. -o
#FLAG_SHOW_ERROR = -Wall -Werror

HEADERS = mfs.h udp.h

all: mfs.o libmfs.so client

mfs.o: mfs.c $(HEADERS)
	gcc $(FLAGS_L) mfs.c $(FLAG_SHOW_ERROR)

libmfs.so: mfs.o udp.o
	gcc $(FLAGS_S) libmfs.so mfs.o udp.o 

client: client.c
	gcc $(FLAGS_R) client client.c $(FLAG_SHOW_ERROR)

clean:
	-rm -f mfs.o client libmfs.so udp.o

run: client
	./client

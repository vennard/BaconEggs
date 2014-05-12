FLAGS_L = -c -fpic
FLAGS_S = -shared -o 
FLAGS_R = -lmfs -L. -o
#FLAG_SHOW_ERROR = -Wall -Werror

HEADERS = mfs.h udp.h

all: server_utils.o mfs.o udp.o libmfs.so client server

server_utils.o: server_utils.c $(HEADERS)
	gcc $(FLAGS_L) server_utils.c $(FLAG_SHOW_ERROR)

mfs.o: mfs.c $(HEADERS)
	gcc $(FLAGS_L) mfs.c $(FLAG_SHOW_ERROR)

udp.o: udp.c $(HEADERS)
	gcc $(FLAGS_L) udp.c $(FLAG_SHOW_ERROR)

libmfs.so: mfs.o 
	gcc $(FLAGS_S) libmfs.so mfs.o  

client: test.c 
	gcc $(FLAGS_R) client test.c udp.o $(FLAG_SHOW_ERROR)

server: server.c
	gcc $(FLAGS_R) server server.c udp.o server_utils.o $(FLAG_SHOW_ERROR)

clean:
	-rm -f mfs.o server_utils.o client server server.o libmfs.so udp.o

run: client
	./client

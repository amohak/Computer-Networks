CC=g++
CFLAGS= -g -Wall -Werror

all: proxy

proxy: proxy.cpp
	$(CC) $(CFLAGS) -o proxy_parse.o -c proxy_parse.c
	$(CC) $(CFLAGS) -o book_keeping.o -c book_keeping.cpp
	$(CC) $(CFLAGS) -o proxy.o -c proxy.cpp
	$(CC) $(CFLAGS) -o proxy proxy_parse.o proxy.o book_keeping.o

clean:
	rm -f proxy *.o
	rm -f proxy

tar:
	tar -cvzf project2.tar proxy.cpp README Makefile proxy_parse.c proxy_parse.h book_keeping.cpp book_keeping.h Project_2_Report.pdf

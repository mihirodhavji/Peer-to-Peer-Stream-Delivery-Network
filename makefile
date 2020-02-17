CC=gcc

CFLAGS=  -Wall -pedantic -g

ALL: IAMROOT clean

IAMROOT: main.o bestpops.o con_udp.o init.o tcp_con.o
	$(CC) $(CFLAGS) -o iamroot main.o bestpops.o con_udp.o init.o tcp_con.o

bestopops.o: bestpops.c bestpops.h
	$(CC) -c $(CFLAGS) bestpops.c 

con_udp.o:	con_udp.c con_udp.h
	$(CC) -c $(CFLAGS) con_udp.c 

init.o:	init.c init.h
	$(CC) -c $(CFLAGS) init.c

tcp_con.o: tcp_con.c tcp_con.h
	$(CC) -c $(CFLAGS) tcp_con.c 

main.o:	main.c tcp_con.h init.h bestpops.h con_udp.h
	$(CC) -c $(CFLAGS) main.c 

clean::
	rm -f *.o core a.out

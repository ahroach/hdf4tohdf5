.c.o:
	$(CC) -c $(CFLAGS) $*.c

CC = gcc -std=c99
CFLAGS = -Wall -I/usr/local/include -I/usr/include/hdf

LDFLAGS =
LIBS = -lmfhdf -ldf -lhdf5

PROGS = hdf4tohdf5

OBJS = main.o

compile : $(OBJS)
	$(CC) -o hdf4tohdf5 $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(PROGS) gmon.{out,sum} *.o *.s *~ core core.[0-9]*[0-9]

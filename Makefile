# vim:ts=8

CC	= cc
CFLAGS	= -O2 -Wall -Wunused -Wmissing-prototypes -Wstrict-prototypes
CFLAGS += -g

PREFIX	 = /usr/local
BINDIR	 = $(DESTDIR)$(PREFIX)/bin

INSTALL_PROGRAM = install -s

PROG	= shutdownd
OBJS	= shutdownd.o
MAN	= shutdownd.1

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(OBJS) $(LDPATH) $(LIBS) -o $@

$(OBJS): *.o: *.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

install: all
	$(INSTALL_PROGRAM) $(PROG) $(BINDIR)

clean:
	rm -f $(PROG) $(OBJS) 

.PHONY: all install clean

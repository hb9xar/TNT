INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
MKDIR = mkdir -p

prefix = /usr/local/tnt
exec_prefix = ${prefix}
includedir = ${prefix}/include
datadir = ${prefix}/share
infodir = ${prefix}/share/info
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
localstatedir = ${prefix}/var
mandir = ${prefix}/share/man
sbindir = ${exec_prefix}/sbin
sysconfdir = ${prefix}/etc
srcdir = .

bindir = ${exec_prefix}/bin

CC		= gcc
CPP		= $(CC) -E
CFLAGS		= -funsigned-char -I.. -I../include -g -O2 -DTNT_LINUX
DEFS		= -DHAVE_CONFIG_H
LD		= $(CC)
LDFLAGS		= 
LIBS		=  -lncurses

HDRS	      = tnt.h \
		window.h \
		init.h \
		config.h \
		shell.h \
		keys.h \
		macro.h \
		connect.h \
		iface.h \
		ifacedef.h \
		xmon.h \
		monbox.h \
		boxlist.h \
		priv.h \
		comp.h \
		bcastadd.h \
		bcast.h \
		pastrix.h \
		dpglobal.h \
		boxglobl.h \
		p2cdef.h \
		ax25k.h

OBJS	      = main.o \
		serial.o \
		keyboard.o \
		display.o \
		window.o \
		file.o \
		codconv.o \
		init.o \
		remote.o \
		cookie.o \
		crc.o \
		shell.o \
		macro.o \
		connect.o \
		log.o \
		iface.o \
		xmon.o \
		monbox.o \
		boxlist.o \
		priv.o \
		comp.o \
		bcastadd.o \
		bcast.o \
		huffman.o \
		yapp.o \
		ax25k.o \
		ax25dump.o

SRCS	      = main.c \
		serial.c \
		keyboard.c \
		display.c \
		window.c \
		file.c \
		codconv.c \
		init.c \
		remote.c \
		cookie.c \
		crc.c \
		shell.c \
		macro.c \
		connect.c \
		log.c \
		iface.c \
		xmon.c \
		monbox.c \
		boxlist.c \
		priv.c \
		comp.c \
		bcastadd.c \
		bcast.c \
		huffman.c \
		yapp.c \
		ax25k.c \
		ax25dump.c

all:		tnt tntc

tnt:		$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS) $(DEFS) -o tnt;

tntc:		tntc.o tnt.h
		$(CC) $(CFLAGS) tntc.o $(LDFLAGS) $(LIBS) $(DEFS) -o tntc;

install:
		$(MKDIR) ${sbindir}; \
		$(INSTALL_PROGRAM) tnt ${sbindir}; \
		$(INSTALL_PROGRAM) tntc ${sbindir};

clean:
		rm -f *.orig *.o *~ .*~ tnt tntc 

dep:
		$(CPP) -M $(CFLAGS) $(SRCS) > .depend

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

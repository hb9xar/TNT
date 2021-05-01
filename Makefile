# uncomment only one compiling options line
# compiling for Linux with optimizing
#CFLAGS		= -m32 -funsigned-char -O2 -DTNT_LINUX
# try 64bit build
CFLAGS		= -funsigned-char -O2 -DTNT_LINUX
# compiling for Linux with debugging
#CFLAGS		= -funsigned-char -ggdb -DTNT_LINUX
# compiling for ISC with optimizing
#CFLAGS		= -funsigned-char -O2 -D_POSIX_SOURCE -DTNT_ISC
# compiling for NetBSD with optimizing (not verified)
#CFLAGS		= -funsigned-char -O2 -DTNT_NETBSD

# uncomment only only additional library line
# Libraries for Linux
LIBS		= -ltermcap
# Libraries for ISC
#LIBS		= -lcposix -linet -ltermcap

CC		= gcc
CPP		= $(CC) -E
LD		= $(CC)
#LDFLAGS		=  -m32
# try 64bit build
LDFLAGS		=  
DEFS		=

HDRS		= tnt.h \
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
		  p2cdef.h

OBJS		= main.o \
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
		  yapp.o

SRCS		= main.c \
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
		  yapp.c

OTHERS          = tntc.c \
		  tnt.ini.smpl \
		  tntc.ini.smpl \
                  tnt.up.smpl \
                  tnt.dwn.smpl \
                  tntrem.inf.smpl \
                  names.tnt.smpl \
                  news.tnt.smpl \
		  netpass.tnt.smpl \
                  fkeys.tnt.smpl \
		  pw.tnt.smpl \
                  boxender.tnt.smpl \
		  sys.tnt.smpl \
		  norem.tnt.smpl \
		  flchk.tnt.smpl \
		  notown.tnt.smpl \
		  routes.tnt.smpl \
		  f6fbb.box.smpl \
		  autostrt.tnt.smpl \
		  extrem.tnt.smpl \
                  ctext.tnt \
                  tntrem.hlp \
		  doc/FILES \
                  doc/tnt.doc \
		  doc/tntdoc.texinfo \
		  doc/tntdoc.html \
                  doc/license \
                  doc/tnt.doc.francais \
		  doc/iface.doc \
		  tnt.hlp \
                  xtnt \
		  xtntc \
                  Makefile \
                  README

all:		tnt tntc

tgz:
		@echo Creating tntsrc.tgz
		@tar -cvzf tntsrc.tgz $(SRCS) $(HDRS) $(OTHERS)

tnt:		$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o tnt;

tntc:		tntc.o tnt.h
		$(CC) $(CFLAGS) tntc.o $(LIBS) -o tntc;
		
clean:
	rm -f   *.o *~ .*~ tnt tntc .depend

dep:
	$(CPP) -M $(CFLAGS) $(SRCS) > .depend

#
# include a dependency file if one exists
#
ifeq (.depend,$(wildcard .depend))
include .depend
endif

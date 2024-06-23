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

CC              = gcc
CPP             = $(CC) -E
CFLAGS          = -g -O2 -DTNT_LINUX -funsigned-char -I..
LD              = $(CC)
LDFLAGS         = 
LIBS            =  -lncurses 

SRCS   = tntrun.h

all:

clean:
	$(RM) $(BIN) *.o *.orig *.bak *.core .depend TAGS

install:
	$(MKDIR)  ${includedir}; 
	@for i in $(SRCS); \
	do \
		$(INSTALL_DATA) $$i ${includedir}; \
	done

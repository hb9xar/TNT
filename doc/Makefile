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

DOCS   = CHANGES COPYING FILES ax25krnl.doc iface.doc tnt.doc \
         tnt.doc.francais tntdoc.html tntdoc.texinfo \
	 Readme Readme1 Readme2 Readme3 Readme4 Readme5 Readme6 Readme7

all:

clean:

install:
	$(MKDIR)  ${prefix}/doc;
	@for i in $(DOCS); \
	do \
		$(INSTALL_DATA) $$i ${prefix}/doc; \
	done

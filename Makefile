INSTALL	= /usr/bin/install -c
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

TARGET =        src .others include examples doc
SUBDIR  =       7plus abin bcast/newmail bcast/save bin ctext \
                doc down macro newmail remote tntusers up yapp sounds
all:
	$(MAKE) -C src all
	$(MAKE) -C .others all
	$(MAKE) -C include all
	$(MAKE) -C examples all
	$(MAKE) -C doc all

install:
	$(MAKE) -C src install
	$(MAKE) -C .others install
	$(MAKE) -C include install
	$(MAKE) -C examples install
	$(MAKE) -C doc install
	@for i in $(SUBDIR); \
	do \
		$(MKDIR) ${prefix}/$$i; \
	done;
	$(INSTALL) -m 544 conf/tnt_setup ${sbindir}
	${sbindir}/tnt_setup ${prefix};

clean:
	@for i in $(TARGET); \
	do \
		(cd $$i; $(MAKE) clean;)\
	done;

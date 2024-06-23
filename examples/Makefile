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

EXAMPLES = autostrt.tnt boxender.tnt ctext.tnt extrem.tnt f6fbb.box \
         fkeys.tnt flchk.tnt names.tnt netpass.tnt news.tnt norem.tnt \
         notown.tnt pw.tnt qtext.tnt routes.tnt sounds.tnt \
         sys.tnt telltexte.tnt termcap.tnt tnt.dwn tnt.hlp tnt.hlp.eng \
         tnt.ini tnt.up tntc.ini tntrem.hlp tntrem.hlp.eng tntrem.inf
SBINS = sounds/play
SOUNDS = sounds/connect.au sounds/disc.au sounds/reset.au sounds/busy.au \
         sounds/failure.au sounds/ring.au

all:

clean:

install:
	$(MKDIR)  ${prefix};
	@for i in $(EXAMPLES); \
	do \
		$(INSTALL_DATA) $$i ${prefix}; \
	done;
	$(MKDIR)  ${prefix}/sounds;
	@for i in $(SBINS); \
	do \
		$(INSTALL) -m 555 $$i ${prefix}/sounds; \
	done;
	@for i in $(SOUNDS); \
	do \
		$(INSTALL_DATA) $$i ${prefix}/sounds; \
	done;


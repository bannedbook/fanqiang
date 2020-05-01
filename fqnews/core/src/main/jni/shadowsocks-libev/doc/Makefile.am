ASCIIDOC = @ASCIIDOC@
ASCIIDOC_EXTRA =
MANPAGE_XSL = manpage-normal.xsl
XMLTO = @XMLTO@
XMLTO_EXTRA = -m manpage-bold-literal.xsl
GZIPCMD = @GZIP@
INSTALL = @INSTALL@
RM = @RM@
MV = @MV@
SED = @SED@
VERSION = `$(SED) -n 's/.*PACKAGE_VERSION "\(.*\)"/\1/p'\
		  ../config.h`

# Guard against environment variables
MAN1_DOC =
MAN1_DOC += ss-local.1
MAN1_DOC += ss-manager.1
MAN1_DOC += ss-nat.1
MAN1_DOC += ss-redir.1
MAN1_DOC += ss-server.1
MAN1_DOC += ss-tunnel.1

MAN8_DOC =
MAN8_DOC += shadowsocks-libev.8

MAN8_XML = $(MAN8_DOC:%.8=%.xml)
MAN1_XML = $(MAN1_DOC:%.1=%.xml)
MAN_XML  = $(MAN8_XML) $(MAN1_XML)

MAN8_HTML = $(MAN8_DOC:%.8=%.html)
MAN1_HTML = $(MAN1_DOC:%.1=%.html)
MAN_HTML  = $(MAN8_HTML) $(MAN1_HTML)

MAN8_TXT = $(MAN8_DOC:%.8=%.asciidoc)
MAN1_TXT = $(MAN1_DOC:%.1=%.asciidoc)
MAN_TXT  = $(MAN8_TXT) $(MAN1_TXT)

man_MANS = $(MAN8_DOC) $(MAN1_DOC)

html-local: $(MAN_HTML)

%.1: %.xml
	$(AM_V_GEN)$(XMLTO) -m $(MANPAGE_XSL) $(XMLTO_EXTRA) man $<

%.8: %.xml
	$(AM_V_GEN)$(XMLTO) -m $(MANPAGE_XSL) $(XMLTO_EXTRA) man $<

%.xml: %.asciidoc
	$(AM_V_GEN)$(ASCIIDOC) -b docbook -d manpage -f asciidoc.conf \
	    -aversion=$(VERSION) $(ASCIIDOC_EXTRA) -o $@ $<

%.html: %.asciidoc
	$(AM_V_GEN)$(ASCIIDOC) -b html4 -d article -f asciidoc.conf \
		-aversion=$(VERSION) $(ASCIIDOC_EXTRA) -o $@ $<

doc_DATA = $(MAN_HTML)

CLEANFILES = $(MAN_XML) $(man_MANS) $(MAN_HTML)

EXTRA_DIST = *.asciidoc asciidoc.conf *.xsl

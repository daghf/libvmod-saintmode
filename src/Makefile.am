AM_CPPFLAGS = @VMOD_INCLUDES@

vmoddir = @VMOD_DIR@
vmod_LTLIBRARIES = libvmod_saintmode.la

libvmod_saintmode_la_LDFLAGS = -module -export-dynamic -avoid-version -shared

libvmod_saintmode_la_SOURCES = \
	vcc_if.c \
	vcc_if.h \
	vmod_saintmode.c

vcc_if.c vcc_if.h: @VMODTOOL@ $(top_srcdir)/src/vmod_saintmode.vcc
	@VMODTOOL@ $(top_srcdir)/src/vmod_saintmode.vcc

VMOD_TESTS = tests/*.vtc
.PHONY: $(VMOD_TESTS)

tests/*.vtc:
	@VARNISHTEST@ -Dvarnishd=@VARNISHD@ -Dvmod_topbuild=$(abs_top_builddir) -Dvmoddir=@VMOD_DIR@ $@

check: $(VMOD_TESTS)

EXTRA_DIST = \
	vmod_saintmode.vcc \
	$(VMOD_TESTS)

CLEANFILES = $(builddir)/vcc_if.c $(builddir)/vcc_if.h

ACLOCAL_AMFLAGS = -I m4

SUBDIRS=doc include src
if BUILD_TOPOLOGY
SUBDIRS += src/topology
endif
if BUILD_MODULES
SUBDIRS += modules
endif
if BUILD_PCM_PLUGIN_SHM
SUBDIRS += aserver
endif
SUBDIRS += test utils
EXTRA_DIST=README.md ChangeLog INSTALL TODO NOTES configure gitcompile libtool \
	   depcomp version MEMORY-LEAK m4/attributes.m4
AUTOMAKE_OPTIONS=foreign

AM_CPPFLAGS=-I$(top_srcdir)/include

rpm: dist
	$(MAKE) -C utils rpm

dist-hook:
	-chmod -R a+r $(distdir)
	@if ! test -z "$(AMTAR)"; then \
		$(AMTAR) --create --verbose --file=- $(distdir) | bzip2 -c -9 > $(distdir).tar.bz2 ; \
	else \
		$(TAR) --create --verbose --file=- $(distdir) | bzip2 -c -9 > $(distdir).tar.bz2 ; \
	fi

doc-dummy:

doc: doc-dummy
	$(MAKE) -C include all
	$(MAKE) -C doc doc

# $Id: Makefile 8427 2013-07-29 01:10:47Z m-kasahr $

top_builddir = ../../../../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

htmldir=$(default_htmldir)/ja/ref/man1

DOCBOOK_DIR=$(top_srcdir)/doc/docbook/ja/ref/man1
include $(DOCBOOK_DIR)/Makefile.inc
HTMLSRC=$(DOCBOOK)
DOCBOOK2HTML_XSL=../docbook2html.xsl

include $(top_srcdir)/makes/html.mk

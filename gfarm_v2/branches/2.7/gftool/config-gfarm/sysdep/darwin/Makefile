# $Id: Makefile 7223 2012-11-30 13:21:46Z n-soda $

top_builddir = ../../../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

datadir=$(default_datadir)/gfarm/config

DATA =	$(srcdir)/config-gfarm.sysdep \
	$(srcdir)/gfarm-pgsql.in \
	$(srcdir)/../default/gfarm-slapd.in \
	$(srcdir)/../default/gfmd.in \
	$(srcdir)/../default/gfsd.in

include $(top_srcdir)/makes/data.mk

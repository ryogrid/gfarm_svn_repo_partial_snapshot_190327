# $Id: Makefile 2589 2006-05-14 13:32:41Z tatebe $

top_builddir = ../../../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

datadir=$(default_datadir)/gfarm/config

DATA =	$(srcdir)/config-gfarm.sysdep \
	$(srcdir)/../default/gfarm-pgsql.in \
	$(srcdir)/../default/gfarm-slapd.in \
	$(srcdir)/../default/gfmd.in \
	$(srcdir)/../default/gfarm_agent.in \
	$(srcdir)/../default/gfsd.in

include $(top_srcdir)/makes/data.mk

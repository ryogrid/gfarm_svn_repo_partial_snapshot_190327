# $Id: Makefile 1265 2004-08-17 01:47:35Z soda $

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

CFLAGS = $(COMMON_CFLAGS)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

PROGRAM = thput-gfpio
OBJS = thput-gfpio.o

all: $(PROGRAM)

include $(top_srcdir)/makes/gfptool.mk

# $Id: Makefile 2373 2006-01-12 00:54:22Z soda $

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

SRCS = gfcp.c
OBJS = gfcp.o
PROGRAM = gfcp
CFLAGS = $(COMMON_CFLAGS)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM)

include $(top_srcdir)/makes/gfptool.mk

###

$(OBJS): $(DEPGFARMINC)

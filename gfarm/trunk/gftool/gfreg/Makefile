# $Id: Makefile 2493 2006-03-17 04:35:09Z soto $

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

SRCS = gfreg.c
OBJS = gfreg.o
PROGRAM = gfreg
CFLAGS = $(COMMON_CFLAGS) -I$(GFARMLIB_SRCDIR)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM)

include $(top_srcdir)/makes/gfptool.mk

###

$(OBJS): $(DEPGFARMINC) $(GFARMLIB_SRCDIR)/schedule.h $(GFARMLIB_SRCDIR)/host.h

# $Id: Makefile 85 2002-04-04 14:28:20Z soda $

top_objdir = ../..
top_srcdir = $(top_objdir)
srcdir = .

include $(top_srcdir)/makes/var.mk

CFLAGS = $(COMMON_CFLAGS)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

PROGRAM = thput-gfpio
OBJS = thput-gfpio.o

all: $(PROGRAM)

include $(top_srcdir)/makes/prog.mk

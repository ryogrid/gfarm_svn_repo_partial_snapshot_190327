# $Id: Makefile 70 2002-02-02 02:32:10Z soda $

top_objdir = ../..
top_srcdir = $(top_objdir)
srcdir = .

include $(top_srcdir)/makes/var.mk

SRCS = gfwhere.c
OBJS = gfwhere.o
PROGRAM = gfwhere
CFLAGS = $(COMMON_CFLAGS)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM)

include $(top_srcdir)/makes/prog.mk

###

$(OBJS): $(DEPGFARMINC)

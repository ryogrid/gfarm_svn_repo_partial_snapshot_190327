# $Id: Makefile 134 2002-08-06 08:04:17Z soda $

top_objdir = ../..
top_srcdir = $(top_objdir)
srcdir = .

include $(top_srcdir)/makes/var.mk

SRCS = gfrcmd.c
OBJS = gfrcmd.o
PROGRAM = gfrcmd
CFLAGS = $(COMMON_CFLAGS) -I$(GFARMLIB_DIR)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM)

include $(top_srcdir)/makes/prog.mk

###

$(OBJS): $(DEPGFARMINC)

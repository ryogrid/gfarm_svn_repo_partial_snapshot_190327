# $Id: Makefile 6685 2012-09-27 15:47:41Z tatebe $

top_builddir = ../..
top_srcdir = $(top_builddir)
srcdir = .

include $(top_srcdir)/makes/var.mk

PROGRAM = gfchown
PROGRAM2 = gfchgrp
SRCS = $(PROGRAM).c
OBJS = $(PROGRAM).o
CFLAGS = $(COMMON_CFLAGS) -I$(GFARMLIB_SRCDIR)
LDLIBS = $(COMMON_LDLIBS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM)

post-install-hook:
	-rm -f $(DESTDIR)$(bindir)/$(PROGRAM2)
	ln $(DESTDIR)$(bindir)/$(PROGRAM) $(DESTDIR)$(bindir)/$(PROGRAM2)

include $(top_srcdir)/makes/prog.mk

###

$(OBJS): $(DEPGFARMINC) $(GFARMLIB_SRCDIR)/gfarm_path.h

# $Id: Makefile 83 2002-04-04 09:57:18Z soda $

top_objdir = ../..
top_srcdir = $(top_objdir)
srcdir = .

include $(top_srcdir)/makes/var.mk

SRCS = gfrun.c
OBJS = gfrun.o
PROGRAM = gfrun

PROGRAM_LINKS = gfrsh gfssh gfrshl gfsshl
EXTRA_VERYCLEAN_TARGETS = $(PROGRAM_LINKS)

CFLAGS = $(COMMON_CFLAGS) -I$(GFARMLIB_DIR)
LDLIBS = $(COMMON_LDFLAGS) $(GFARMLIB) $(LIBS)
DEPLIBS = $(DEPGFARMLIB)

all: $(PROGRAM) $(PROGRAM_LINKS)

include $(top_srcdir)/makes/prog.mk

$(PROGRAM_LINKS): $(PROGRAM)
	rm -f $@
	ln $(PROGRAM) $@

post-install-hook:
	for f in $(PROGRAM_LINKS); do \
		rm -f $(bindir)/$$f; ln $(bindir)/$(PROGRAM) $(bindir)/$$f; \
	done

###

$(OBJS): $(DEPGFARMINC)

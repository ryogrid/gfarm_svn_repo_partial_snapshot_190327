all: post-all-hook
install: post-install-hook
clean: post-clean-hook
veryclean: post-veryclean-hook
distclean: post-distclean-hook
man: html-man
html: html-html
msgno: html-msgno
catalog: html-catalog

post-all-hook: html-all
post-install-hook: html-install
post-clean-hook: html-clean
post-veryclean-hook: html-veryclean
post-distclean-hook: html-distclean

html-all:

html-install: all
	@$(MKDIR_P) $(DESTDIR)$(htmldir)
	@for i in -- $(HTML); do \
		case $$i in --) continue;; esac; \
		echo \
		$(INSTALL_DOC) $(srcdir)/$${i} $(DESTDIR)$(htmldir)/$${i}; \
		$(INSTALL_DOC) $(srcdir)/$${i} $(DESTDIR)$(htmldir)/$${i}; \
	done
	@for i in -- $(HTMLSRC); do \
		case $$i in --) continue;; esac; \
		echo \
		$(INSTALL_DOC) $(srcdir)/$${i}.html \
			$(DESTDIR)$(htmldir)/$${i}.html; \
		$(INSTALL_DOC) $(srcdir)/$${i}.html \
			$(DESTDIR)$(htmldir)/$${i}.html; \
	done

html-clean:
	-test -z "$(DOCBOOK2HTML_XSL)" || $(RM) -f $(DOCBOOK2HTML_XSL)
	-test -z "$(EXTRA_CLEAN_TARGETS)" || $(RM) -f $(EXTRA_CLEAN_TARGETS)

html-veryclean: clean
	-test -z "$(EXTRA_VERYCLEAN_TARGETS)" || $(RM) -f $(EXTRA_VERYCLEAN_TARGETS)

html-distclean: veryclean
	-test ! -f $(srcdir)/Makefile.in || $(RM) -f Makefile

html-man:

$(DOCBOOK2HTML_XSL): $(srcdir)/$(DOCBOOK2HTML_XSL).in
	for i in -- $(DOCBOOK_XSLDIRS); do \
		case $$i in --) continue;; esac; \
		test -d $$i \
			&& sed -e "s|@DOCBOOK_XSLDIR@|$$i|" $? > $@ \
			&& exit 0; \
	done; \
	echo "No DocBook XSL directory found."; \
	exit 1

$(dstsubst): $(srcsubst)
	$(XSLTPROC) $(DOCBOOK2HTML_XSL) $(srcsubst) > $(dstsubst)

html-html: $(DOCBOOK2HTML_XSL)
	for i in -- $(HTMLSRC); do \
		case $$i in --) continue;; esac; \
		$(MAKE) -f $(srcdir)/Makefile \
			srcsubst=$(DOCBOOK_DIR)/$${i}.docbook \
			dstsubst=$${i}.html $${i}.html; \
	done

html-msgno:
html-catalog:

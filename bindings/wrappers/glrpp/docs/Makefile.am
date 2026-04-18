DOXYGEN ?= @DOXYGEN@
AWK ?= awk
SED ?= sed
srcdir ?= .
builddir ?= .
top_srcdir ?= ..
abs_srcdir ?= $(abspath $(srcdir))
abs_builddir ?= $(abspath $(builddir))
abs_top_srcdir ?= $(abspath $(top_srcdir))
PACKAGE_NAME ?= glrpp
PACKAGE_VERSION ?= 0.1.0

BOOKLET_MANIFEST = $(srcdir)/booklet/Manifest.yaml
BOOKLET_INPUT = $(builddir)/booklet-input.dox
DOXYFILE = $(builddir)/Doxyfile
HTML_INDEX = $(builddir)/html/index.html
LATEX_REFMAN = $(builddir)/latex/refman.tex
PDF_MANUAL = $(builddir)/glrpp-reference.pdf

.PHONY: all html latex pdf docs clean

all: html

$(BOOKLET_INPUT): $(BOOKLET_MANIFEST)
	@mkdir -p $(builddir)
	@rm -f $@
	@{ $(AWK) '/^[[:space:]]*- file:[[:space:]]*/ { print $$3 }' $(BOOKLET_MANIFEST); find "$(srcdir)/booklet" -maxdepth 1 -name '*.md' -printf '%f\n' | sort; } | $(AWK) '!seen[$$0]++' | \
	while read -r chapter; do \
		if test -f "$(srcdir)/booklet/$$chapter"; then \
			printf 'INPUT += "%s/booklet/%s"\n' "$(abs_srcdir)" "$$chapter" >> $@; \
		fi; \
	done

$(DOXYFILE): $(srcdir)/Doxyfile.in $(BOOKLET_INPUT)
	@mkdir -p $(builddir)
	@$(SED) \
		-e 's|@GLRPP_PROJECT_NAME@|@PACKAGE_NAME@|g' \
		-e 's|@GLRPP_PROJECT_VERSION@|@PACKAGE_VERSION@|g' \
		-e 's|@GLRPP_PROJECT_SOURCE_DIR@|$(abs_top_srcdir)|g' \
		-e 's|@GLRPP_DOCS_SOURCE_DIR@|$(abs_srcdir)|g' \
		-e 's|@GLRPP_DOCS_OUTPUT_DIR@|$(abs_builddir)|g' \
		-e '/@GLRPP_BOOKLET_INPUT@/r $(BOOKLET_INPUT)' \
		-e 's|@GLRPP_BOOKLET_INPUT@||g' \
		$(srcdir)/Doxyfile.in > $@

$(HTML_INDEX): $(DOXYFILE)
	$(DOXYGEN) $(DOXYFILE)

$(LATEX_REFMAN): $(DOXYFILE)
	$(DOXYGEN) $(DOXYFILE)

$(PDF_MANUAL): $(LATEX_REFMAN)
	$(MAKE) -C $(builddir)/latex
	cp $(builddir)/latex/refman.pdf $@

html: $(HTML_INDEX)

latex: $(LATEX_REFMAN)

pdf: $(PDF_MANUAL)

docs: html

clean:
	rm -rf $(BOOKLET_INPUT) $(DOXYFILE) $(builddir)/html $(builddir)/latex $(PDF_MANUAL)

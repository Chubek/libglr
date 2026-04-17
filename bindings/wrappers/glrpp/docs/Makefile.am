DOC_SOURCES = \
    mainpage.md \
    glrpp-actions-basics.md \
    glrpp-advanced-metaprogramming.md \
    glrpp-basic-usage.md \
    glrpp-case-study-semgrep.md \
    glrpp-case-study-sql.md \
    glrpp-case-study-syntax-highlighting.md \
    glrpp-case-study-yaml.md \
    glrpp-compile-time-regex.md \
    glrpp-configuration.md \
    glrpp-debugging-grammars.md \
    glrpp-disambiguation-dsl.md \
    glrpp-disambiguation-forests.md \
    glrpp-explaining-files.md \
    glrpp-explaining-rules.md \
    glrpp-forking-parsers.md \
    glrpp-handling-reflections.md \
    glrpp-hooking-the-reader.md \
    glrpp-hooking-up-lexer.md \
    glrpp-installation.md \
    glrpp-lexer-hooks.md \
    glrpp-list-of-data-structures.md \
    glrpp-managing-ast.md \
    glrpp-managing-context.md \
    glrpp-managing-dependencies.md \
    glrpp-managing-stack.md \
    glrpp-managing-symbols.md \
    glrpp-metaprogramming-basics.md \
    glrpp-metaprogramming-guide.md \
    glrpp-native-dsl-specs.md \
    glrpp-parser-object.md \
    glrpp-pipeline-operator.md \
    glrpp-reading-files.md \
    glrpp-reading-tokens.md \
    glrpp-reporting-errors.md \
    glrpp-rewrite-dsl.md \
    glrpp-rewriting-grammars.md \
    glrpp-scannerless-parser.md \
    glrpp-semantic-actions.md \
    glrpp-semantic-basics.md \
    glrpp-setting-up.md \
    glrpp-specifying-grammars.md \
    glrpp-string-utils.md \
    glrpp-using-native-dsl.md \
    glrpp-utf16-conversion.md \
    glrpp-what-are-nodes.md \
    glrpp-what-is-glr-parsing.md

EXTRA_DIST = $(DOC_SOURCES) Doxyfile.in layout.xml Doxygen.local

if HAVE_DOXYGEN
docs: Doxyfile $(DOC_SOURCES)
	$(DOXYGEN) Doxyfile

clean-local:
	rm -rf html latex
endif

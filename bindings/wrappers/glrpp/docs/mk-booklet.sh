#!/bin/bash

BOOKLET_CONCAT="glrpp-booklet.md"

# Clear output file
> "$BOOKLET_CONCAT"

# Ordered list (same order as the YAML manifest)
FILES=(
  booklet/glrpp-what-is-glr-parsing.md
  booklet/glrpp-setting-up.md
  booklet/glrpp-installation.md
  booklet/glrpp-basic-usage.md
  booklet/glrpp-parser-object.md
  booklet/glrpp-reading-files.md
  booklet/glrpp-reading-tokens.md
  booklet/glrpp-hooking-up-lexer.md
  booklet/glrpp-lexer-hooks.md
  booklet/glrpp-hooking-the-reader.md
  booklet/glrpp-string-utils.md
  booklet/glrpp-utf16-conversion.md
  booklet/glrpp-what-are-nodes.md
  booklet/glrpp-specifying-grammars.md
  booklet/glrpp-native-dsl-specs.md
  booklet/glrpp-using-native-dsl.md
  booklet/glrpp-disambiguation-dsl.md
  booklet/glrpp-disambiguation-forests.md
  booklet/glrpp-actions-basics.md
  booklet/glrpp-semantic-basics.md
  booklet/glrpp-semantic-actions.md
  booklet/glrpp-configuration.md
  booklet/glrpp-pipeline-operator.md
  booklet/glrpp-forking-parsers.md
  booklet/glrpp-rewrite-dsl.md
  booklet/glrpp-rewriting-grammars.md
  booklet/glrpp-managing-ast.md
  booklet/glrpp-handling-reflections.md
  booklet/glrpp-managing-symbols.md
  booklet/glrpp-managing-context.md
  booklet/glrpp-managing-stack.md
  booklet/glrpp-managing-dependencies.md
  booklet/glrpp-compile-time-regex.md
  booklet/glrpp-metaprogramming-basics.md
  booklet/glrpp-metaprogramming-guide.md
  booklet/glrpp-advanced-metaprogramming.md
  booklet/glrpp-debugging-grammars.md
  booklet/glrpp-scannerless-parser.md
  booklet/glrpp-explaining-files.md
  booklet/glrpp-explaining-rules.md
  booklet/glrpp-reporting-errors.md
  booklet/glrpp-case-study-sql.md
  booklet/glrpp-case-study-yaml.md
  booklet/glrpp-case-study-syntax-highlighting.md
  booklet/glrpp-case-study-semgrep.md
  booklet/glrpp-list-of-data-structures.md
)

for f in "${FILES[@]}"; do
    if [ ! -f "$f" ]; then
        echo "Skipping missing chapter $f" >&2
        continue
    fi
    echo "# $(basename "$f" .md)" >> "$BOOKLET_CONCAT"
    echo "" >> "$BOOKLET_CONCAT"
    cat "$f" >> "$BOOKLET_CONCAT"
    echo -e "\n\n" >> "$BOOKLET_CONCAT"
done

echo "Generated $BOOKLET_CONCAT"

BOOKLET_TARGET="${BOOKLET_TARGET:-glrpp.html}"

if ! command -v markdown >/dev/null 2>&1; then
	echo "markdown(1) not found, either not installed or not discoverable by the script"
	exit 1
fi

markdown "$BOOKLET_CONCAT" > "$BOOKLET_TARGET"

echo "Compiled $BOOKLET_CONCAT to $BOOKLET_TARGET"

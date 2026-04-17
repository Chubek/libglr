#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

BOOKLET_DIR="${BOOKLET_DIR:-$SCRIPT_DIR/booklet}"
BOOKLET_CONCAT="glrpp-booklet.md"

echo "Booklet directory was selected as $BOOKLET_DIR"

# Clear output file
> "$BOOKLET_CONCAT"

# Ordered list (same order as the YAML manifest)
FILES=(
  $BOOKLET_DIR/glrpp-what-is-glr-parsing.md
  $BOOKLET_DIR/glrpp-setting-up.md
  $BOOKLET_DIR/glrpp-installation.md
  $BOOKLET_DIR/glrpp-basic-usage.md
  $BOOKLET_DIR/glrpp-parser-object.md
  $BOOKLET_DIR/glrpp-reading-files.md
  $BOOKLET_DIR/glrpp-reading-tokens.md
  $BOOKLET_DIR/glrpp-hooking-up-lexer.md
  $BOOKLET_DIR/glrpp-lexer-hooks.md
  $BOOKLET_DIR/glrpp-hooking-the-reader.md
  $BOOKLET_DIR/glrpp-string-utils.md
  $BOOKLET_DIR/glrpp-utf16-conversion.md
  $BOOKLET_DIR/glrpp-what-are-nodes.md
  $BOOKLET_DIR/glrpp-specifying-grammars.md
  $BOOKLET_DIR/glrpp-native-dsl-specs.md
  $BOOKLET_DIR/glrpp-using-native-dsl.md
  $BOOKLET_DIR/glrpp-disambiguation-dsl.md
  $BOOKLET_DIR/glrpp-disambiguation-forests.md
  $BOOKLET_DIR/glrpp-actions-basics.md
  $BOOKLET_DIR/glrpp-semantic-basics.md
  $BOOKLET_DIR/glrpp-semantic-actions.md
  $BOOKLET_DIR/glrpp-configuration.md
  $BOOKLET_DIR/glrpp-pipeline-operator.md
  $BOOKLET_DIR/glrpp-forking-parsers.md
  $BOOKLET_DIR/glrpp-rewrite-dsl.md
  $BOOKLET_DIR/glrpp-rewriting-grammars.md
  $BOOKLET_DIR/glrpp-managing-ast.md
  $BOOKLET_DIR/glrpp-handling-reflections.md
  $BOOKLET_DIR/glrpp-managing-symbols.md
  $BOOKLET_DIR/glrpp-managing-context.md
  $BOOKLET_DIR/glrpp-managing-stack.md
  $BOOKLET_DIR/glrpp-managing-dependencies.md
  $BOOKLET_DIR/glrpp-compile-time-regex.md
  $BOOKLET_DIR/glrpp-metaprogramming-basics.md
  $BOOKLET_DIR/glrpp-metaprogramming-guide.md
  $BOOKLET_DIR/glrpp-advanced-metaprogramming.md
  $BOOKLET_DIR/glrpp-debugging-grammars.md
  $BOOKLET_DIR/glrpp-scannerless-parser.md
  $BOOKLET_DIR/glrpp-explaining-files.md
  $BOOKLET_DIR/glrpp-explaining-rules.md
  $BOOKLET_DIR/glrpp-reporting-errors.md
  $BOOKLET_DIR/glrpp-case-study-sql.md
  $BOOKLET_DIR/glrpp-case-study-yaml.md
  $BOOKLET_DIR/glrpp-case-study-syntax-highlighting.md
  $BOOKLET_DIR/glrpp-case-study-semgrep.md
  $BOOKLET_DIR/glrpp-list-of-data-structures.md
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

echo "Generated $BOOKLET_CONCAT by concatenating booklet chapters"

BOOKLET_TARGET_FILE="${1:-$HOME/glrpp-booklet.html}"

echo "$BOOKLET_TARGET_FILE was selected as the destination of HTML file"

if ! command -v md2html >/dev/null 2>&1; then
	echo "md2html(1) not found, please install it from the third_party/md4c directory"
	exit 1
fi

md2html --full-html --ftables --fcollapse-whitespace -o "$BOOKLET_TARGET_FILE" "$BOOKLET_CONCAT"

echo "Compiled $BOOKLET_CONCAT to $BOOKLET_TARGET_FILE using md2html(1)"

rm "$BOOKLET_CONCAT"

echo "Artifacts removed"

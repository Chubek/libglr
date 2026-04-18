#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
booklet_dir="${script_dir}/booklet"
manifest="${booklet_dir}/Manifest.yaml"
output_dir="${script_dir}/booklet-output"
mode="${1:-both}"

require() {
    command -v "$1" >/dev/null 2>&1 || {
        printf 'missing required command: %s\n' "$1" >&2
        exit 1
    }
}

collect_chapters() {
    awk '/^[[:space:]]*- file:[[:space:]]*/ { print $3 }' "${manifest}" | while read -r chapter; do
        if [[ -f "${booklet_dir}/${chapter}" ]]; then
            printf '%s\n' "${booklet_dir}/${chapter}"
        fi
    done
}

build_combined_markdown() {
    local combined="${output_dir}/glrpp-booklet.md"

    mkdir -p "${output_dir}"
    {
        printf '# GLR++: A Comprehensive Guide\n\n'
        printf '_Generated on %s._\n\n' "$(date +%Y-%m-%d)"
        while read -r chapter_path; do
            [[ -n "${chapter_path}" ]] || continue
            cat "${chapter_path}"
            printf '\n\n'
        done < <(collect_chapters)
    } > "${combined}"
}

build_html() {
    require pandoc
    pandoc \
        "${output_dir}/glrpp-booklet.md" \
        --standalone \
        --toc \
        --css "${script_dir}/doxygen-darkmode.css" \
        --metadata title="GLR++: A Comprehensive Guide" \
        -o "${output_dir}/glrpp-booklet.html"
}

build_pdf() {
    require pandoc
    require pdflatex
    pandoc \
        "${output_dir}/glrpp-booklet.md" \
        --standalone \
        --toc \
        --include-in-header "${script_dir}/ltx-header.tex" \
        --include-after-body "${script_dir}/ltx-footer.tex" \
        --pdf-engine=pdflatex \
        --metadata title="GLR++: A Comprehensive Guide" \
        -o "${output_dir}/glrpp-booklet.pdf"
}

main() {
    [[ -f "${manifest}" ]] || {
        printf 'manifest not found: %s\n' "${manifest}" >&2
        exit 1
    }

    build_combined_markdown

    case "${mode}" in
        html)
            build_html
            ;;
        pdf)
            build_pdf
            ;;
        both)
            build_html
            build_pdf
            ;;
        *)
            printf 'usage: %s [html|pdf|both]\n' "$0" >&2
            exit 1
            ;;
    esac
}

main "$@"

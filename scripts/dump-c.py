#!/usr/bin/env python3
"""
C Source Code Documentation Generator using pycparser

Generates a plaintext file documenting C source files with:
- File contents
- Dependencies (includes)
- Declared/defined structures, functions, typedefs, enums
- Optional AST S-expression representation
- Optional interface/implementation pairing
"""

import argparse
import os
import sys
from pathlib import Path
from pycparser import parse_file, c_ast, c_generator
from pycparser.plyparser import ParseError


class CFileAnalyzer(c_ast.NodeVisitor):
    """Visitor to extract declarations and definitions from C AST"""

    def __init__(self):
        self.functions = []
        self.structs = []
        self.typedefs = []
        self.enums = []
        self.variables = []

    def visit_FuncDef(self, node):
        func_name = node.decl.name
        coord = node.decl.coord
        gen = c_generator.CGenerator()
        func_decl = gen.visit(node.decl)
        self.functions.append({
            'name': func_name,
            'signature': func_decl,
            'coord': str(coord) if coord else 'unknown',
            'type': 'definition'
        })
        self.generic_visit(node)

    def visit_Decl(self, node):
        if isinstance(node.type, c_ast.FuncDecl):
            gen = c_generator.CGenerator()
            func_decl = gen.visit(node)
            self.functions.append({
                'name': node.name,
                'signature': func_decl,
                'coord': str(node.coord) if node.coord else 'unknown',
                'type': 'declaration'
            })
        elif isinstance(node.type, c_ast.Struct):
            if node.type.name:
                gen = c_generator.CGenerator()
                struct_def = gen.visit(node.type)
                self.structs.append({
                    'name': node.type.name,
                    'definition': struct_def,
                    'coord': str(node.coord) if node.coord else 'unknown'
                })
        elif node.name:
            gen = c_generator.CGenerator()
            var_decl = gen.visit(node)
            self.variables.append({
                'name': node.name,
                'declaration': var_decl,
                'coord': str(node.coord) if node.coord else 'unknown'
            })
        self.generic_visit(node)

    def visit_Typedef(self, node):
        gen = c_generator.CGenerator()
        typedef_str = gen.visit(node)
        self.typedefs.append({
            'name': node.name,
            'definition': typedef_str,
            'coord': str(node.coord) if node.coord else 'unknown'
        })
        self.generic_visit(node)

    def visit_Struct(self, node):
        if node.name and node.decls:
            gen = c_generator.CGenerator()
            struct_def = gen.visit(node)
            if not any(s['name'] == node.name for s in self.structs):
                self.structs.append({
                    'name': node.name,
                    'definition': struct_def,
                    'coord': str(node.coord) if node.coord else 'unknown'
                })
        self.generic_visit(node)

    def visit_Enum(self, node):
        gen = c_generator.CGenerator()
        enum_def = gen.visit(node)
        enum_name = node.name if node.name else 'anonymous'
        self.enums.append({
            'name': enum_name,
            'definition': enum_def,
            'coord': str(node.coord) if node.coord else 'unknown'
        })
        self.generic_visit(node)


def extract_includes(filepath):
    includes = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                line = line.strip()
                if line.startswith('#include'):
                    includes.append(line)
    except Exception as e:
        print(f"Warning: Could not read {filepath} for includes: {e}", file=sys.stderr)
    return includes


def ast_to_sexp(node, indent=0):
    if node is None:
        return "nil"

    node_type = node.__class__.__name__
    result = "(" + node_type

    attrs = []
    for attr_name in node.attr_names:
        attr_value = getattr(node, attr_name, None)
        if attr_value is not None:
            attrs.append(f":{attr_name} {repr(attr_value)}")

    if attrs:
        result += " " + " ".join(attrs)

    for child_name, child in node.children():
        result += "\n" + "  " * (indent + 1)
        if isinstance(child, list):
            result += f"({child_name}"
            for item in child:
                result += "\n" + "  " * (indent + 2) + ast_to_sexp(item, indent + 2)
            result += ")"
        else:
            result += f"({child_name} {ast_to_sexp(child, indent + 1)})"

    result += ")"
    return result


def code_block(language, source, content):
    """Format a plaintext code block"""
    src_attr = f"source={source}" if source else "source="
    lines = [
        f"== Code[language={language}, {src_attr}]",
        "<BeginCode>",
        content,
        "<EndCode>",
    ]
    return "\n".join(lines)


def analyze_c_file(filepath, cpp_args=None):
    try:
        ast = parse_file(filepath, use_cpp=True, cpp_args=cpp_args or ['-E'])
        analyzer = CFileAnalyzer()
        analyzer.visit(ast)
        includes = extract_includes(filepath)
        return {
            'ast': ast,
            'functions': analyzer.functions,
            'structs': analyzer.structs,
            'typedefs': analyzer.typedefs,
            'enums': analyzer.enums,
            'variables': analyzer.variables,
            'includes': includes
        }
    except ParseError as e:
        print(f"Parse error in {filepath}: {e}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"Error analyzing {filepath}: {e}", file=sys.stderr)
        return None


def match_interface_implementation(intf_files, impl_files):
    pairs = []
    unmatched_intf = []
    unmatched_impl = list(impl_files)

    for intf_path in intf_files:
        intf_name = Path(intf_path).stem
        matched = False
        for impl_path in impl_files:
            if intf_name == Path(impl_path).stem:
                pairs.append((intf_path, impl_path))
                if impl_path in unmatched_impl:
                    unmatched_impl.remove(impl_path)
                matched = True
                break
        if not matched:
            unmatched_intf.append(intf_path)

    return pairs, unmatched_intf, unmatched_impl


def generate_plaintext_block(filepath, analysis, show_ast=False):
    out = []
    filename = os.path.basename(filepath)

    out.append("=" * 60)
    out.append(f"FILE: {filename}")
    out.append(f"PATH: {filepath}")
    out.append("=" * 60)

    # Dependencies
    if analysis['includes']:
        out.append("\n-- Dependencies --")
        for inc in analysis['includes']:
            out.append(f"  {inc}")

    # Structures
    if analysis['structs']:
        out.append("\n-- Structures --")
        for struct in analysis['structs']:
            out.append(f"\n  {struct['name']}  (at {struct['coord']})")
            out.append(code_block("C", filepath, struct['definition']))

    # Enums
    if analysis['enums']:
        out.append("\n-- Enumerations --")
        for enum in analysis['enums']:
            out.append(f"\n  {enum['name']}  (at {enum['coord']})")
            out.append(code_block("C", filepath, enum['definition']))

    # Typedefs
    if analysis['typedefs']:
        out.append("\n-- Type Definitions --")
        for typedef in analysis['typedefs']:
            out.append(f"\n  {typedef['name']}  (at {typedef['coord']})")
            out.append(code_block("C", filepath, typedef['definition']))

    # Functions
    if analysis['functions']:
        out.append("\n-- Functions --")
        for func in analysis['functions']:
            out.append(f"\n  {func['name']}  [{func['type']}]  (at {func['coord']})")
            out.append(code_block("C", filepath, func['signature']))

    # Variables
    if analysis['variables']:
        out.append("\n-- Global Variables --")
        for var in analysis['variables']:
            out.append(f"  {var['name']}  (at {var['coord']})")
            out.append(code_block("C", filepath, var['declaration']))

    # Source code
    out.append("\n-- Source Code --")
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            source = f.read()
    except Exception as e:
        source = f"// Error reading file: {e}"
    out.append(code_block("C", filepath, source))

    # AST
    if show_ast and analysis['ast']:
        out.append("\n-- Abstract Syntax Tree (S-Expression) --")
        out.append(code_block("Lisp", "", ast_to_sexp(analysis['ast'])))

    out.append("\n")
    return "\n".join(out)


def main():
    parser = argparse.ArgumentParser(
        description='Generate plaintext documentation from C source files using pycparser'
    )
    parser.add_argument('files', nargs='*', help='C source files to document')
    parser.add_argument('-o', '--output', default='documentation.txt',
                        help='Output file (default: documentation.txt)')
    parser.add_argument('--show-ast', action='store_true',
                        help='Include AST S-expression in output')
    parser.add_argument('--intf-dir', help='Interface directory (e.g., include/)')
    parser.add_argument('--impl-dir', help='Implementation directory (e.g., src/)')
    parser.add_argument('--cpp-args', help='Additional arguments for C preprocessor')

    args = parser.parse_args()

    cpp_args = ['-E']
    if args.cpp_args:
        cpp_args.extend(args.cpp_args.split())

    all_files = []
    if args.files:
        for pattern in args.files:
            path = Path(pattern)
            if path.is_file():
                all_files.append(str(path))
            elif path.is_dir():
                all_files.extend([str(f) for f in path.rglob('*.c')])
                all_files.extend([str(f) for f in path.rglob('*.h')])

    if args.intf_dir and args.impl_dir:
        intf_files = list(Path(args.intf_dir).rglob('*.h'))
        impl_files = list(Path(args.impl_dir).rglob('*.c'))

        pairs, unmatched_intf, unmatched_impl = match_interface_implementation(
            [str(f) for f in intf_files],
            [str(f) for f in impl_files]
        )

        with open(args.output, 'w', encoding='utf-8') as out:
            out.write("C SOURCE CODE DOCUMENTATION\n")
            out.write("Generated from interface/implementation pairs\n\n")

            if pairs:
                out.write("*** Interface/Implementation Pairs ***\n\n")
                for intf, impl in pairs:
                    out.write(f"  Pair: {Path(intf).name} <-> {Path(impl).name}\n\n")

                    out.write(f"  [Interface: {Path(intf).name}]\n")
                    analysis = analyze_c_file(intf, cpp_args)
                    if analysis:
                        out.write(generate_plaintext_block(intf, analysis, args.show_ast))

                    out.write(f"  [Implementation: {Path(impl).name}]\n")
                    analysis = analyze_c_file(impl, cpp_args)
                    if analysis:
                        out.write(generate_plaintext_block(impl, analysis, args.show_ast))

            if unmatched_intf:
                out.write("*** Unmatched Interface Files ***\n\n")
                for intf in unmatched_intf:
                    analysis = analyze_c_file(intf, cpp_args)
                    if analysis:
                        out.write(generate_plaintext_block(intf, analysis, args.show_ast))

            if unmatched_impl:
                out.write("*** Unmatched Implementation Files ***\n\n")
                for impl in unmatched_impl:
                    analysis = analyze_c_file(impl, cpp_args)
                    if analysis:
                        out.write(generate_plaintext_block(impl, analysis, args.show_ast))

    elif all_files:
        with open(args.output, 'w', encoding='utf-8') as out:
            out.write("C SOURCE CODE DOCUMENTATION\n")
            out.write(f"Generated from {len(all_files)} file(s)\n\n")

            for filepath in all_files:
                print(f"Processing {filepath}...", file=sys.stderr)
                analysis = analyze_c_file(filepath, cpp_args)
                if analysis:
                    out.write(generate_plaintext_block(filepath, analysis, args.show_ast))
    else:
        print("No files specified. Use files argument or --intf-dir/--impl-dir", file=sys.stderr)
        return 1

    print(f"Documentation written to {args.output}")
    return 0


if __name__ == '__main__':
    sys.exit(main())

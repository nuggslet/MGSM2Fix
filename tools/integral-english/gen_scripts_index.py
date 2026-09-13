#!/usr/bin/env python
"""Regenerate SCRIPTS.md from every script's own module docstring.

    py gen_scripts_index.py

Takes the first sentence of each .py file's module docstring (the first
paragraph, cut at the first ". "). Run this after adding a script or changing
an opening docstring; SCRIPTS.md is committed, not built on the fly, so it
stays readable without running Python.
"""
import ast
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent


def first_sentence(doc):
    if not doc:
        return '(no module docstring)'
    para_lines = []
    for line in doc.strip().split('\n'):
        if not line.strip():
            break
        para_lines.append(line.strip())
    para = ' '.join(para_lines)
    m = re.search(r'\.(?:\s|$)', para)
    sentence = para[:m.end()].rstrip() if m else para
    if len(sentence) > 160:
        sentence = sentence[:157].rstrip() + '...'
    return sentence


def main():
    rows = []
    for path in sorted(HERE.glob('*.py')):
        f = path.name
        if f == 'gen_scripts_index.py':
            continue
        src = path.read_text(encoding='utf-8')
        try:
            doc = ast.get_docstring(ast.parse(src, filename=f)) or ''
        except SyntaxError:
            doc = ''
        rows.append((f, first_sentence(doc)))

    with (HERE / 'SCRIPTS.md').open('w', encoding='utf-8', newline='\n') as out:
        out.write('# Script index\n\n')
        out.write(
            "One line per script in this directory, pulled from its module "
            "docstring's first sentence by `gen_scripts_index.py`. Each "
            "script's own docstring has the full picture and usually a "
            "worked example; `REFERENCE.md` has the mechanisms, and "
            "`NextSteps.md` has how the pieces fit together and what "
            "remains.\n\n")
        out.write('| Script | What it does |\n|---|---|\n')
        for f, sentence in rows:
            out.write('| `%s` | %s |\n' % (f, sentence.replace('|', '\\|')))
    print('%d scripts indexed' % len(rows))


if __name__ == '__main__':
    main()

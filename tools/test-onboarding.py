#!/usr/bin/env python3
"""Tests for onboarding: the walk follows the value onboarding as the machine evaluates it, serves
only the bytes that value identifies, whole, in chunks; no C build or network needed (the machine's
evaluation is replaced by its shown output).

Run: python3 tools/test-onboarding.py -v
"""
import hashlib
import importlib.machinery
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

REPO = Path(__file__).resolve().parents[1]
loader = importlib.machinery.SourceFileLoader('hyperactive_onboarding', str(REPO / 'hyperactive'))
spec = importlib.util.spec_from_loader(loader.name, loader)
cli = importlib.util.module_from_spec(spec)
loader.exec_module(cli)
tg = cli.task_graph_module()


def shown(name, docs):
    """the machine's #show output for a List Document value"""
    def lst(text):
        acc = 'nil'
        for b in reversed(text.encode('utf-8')):
            acc = f'(cons #{b} {acc})'
        return acc
    body = 'nil'
    for path, digest in reversed(docs):
        body = f'(cons (document-at {lst(path)} {lst(digest)}) {body})'
    return f'{name}\n  = {body[1:-1]}\n196 passed, 0 failed\n'


class Onboarding(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        p = patch.object(cli, 'ROOT', str(self.root)); p.start(); self.addCleanup(p.stop)

    def write(self, relative, data):
        p = self.root / relative
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data.encode('utf-8') if isinstance(data, str) else data)
        return relative, hashlib.sha256(p.read_bytes()).hexdigest()

    def test_decoding_the_shown_value(self):
        docs = [('AGENTS.md', 'ab' * 32), ('docs/é-piece.md', 'cd' * 32)]
        self.assertEqual(tg.decode_documents(shown('onboarding', docs), 'onboarding'), docs)

    def test_no_value_shown_is_explicit(self):
        with self.assertRaisesRegex(SystemExit, 'showed no value'):
            tg.decode_documents('FAIL: checks onboarding\n', 'onboarding')

    def test_a_changed_file_is_not_served(self):
        rel, digest = self.write('docs/piece.md', 'as evaluated')
        self.write('docs/piece.md', 'changed since')
        with self.assertRaisesRegex(SystemExit, 'changed while the walk'):
            cli.served(rel, digest)

    def test_walk_follows_the_value_whole_and_in_order(self):
        docs = [self.write('AGENTS.md', 'Repository-only instruction'),
                self.write('docs/new-piece.md', '# Newly pointed\n\n' + '\n\n'.join('Paragraph %d.' % i for i in range(30))),
                self.write('docs/word-list.md', 'word')]
        with patch.object(cli, 'onboarding_documents', return_value=docs), \
             patch.object(cli, 'tool', return_value='Actual graph output'), \
             patch.object(cli, 'open_work', lambda: print('open')), patch.object(cli, 'CHUNK', 80):
            chunks = cli.walk_chunks()
        self.assertEqual(chunks[0], ('AGENTS.md', 'Repository-only instruction'))
        piece = [t for title, t in chunks if title.startswith('docs/new-piece.md')]
        self.assertGreater(len(piece), 1)
        self.assertEqual('\n\n'.join(piece), (self.root / 'docs/new-piece.md').read_text())
        self.assertTrue(all(len(t) <= 80 for _, t in chunks))
        titles = [t.split(',')[0] for t, _ in chunks]
        self.assertLess(titles.index('docs/new-piece.md'), titles.index('docs/word-list.md'))

    def test_a_pdf_is_labelled_a_snapshot(self):
        rel, digest = self.write('docs/paper.pdf', b'%PDF')
        def extract(args, **kw):
            Path(args[3]).write_text('Section 1\fSection 2')
            return subprocess.CompletedProcess(args, 0, '', '')
        with patch.object(cli.subprocess, 'run', side_effect=extract):
            text = cli.pdf_text(str(self.root / rel), b'%PDF')
        self.assertTrue(text.startswith('SOURCE IMPORT REQUIRED'))
        self.assertIn('Section 1\n\nSection 2', text)

    def test_task_graph_failure_does_not_look_like_empty_success(self):
        r = subprocess.CompletedProcess([], 2, 'partial output', 'actual failure')
        with patch.object(cli.subprocess, 'run', return_value=r):
            with self.assertRaisesRegex(SystemExit, 'actual failure'):
                cli.tool('tree')


if __name__ == '__main__':
    unittest.main()

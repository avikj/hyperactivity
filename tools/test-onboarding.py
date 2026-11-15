#!/usr/bin/env python3
"""Tests for repository-owned, live onboarding inputs; no C build or network needed.

Run: python tools/test-onboarding.py -v
"""
import importlib.machinery
import importlib.util
import os
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


class OnboardingSources(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / 'docs').mkdir()
        paths = {
            'ROOT': self.root,
            'PAPER_SOURCE': self.root / 'docs/Hyperactivity_Current_Draft.md',
            'PAPER': self.root / 'docs/hyperactivity_v31.pdf',
            'ONBOARDING': self.root / 'docs/onboarding.cub',
        }
        for key, value in paths.items():
            p = patch.object(cli, key, str(value)); p.start(); self.addCleanup(p.stop)
        self.source = Path(cli.PAPER_SOURCE)
        self.pdf = Path(cli.PAPER)

    def write(self, relative, text):
        p = self.root / relative
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text, encoding='utf-8')
        return p

    def test_source_wins_without_pdf_or_extractor(self):
        self.source.write_text('# Part I\n\nActual source.\n', encoding='utf-8')
        with patch.object(cli.subprocess, 'run', side_effect=AssertionError('must not extract PDF')):
            self.assertEqual(cli.paper_text(), self.source.read_text())

    def test_source_edits_are_visible_even_with_unchanged_mtime(self):
        self.source.write_text('Before')
        st = self.source.stat()
        self.assertEqual(cli.paper_text(), 'Before')
        self.source.write_text('After')
        os.utime(self.source, ns=(st.st_atime_ns, st.st_mtime_ns))
        self.assertEqual(cli.paper_text(), 'After')

    def test_invalid_source_is_not_silently_replaced_by_pdf(self):
        self.source.write_bytes(b'\xff')
        with self.assertRaises(UnicodeDecodeError):
            cli.paper_text()

    def extract(self, args, **kwargs):
        Path(args[-1]).write_text(Path(args[2]).read_bytes().decode(), encoding='utf-8')
        return subprocess.CompletedProcess(args, 0, '', '')

    def test_legacy_fallback_is_labelled_and_cached_by_content(self):
        self.pdf.write_bytes(b'Old PDF text')
        st = self.pdf.stat()
        with patch.object(cli.subprocess, 'run', side_effect=self.extract) as run:
            first = cli.paper_text()
            self.assertIn('SOURCE IMPORT REQUIRED', first)
            self.assertIn('historical v31 PDF, not a current editable draft', first)
            self.assertTrue(first.endswith('Old PDF text'))
            self.assertEqual(cli.paper_text(), first)
            self.assertEqual(run.call_count, 1)
            self.pdf.write_bytes(b'New PDF text')
            os.utime(self.pdf, ns=(st.st_atime_ns, st.st_mtime_ns))
            self.assertTrue(cli.paper_text().endswith('New PDF text'))
            self.assertEqual(run.call_count, 2)

    def test_missing_paper_is_explicit(self):
        with self.assertRaisesRegex(SystemExit, 'historical PDF is also absent'):
            cli.paper_text()

    def onboarding(self, delta_docs):
        self.write('docs/paper-delta.cub', 'paper-delta : List Document\npaper-delta =\n  ' +
                   ''.join(f'cons (document "{d}")\n  (' for d in delta_docs) + 'nil' + ')' * len(delta_docs) + '\n')
        self.write('docs/onboarding.cub', 'import "paper-delta.cub"\nonboarding : List Document\nonboarding =\n'
                   '  cons (document "../AGENTS.md")\n  (cons (document "hyperactivity_v31.pdf")\n'
                   '  (list-append paper-delta\n  (cons (document "word-list.md")\n  nil)))\n')

    def test_pointers_follow_the_value_in_order(self):
        for f in ('AGENTS.md', 'docs/hyperactivity_v31.pdf', 'docs/b.md', 'docs/a.md', 'docs/word-list.md'):
            self.write(f, f)
        self.onboarding(['b.md', 'a.md'])
        got = [os.path.relpath(p, os.path.realpath(self.root)) for p in cli.pointers(cli.ONBOARDING, 'onboarding')]
        self.assertEqual(got, ['AGENTS.md', 'docs/hyperactivity_v31.pdf', 'docs/b.md', 'docs/a.md', 'docs/word-list.md'])

    def test_missing_piece_is_not_silently_skipped(self):
        for f in ('AGENTS.md', 'docs/hyperactivity_v31.pdf', 'docs/word-list.md'):
            self.write(f, f)
        self.onboarding(['missing.md'])
        with self.assertRaisesRegex(SystemExit, 'missing document'):
            cli.pointers(cli.ONBOARDING, 'onboarding')

    def test_piece_cannot_escape_repo(self):
        for f in ('AGENTS.md', 'docs/hyperactivity_v31.pdf', 'docs/word-list.md'):
            self.write(f, f)
        self.onboarding(['../../outside.md'])
        with self.assertRaisesRegex(SystemExit, 'leaves the repository'):
            cli.pointers(cli.ONBOARDING, 'onboarding')

    def test_walk_reads_policy_source_and_newly_pointed_piece_whole(self):
        self.write('AGENTS.md', 'Repository-only instruction')
        self.write('docs/hyperactivity_v31.pdf', 'pdf')
        self.write('docs/word-list.md', 'word')
        self.write('docs/new-piece.md', '# Newly pointed\nImportant new content')
        self.onboarding(['new-piece.md'])
        paper = '\n\n'.join('Paragraph %d with source content.' % i for i in range(10))
        self.source.write_text(paper)
        with patch.object(cli, 'tool', return_value='Actual graph output'), patch.object(cli, 'open_work', lambda: print('open')), patch.object(cli, 'CHUNK', 80):
            chunks = cli.walk_chunks()
        self.assertEqual(chunks[0], ('AGENTS.md', 'Repository-only instruction'))
        paper_chunks = [text for title, text in chunks if title.startswith('docs/Hyperactivity_Current_Draft.md')]
        self.assertGreater(len(paper_chunks), 1)
        self.assertEqual('\n\n'.join(paper_chunks), paper)
        self.assertTrue(all(len(t) <= 80 for _, t in chunks))
        self.assertIn('Important new content', '\n'.join(t for title, t in chunks if title.startswith('docs/new-piece.md')))

    def test_task_graph_failure_does_not_look_like_empty_success(self):
        r = subprocess.CompletedProcess([], 2, 'partial output', 'actual failure')
        with patch.object(cli.subprocess, 'run', return_value=r):
            with self.assertRaisesRegex(SystemExit, 'actual failure'):
                cli.tool('tree')


if __name__ == '__main__':
    unittest.main()

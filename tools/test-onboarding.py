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
            'AGENTS': self.root / 'AGENTS.md',
            'LABS': self.root / 'docs/Hyperactive_Labs.md',
            'WORDS': self.root / 'docs/word-list.md',
            'TO_BE_INCORPORATED': self.root / 'docs/to-be-incorporated-into-paper.md',
            'WELCOME': self.root / 'AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md',
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

    def test_standing_pieces_follow_delta_order_and_deduplicate(self):
        self.write('docs/b.md', '# B\nSecond content')
        self.write('docs/a.md', '# A\nFirst content')
        delta = ('# Delta\n\n## Standing pieces to be incorporated\n'
                 '- B (`docs/b.md`, date).\n- A (`docs/a.md`).\n- B again `docs/b.md`.\n'
                 '## Other section\n`docs/not-a-standing-piece.md`\n')
        items = list(cli.standing_pieces(delta))
        self.assertEqual([p for p, _ in items], ['docs/b.md', 'docs/a.md'])
        self.assertEqual(items[0][1], '# B\nSecond content')

    def test_missing_standing_piece_is_not_silently_skipped(self):
        with self.assertRaisesRegex(SystemExit, 'missing standing piece'):
            list(cli.standing_pieces('## Standing pieces to be incorporated\n`docs/missing.md`'))

    def test_missing_standing_inventory_is_explicit(self):
        with self.assertRaisesRegex(SystemExit, 'missing its Standing pieces'):
            list(cli.standing_pieces('# Delta\nNo inventory'))

    def test_standing_piece_cannot_escape_repo(self):
        with self.assertRaisesRegex(SystemExit, 'leaves the repository'):
            list(cli.standing_pieces('## Standing pieces to be incorporated\n`docs/../../outside.md`'))

    def test_walk_reads_policy_and_newly_registered_document(self):
        self.write('AGENTS.md', 'Repository-only instruction')
        self.write('AI-AGENTS-WELCOME-TO-HYPERACTIVITY.md',
                   '# Welcome\nInstructions\n## Avik\'s words, verbatim\nAuthor text')
        self.write('docs/Hyperactive_Labs.md', '# Initiative\nLocal context')
        self.write('docs/word-list.md', 'word')
        self.write('docs/new-standing-piece.md', '# Newly registered\nImportant new content')
        delta = '# Delta\n\n## Standing pieces to be incorporated\n`docs/new-standing-piece.md`\n'
        self.write('docs/to-be-incorporated-into-paper.md', delta)
        paper = '\n\n'.join('Paragraph %d with source content.' % i for i in range(10))
        self.source.write_text(paper)
        with patch.object(cli, 'tool', return_value='Actual graph output'), patch.object(cli, 'STEP', 80):
            steps = cli.walk_steps()
        self.assertEqual(steps[1][0], 'Repository ownership and agent entry')
        self.assertIn('Repository-only instruction', steps[1][1])
        paper_steps = [text for title, text in steps if title.startswith('The paper, part ')]
        self.assertGreater(len(paper_steps), 1)
        self.assertEqual('\n\n'.join(paper_steps), paper)
        self.assertIn(('docs/new-standing-piece.md', '# Newly registered\nImportant new content'), steps)
        self.write('docs/new-standing-piece.md', '# Newly registered\nChanged now')
        with patch.object(cli, 'tool', return_value='Actual graph output'):
            self.assertIn(('docs/new-standing-piece.md', '# Newly registered\nChanged now'), cli.walk_steps())

    def test_task_graph_failure_does_not_look_like_empty_success(self):
        r = subprocess.CompletedProcess([], 2, 'partial output', 'actual failure')
        with patch.object(cli.subprocess, 'run', return_value=r):
            with self.assertRaisesRegex(SystemExit, 'actual failure'):
                cli.tool('tree')


if __name__ == '__main__':
    unittest.main()

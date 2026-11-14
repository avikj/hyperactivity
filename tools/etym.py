#!/usr/bin/env python3
"""etym — any English text returned augmented with the etymology of its words (Avik, 2026-11-14).

    hyperactive etym "a sentence"        the sentence, each word followed by its origin, then each
                                         word's full etymology and first sense
    hyperactive etym --all "..."         the same, including the common function words
    echo "..." | hyperactive etym        read the text from standard input

The source is the GNU Collaborative International Dictionary of English (GCIDE, built on Webster's
1913, GPL), whose entries give each word's etymology with the senses of its roots ("[F., fr. L.
mentalis, fr. mens, mentis, the mind]"). It is fetched once into build/gcide and indexed into
build/etym-index.pickle. A word the dictionary lacks is taken apart into a known base and its
prefixes and suffixes, each with its own etymology; a root the etymology points to ("See Mind") is
followed one step.
"""
import html, os, pickle, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'build', 'gcide')
INDEX = os.path.join(ROOT, 'build', 'etym-index.pickle')
REPO = 'https://github.com/gnu-mirror-unofficial/gcide'

STOP = set('''a an the and or but nor so yet of to in on at by for with from as into onto upon is
are was were be been being am do does did has have had i you he she it we they me him her us them my
your his its our their this that these those there here not no if then than too very just also'''.split())


MARKS = {'acute': 'ACUTE', 'grave': 'GRAVE', 'cir': 'CIRCUMFLEX', 'circ': 'CIRCUMFLEX', 'uml': 'DIAERESIS',
         'til': 'TILDE', 'tilde': 'TILDE', 'cedil': 'CEDILLA', 'mac': 'MACRON', 'macr': 'MACRON',
         'breve': 'BREVE', 'ring': 'RING ABOVE', 'caron': 'CARON', 'dot': 'DOT ABOVE'}
LIGATURES = {'ae': 'æ', 'AE': 'Æ', 'oe': 'œ', 'OE': 'Œ', 'eth': 'ð', 'thorn': 'þ', 'THORN': 'Þ',
             'ETH': 'Ð', 'aring': 'å', 'oslash': 'ø', 'Oslash': 'Ø', 'szlig': 'ß'}


def letter(m):
    """GCIDE's accented letters (<eacute/, <ouml/, <ae/ …) as the letters themselves"""
    import unicodedata
    name = m.group(1)
    if name in LIGATURES:
        return LIGATURES[name]
    for mark, uname in MARKS.items():
        if name.endswith(mark) and len(name) == len(mark) + 1:
            c = name[0]
            try:
                return unicodedata.lookup(f"LATIN {'CAPITAL' if c.isupper() else 'SMALL'} LETTER {c.upper()} WITH {uname}")
            except KeyError:
                return c
    return ''


def greek(m):
    """GCIDE's Greek transcription, read as a transliteration: rough breathing as h, accents dropped"""
    w = re.sub(r'(^|\s)"', r'\1h', m.group(1))
    return re.sub(r"[`'~=|]", '', w)


def clean(s):
    s = re.sub(r'<grk>(.*?)</grk>', greek, s, flags=re.S)
    s = re.sub(r'<([A-Za-z]+)/', letter, s)
    s = s.replace('<br/', ' ')
    s = re.sub(r'<[^>]*>', '', s)
    s = re.sub(r'<[?\w]*/', '', s)
    s = html.unescape(s)
    return re.sub(r'\s+', ' ', s).strip()


def first(tag, s):
    m = re.search(r'<%s>(.*?)</%s>' % (tag, tag), s, re.S)
    return m.group(1) if m else ''


def build_index():
    if not os.path.isdir(SRC):
        os.makedirs(os.path.dirname(SRC), exist_ok=True)
        print('etym: fetching the dictionary (GCIDE) once …', file=sys.stderr)
        subprocess.run(['git', 'clone', '-q', '--depth', '1', REPO, SRC], check=True)
    index = {}
    for f in sorted(os.listdir(SRC)):
        if not re.fullmatch(r'CIDE\.[A-Z]', f):
            continue
        text = open(os.path.join(SRC, f), encoding='latin-1').read()
        for block in text.split('<p><ent>')[1:]:
            ent = clean(block.split('</ent>', 1)[0])
            key = re.sub(r'[*"`\']', '', ent).lower()
            for chunk in block.split('<hw>')[1:]:
                sense = {
                    'pos': clean(first('pos', chunk)),
                    'ety': clean(first('ety', chunk)).strip('[] '),
                    'def': clean(first('def', chunk)),
                    'fld': clean(first('fld', chunk.split('<def>', 1)[0])),
                    'see': [clean(x) for x in re.findall(r'(?:See|From|fr\.) <er>(.*?)</er>', first('ety', chunk))],
                }
                if sense['ety'] or sense['def']:
                    index.setdefault(key, []).append(sense)
    with open(INDEX, 'wb') as fh:
        pickle.dump(index, fh)
    return index


def load():
    if os.path.exists(INDEX):
        with open(INDEX, 'rb') as fh:
            return pickle.load(fh)
    return build_index()


def senses(index, w):
    """the word's senses with an etymology, general senses before field-specific ones"""
    ss = [s for s in index.get(w, []) if s['ety']]
    return sorted(ss, key=lambda s: bool(s['fld']))


INFLECT = [('ies', 'y'), ('ied', 'y'), ('es', ''), ('s', ''), ('ed', ''), ('ed', 'e'), ('d', ''),
           ('ing', ''), ('ing', 'e'), ('ly', ''), ('ily', 'y')]
SUFFIX = ['ization', 'ation', 'ness', 'ment', 'ity', 'ism', 'ist', 'ize', 'ise', 'ion', 'ive', 'al',
          'ic', 'ous', 'ful', 'less', 'able', 'ible', 'er', 'or', 'ship', 'hood', 'dom', 'ology', 'logy']
PREFIX = ['hyper', 'super', 'under', 'over', 'inter', 'trans', 'counter', 'anti', 'post', 'pre',
          'non', 'sub', 'mis', 'dis', 'un', 're', 'de', 'co', 'meta', 'para', 'proto', 'pseudo',
          'neuro', 'homo', 'auto', 'tele', 'micro', 'macro', 'multi', 'poly', 'mono']


def resolve(index, w, depth=0):
    """[(part, senses)] explaining w: itself, an inflection of it, or its prefixes, base and suffixes"""
    if senses(index, w):
        return [(w, senses(index, w))]
    for end, rep in INFLECT:
        if w.endswith(end) and len(w) - len(end) >= 3:
            b = w[:-len(end)] + rep
            if senses(index, b):
                return [(b, senses(index, b))]
    if depth > 2:
        return []
    for p in PREFIX:
        if w.startswith(p) and len(w) - len(p) >= 3:
            rest = resolve(index, w[len(p):], depth + 1)
            if rest:
                return [(p + '-', senses(index, p + '-'))] + rest
    for s in SUFFIX:
        if w.endswith(s) and len(w) - len(s) >= 3:
            stem = w[:-len(s)]
            for b in (stem, stem + 'e', stem[:-1] if stem[-1:] == stem[-2:-1] else None, stem + 'y',
                      stem[:-1] + 'y' if stem.endswith('i') else None):
                if b:
                    base = resolve(index, b, depth + 1)
                    if base:
                        return base + [('-' + s, senses(index, '-' + s))]
    return []


def head(ety):
    """the immediate origin and the first step past it, without the cross-references"""
    out = []
    for c in ety.split(';')[:2]:
        c = re.split(r'(?:[:,.]\s*(?:cf\.|Cf\.|See)\s)', c)[0]
        c = re.sub(r'[.\s]+\d+(?:,\s*\d+)*\s*$', '', c).strip(' .,')
        if len(c) > 70:
            c = c[:c.rfind(', ', 0, 70)] + ' …' if ', ' in c[:70] else c[:70] + ' …'
        if c and not re.match(r'(Cf\.|cf\.|See)\b', c):
            out.append(c)
    return '; '.join(out) or ety.split(';')[0].strip()


def augment(text, all_words=False):
    index = load()
    words = re.findall(r"[A-Za-z][A-Za-z'-]*", text)
    seen, inline, detail = set(), text, []
    for w in words:
        lw = w.lower().strip("'-")
        if lw in seen or (not all_words and lw in STOP):
            continue
        seen.add(lw)
        parts = resolve(index, lw)
        if not parts:
            continue
        def gloss1(p, ss):
            g = head(ss[0]['ety'])
            if re.match(r'(From|See|Cf\.)', g) and ss[0]['see']:
                rs = senses(index, re.sub(r'[*"`\']', '', ss[0]['see'][0]).lower())
                if rs:
                    g += ' ← ' + head(rs[0]['ety'])
            return ('' if p == lw else p + ': ') + g
        gloss = ' + '.join(gloss1(p, ss) if ss else p for p, ss in parts)
        inline = re.sub(r"(?<![A-Za-z'-])%s(?![A-Za-z'-])" % re.escape(w), w + ' ⟨' + gloss + '⟩', inline, count=1)
        lines = [w]
        for p, ss in parts:
            if p != lw:
                lines.append('  ' + p)
            shown = []
            for s in ss:
                if s['ety'] in shown:
                    continue
                shown.append(s['ety'])
                fld = f" ({s['fld'].strip('()')})" if s['fld'] else ''
                lines.append(f"    [{s['ety']}]{fld}")
                if s['def']:
                    lines.append(f"      {s['def'][:240]}")
                for r in s['see'][:2]:
                    rs = senses(index, re.sub(r'[*"`\']', '', r).lower())
                    if rs:
                        lines.append(f"      → {r}: [{rs[0]['ety']}]")
        detail.append('\n'.join(lines))
    return inline + '\n\n' + '\n\n'.join(detail)


def main():
    args = sys.argv[1:]
    all_words = '--all' in args
    args = [a for a in args if a != '--all']
    text = ' '.join(args) if args else sys.stdin.read()
    if not text.strip():
        sys.exit(__doc__)
    print(augment(text, all_words))


if __name__ == '__main__':
    main()

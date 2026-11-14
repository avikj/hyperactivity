#!/usr/bin/env python3
"""etym — any English text returned augmented with the etymology of its words (Avik, 2026-11-14).

    hyperactive etym "a sentence"        the sentence, each word followed by ⟨its immediate origin
                                         and that origin's sense ← its deepest root and that root's
                                         sense⟩, then each word's first sense, its full etymology,
                                         and the chain of origins followed back to the root
    hyperactive etym --all "..."         the same, including the common function words
    echo "..." | hyperactive etym        read the text from standard input

SOURCE. English Wiktionary (snapshot 2026-10-01), as rendered by MediaWiki itself in the Wikimedia
Enterprise HTML dumps and packaged per language by xxyzz/wiktionary_stardict (GitHub release
20261029, StarDict files; code GPL-3.0, content CC BY-SA 4.0 from Wiktionary's contributors). This
is the same data kaikki.org/wiktextract extracts (kaikki.org is not reachable from these sessions;
GitHub release downloads are). The rendered etymology keeps Wiktionary's markup for every mentioned
term: its language, the term in its own script (Greek as Greek), its transliteration and its
gloss. Of 817,247 English headwords, 450,125 have an etymology, coinages to the present included. An
origin in Latin, Ancient Greek, French, Old English, Arabic, Sanskrit … is followed into that
language's own Wiktionary entries (the 50 languages in LANGS) until the chain ends: for 200,829 of
the 450,125 at a Proto-Indo-European root, for 257,930 at a root in some reconstructed
proto-language. Wiktionary's reconstruction pages are not in the dumps, so a root's sense is the
gloss the entries citing it give it most often.
Fallback: the GNU Collaborative International Dictionary of English (GCIDE, Webster's 1913, GPL),
for a word Wiktionary lacks. A word neither has is taken apart into prefixes, a base and suffixes.

Everything is fetched once into build/ (release files by HTTPS from github.com, GCIDE and, where no
zstd decompressor exists, facebook/zstd by git clone, about 600 MB in all) and indexed into
build/etym.sqlite (about 2.3 GB, some 7 minutes once); a run after that reads only the index, in
a fraction of a second.
"""
import gzip, html, json, os, pickle, re, shutil, sqlite3, struct, subprocess, sys, tarfile, unicodedata

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, 'build')
DB = os.path.join(BUILD, 'etym.sqlite')
WIKT = os.path.join(BUILD, 'wiktionary')
RELEASE = 'https://github.com/xxyzz/wiktionary_stardict/releases/download/20261029/'
GCIDE_SRC = os.path.join(BUILD, 'gcide')
GCIDE_INDEX = os.path.join(BUILD, 'etym-index.pickle')
GCIDE_REPO = 'https://github.com/gnu-mirror-unofficial/gcide'
ZSTD_REPO = 'https://github.com/facebook/zstd'

# English, then the languages English etymologies cite most (counted over the English entries),
# whose own entries carry the chain further back
LANGS = '''en la enm ang grc fr fro de nl it es frm sv non sa ru ar ja sco ga da is nds pl he fy ro
dum hi fa pt goh cy zh gd cs ms yi el ko nn ur af sh got tr hy tl ota pi'''.split()
# mention language codes whose entries live in another language's file
LANG_OF = {'cmn': 'zh', 'yue': 'zh', 'nan-hbl': 'zh', 'wuu': 'zh', 'hak': 'zh', 'ltc': 'zh', 'och': 'zh',
           'no': 'nb', 'nb': 'nb', 'arz': 'ar', 'apc': 'ar', 'ajp': 'ar', 'acm': 'ar'}

STOP = set('''a an the and or but nor so yet of to in on at by for with from as into onto upon is
are was were be been being am do does did has have had i you he she it we they me him her us them my
your his its our their this that these those there here not no if then than too very just also'''.split())


def key(w):
    """a page name as Wiktionary links it: macrons and breves dropped (Latin īnfāns → infans), lowercase"""
    d = unicodedata.normalize('NFD', w)
    return unicodedata.normalize('NFC', d.replace('̄', '').replace('̆', '')).lower()


def text(s):
    s = re.sub(r'<style\b.*?</style>', '', s, flags=re.S)
    s = re.sub(r'</p>\s*<p>', ' ', s)
    s = html.unescape(re.sub(r'<[^>]+>', '', s)).replace('‎', '')
    return re.sub(r'\s+', ' ', s).strip()


# ---------------------------------------------------------------------------------------- fetching

def zstd_cmd():
    z = shutil.which('zstd')
    if z:
        return [z, '-qdc']
    z = os.path.join(BUILD, 'zstd-src', 'programs', 'zstd')
    if not os.path.exists(z):
        print('etym: building a zstd decompressor (facebook/zstd) once …', file=sys.stderr)
        if not os.path.isdir(os.path.dirname(os.path.dirname(z))):
            subprocess.run(['git', 'clone', '-q', '--depth', '1', ZSTD_REPO, os.path.dirname(os.path.dirname(z))], check=True)
        subprocess.run(['make', '-s', '-C', os.path.dirname(z), 'zstd-release'], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return [z, '-qdc']


def fetch(lang):
    """build/wiktionary/LANG-en/ with the StarDict index, synonym and dictionary files; False if absent"""
    d = os.path.join(WIKT, f'{lang}-en')
    if os.path.exists(os.path.join(d, f'{lang}-en.idx')):
        return True
    os.makedirs(d, exist_ok=True)
    tz = d + '.tar.zst'
    r = subprocess.run(['curl', '-fsSL', '-o', tz, RELEASE + f'{lang}-en.tar.zst'], stderr=subprocess.DEVNULL)
    if r.returncode != 0:
        os.rmdir(d)
        return False
    p = subprocess.Popen(zstd_cmd() + [tz], stdout=subprocess.PIPE)
    with tarfile.open(fileobj=p.stdout, mode='r|') as tar:
        for m in tar:
            if m.isfile() and re.search(r'\.(idx|syn|dict\.dz)$', m.name):
                with tar.extractfile(m) as src, open(os.path.join(d, os.path.basename(m.name)), 'wb') as dst:
                    shutil.copyfileobj(src, dst)
    p.wait()
    os.remove(tz)
    return True


# ---------------------------------------------------------------------------------------- indexing

MENTION = re.compile(
    r'<(i|b|span) class="((?:[^"]*\s)?mention(?:\s[^"]*)?)" lang="([^"]+)"[^>]*>(.*?)</\1>'
    r'((?:\s|<span> </span>)*<span class="mention-gloss-paren annotation-paren">\(</span>.*?'
    r'<span class="mention-gloss-paren annotation-paren">\)</span>)?', re.S)
ETYL = re.compile(r'<span class="etyl">([^<]*(?:<a[^>]*>[^<]*</a>[^<]*)*)</span>(?=\s*(?:<span> </span>)?\s*<(?:i|b|span) class="[^"]*mention)', re.S)
SKIP = re.compile(r'\b(?:[Cc]ognates?|[Cc]ompar\w*|[Cc]f\.|[Dd]oublets?|[Rr]elated|[Ss]ee|[Mm]ore at|[Aa]kin|'
                  r'[Dd]isplac\w*|[Rr]eplac\w*|[Ss]upersed\w*|[Ss]ynonym\w*|[Aa]nonym\w*|[Ww]hence|'
                  r'[Uu]nrelated|[Pp]iecewise|[Dd]escendants?|[Cc]ontrast|[Pp]aralle\w*)\b')


def mentions(frag):
    """the etymology paragraph as plain text and its chain: the terms it derives the word from, in order,
    each [joining words, language code, language name, term, page, transliteration, gloss]; terms
    cited only for comparison (cognates, doublets, 'compare', 'whence' …) are left out"""
    found = []

    def mark(m):
        inner, paren = m.group(4), m.group(5) or ''
        t = re.search(r'title="([^"]*)"', inner)
        tr = re.search(r'class="[^"]*mention-tr[^"]*"[^>]*>(.*?)</span>', paren, re.S)
        gl = re.findall(r'<span class="mention-gloss">(.*?)</span>', paren, re.S)
        term = text(inner)
        found.append([m.group(3), term, html.unescape(t.group(1)) if t else term,
                      text(tr.group(1)) if tr else '', '; '.join(text(g) for g in gl)])
        return '\x00%d\x00' % (len(found) - 1)
    s = ETYL.sub(lambda m: '\x01' + text(m.group(1)) + '\x01', frag)
    s = text(MENTION.sub(mark, s))
    # remove comparisons: to the closing parenthesis if inside one, else to the end of the sentence
    while True:
        m = SKIP.search(s)
        if not m:
            break
        p, depth = m.start(), s[:m.start()].count('(') - s[:m.start()].count(')')
        if depth > 0:
            e, d = p, depth
            while e < len(s) and d > 0:
                d += {'(': 1, ')': -1}.get(s[e], 0)
                e += 1
            e = e - 1 if d == 0 else e
        else:
            n = re.search(r'\.(?=\s|$)', s[m.end():])
            e = m.end() + n.end() if n else len(s)
        s = s[:p] + s[e:]
    # the chain: terms joined by derivation (from, of, +, root …); a term joined otherwise ('to
    # translate German …', a later sentence on another matter) and the list it heads are left out
    chain, last, active = [], 0, True
    for m in re.finditer(r'\x00(\d+)\x00', s):
        before = s[last:m.start()]
        name = re.search(r'\x01([^\x01]*)\x01\s*$', before)
        plain = re.sub(r'\x01[^\x01]*\x01', '', before)
        bare = re.search(r'(?:^|(?<=\s))((?:(?:Late|Early|Old|Middle|Modern|Medieval|Vulgar|New|Ancient|Classical|Byzantine|Ecclesiastical|Proto-[\w-]+|Anglo-[\w]+)\s)*[A-Z][a-z]+(?:-[A-Z][a-z]+)*)\s*$', plain)
        if bare and not name and not re.fullmatch(r'From|The|This|Its|Compare|See', bare.group(1)):
            name, plain = bare, plain[:bare.start()]
        if last and re.search(r'\.(\s|$)', plain):
            after = re.split(r'\.(?:\s|$)', plain)[-1].strip()
            active = bool(re.match(r'(?:(?:The|This|That|Its|Ultimately|Further|Ultimate)\b.*\b(?:from|borrowed|derived|connected to)|(?:Ultimately )?[Ff]rom)\b', after))
        elif not last:
            active = True
        elif re.search(r'(?:\bfrom|\bof|\broot|\bstem|\bvia|\bthrough|\+|<|\(|\bequivalent to|\bconnected to|\btraced to|\bback to)\s*$', plain.strip()):
            active = True
        elif not re.search(r'(?:,|\band|\bor|/)\s*$', plain.strip()):
            active = False
        last = m.end()
        if active:
            code, term, page, tr, gl = found[int(m.group(1))]
            chain.append([plain.strip()[-24:], code, name.group(1) if name else '', term, page, tr, gl])
    return chain


def articles(d, lang):
    idx = open(os.path.join(d, f'{lang}-en.idx'), 'rb').read()
    heads, i = [], 0
    while i < len(idx):
        j = idx.index(b'\0', i)
        off, sz = struct.unpack('>II', idx[j + 1:j + 9])
        heads.append((off, sz, idx[i:j].decode()))
        i = j + 9
    syn = []
    if os.path.exists(os.path.join(d, f'{lang}-en.syn')):
        s, i = open(os.path.join(d, f'{lang}-en.syn'), 'rb').read(), 0
        while i < len(s):
            j = s.index(b'\0', i)
            syn.append((s[i:j].decode(), heads[struct.unpack('>I', s[j + 1:j + 5])[0]][2]))
            i = j + 5
    with gzip.open(os.path.join(d, f'{lang}-en.dict.dz')) as f:
        for off, sz, w in sorted(heads):
            f.seek(off)
            yield w, f.read(sz).decode()
    yield None, syn


def parse(a):
    pos = re.search(r'<h4>(.*?)</h4>', a)
    sense = re.search(r'<ol>\s*<li[^>]*>(.*?)(?=<dl>|<ul>|<ol>|</li>)', a, re.S)
    formof = ''
    if sense and (re.search(r'form-of', sense.group(1)) or
                  re.match(r'(?:\([^)]*\)\s*)?(?:Of(?:,| or) (?:pertaining|relating)|(?:Relating|Pertaining) to|'
                           r'[\w\s-]{0,40}\b(?:form|spelling|plural|participle|tense|abbreviation|initialism|acronym|'
                           r'clipping|diminutive|superlative|comparative|ellipsis|contraction) of)', text(sense.group(1)), re.I)):
        links = re.findall(r'href="bword://([^"]+)"', sense.group(1))
        formof = html.unescape(links[-1]) if links else ''
    ety = re.search(r'<h4>Etymology[^<]*</h4>(.*?)</section>', a, re.S)
    chain, etext = [], ''
    if ety:
        paras = re.findall(r'<p>(.*?)</p>', ety.group(1), re.S) or [ety.group(1)]
        etext = '\n'.join(text(p) for p in paras if text(p))
        for p in paras:
            chain = mentions(p)
            if chain:
                break
    return (text(pos.group(1)) if pos else '', text(sense.group(1))[:300] if sense else '', etext[:3000],
            json.dumps(chain, ensure_ascii=False) if chain else '', formof)


def build_db():
    os.makedirs(BUILD, exist_ok=True)
    print('etym: fetching English Wiktionary and the languages it derives from, once …', file=sys.stderr)
    tmp = DB + '.tmp'
    if os.path.exists(tmp):
        os.remove(tmp)
    db = sqlite3.connect(tmp)
    db.execute('create table e (lang text, key text, head text, pos text, def text, ety text, chain text, formof text)')
    db.execute('create table f (lang text, key text, lemma text)')
    db.execute('create table n (code text primary key, value text)')
    names = {}
    for lang in LANGS:
        if not fetch(lang):
            continue
        rows, syn, n = [], [], 0
        for w, a in articles(os.path.join(WIKT, f'{lang}-en'), lang):
            if w is None:
                syn = a
                break
            rows.append((lang, key(w), w) + parse(a))
            for m in json.loads(rows[-1][6] or '[]'):
                if m[2]:
                    c = names.setdefault(m[1], {})
                    c[m[2]] = c.get(m[2], 0) + 1
            if len(rows) == 20000:
                db.executemany('insert into e values (?,?,?,?,?,?,?,?)', rows)
                n, rows = n + len(rows), []
        db.executemany('insert into e values (?,?,?,?,?,?,?,?)', rows)
        n += len(rows)
        db.executemany('insert into f values (?,?,?)', [(lang, key(s), l) for s, l in syn if key(s) != key(l)])
        db.commit()
        print(f'etym: {lang}: {n} entries', file=sys.stderr)
    db.executemany('insert into n values (?,?)', [(c, max(v, key=v.get)) for c, v in names.items()])
    db.execute('create index ei on e (lang, key)')
    db.execute('create index fi on f (lang, key)')
    roots_table(db)
    db.commit()
    db.close()
    os.replace(tmp, DB)


def roots_table(db):
    """r: the sense of each reconstructed term (no dictionary has its page), the gloss the entries
    citing it give most often"""
    db.execute('drop table if exists r')
    db.execute('create table r (code text, term text, gloss text, primary key (code, term))')
    seen = {}
    for (c,) in db.execute("select chain from e where chain like '%*%'"):
        for m in json.loads(c):
            if m[3].startswith('*') and m[6]:
                g = seen.setdefault((m[1], m[3]), {})
                g[m[6]] = g.get(m[6], 0) + 1
    db.executemany('insert into r values (?,?,?)', [(c, t, max(g, key=g.get)) for (c, t), g in seen.items()])
    db.commit()


# ---------------------------------------------------------------------------------------- GCIDE

MARKS = {'acute': 'ACUTE', 'grave': 'GRAVE', 'cir': 'CIRCUMFLEX', 'circ': 'CIRCUMFLEX', 'uml': 'DIAERESIS',
         'til': 'TILDE', 'tilde': 'TILDE', 'cedil': 'CEDILLA', 'mac': 'MACRON', 'macr': 'MACRON',
         'breve': 'BREVE', 'ring': 'RING ABOVE', 'caron': 'CARON', 'dot': 'DOT ABOVE'}
LIGATURES = {'ae': 'æ', 'AE': 'Æ', 'oe': 'œ', 'OE': 'Œ', 'eth': 'ð', 'thorn': 'þ', 'THORN': 'Þ',
             'ETH': 'Ð', 'aring': 'å', 'oslash': 'ø', 'Oslash': 'Ø', 'szlig': 'ß'}


def letter(m):
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


def gclean(s):
    s = re.sub(r'<grk>(.*?)</grk>', lambda m: re.sub(r"[`'~=|]", '', re.sub(r'(^|\s)"', r'\1h', m.group(1))), s, flags=re.S)
    s = re.sub(r'<([A-Za-z]+)/', letter, s)
    s = s.replace('<br/', ' ')
    s = re.sub(r'<[^>]*>', '', s)
    s = re.sub(r'<[?\w]*/', '', s)
    return re.sub(r'\s+', ' ', html.unescape(s)).strip()


def gcide():
    """{word: [{pos, ety, def}]} from GCIDE, the 1913 fallback"""
    if os.path.exists(GCIDE_INDEX):
        with open(GCIDE_INDEX, 'rb') as fh:
            idx = pickle.load(fh)
        if idx and isinstance(next(iter(idx.values()))[0], dict):
            return idx
    if not os.path.isdir(GCIDE_SRC):
        subprocess.run(['git', 'clone', '-q', '--depth', '1', GCIDE_REPO, GCIDE_SRC], check=True)
    idx = {}
    for f in sorted(os.listdir(GCIDE_SRC)):
        if not re.fullmatch(r'CIDE\.[A-Z]', f):
            continue
        for block in open(os.path.join(GCIDE_SRC, f), encoding='latin-1').read().split('<p><ent>')[1:]:
            k = re.sub(r'[*"`\']', '', gclean(block.split('</ent>', 1)[0])).lower()
            for chunk in block.split('<hw>')[1:]:
                g = lambda t: gclean((re.search(r'<%s>(.*?)</%s>' % (t, t), chunk, re.S) or [0, ''])[1])
                s = {'pos': g('pos'), 'ety': g('ety').strip('[] '), 'def': g('def')}
                if s['ety']:
                    idx.setdefault(k, []).append(s)
    with open(GCIDE_INDEX, 'wb') as fh:
        pickle.dump(idx, fh)
    return idx


# ---------------------------------------------------------------------------------------- lookup

class Etym:
    def __init__(self):
        if not os.path.exists(DB):
            build_db()
        self.db = sqlite3.connect(DB)
        if not self.db.execute("select 1 from sqlite_master where name='r'").fetchone():
            roots_table(self.db)
        self.langs = {r[0] for r in self.db.execute('select distinct lang from e')}
        self._gcide = None

    def gcide(self):
        if self._gcide is None:
            self._gcide = gcide()
        return self._gcide

    def file_of(self, code):
        c = LANG_OF.get(code, code)
        if c in self.langs:
            return c
        c = c.split('-')[0]
        return c if c in self.langs else None

    def entries(self, lang, w, exact=None):
        """the entries of page w in lang (a word form is taken to its lemma), those with an etymology
        first, the exact capitalisation first"""
        rows = self.db.execute('select head, pos, def, ety, chain, formof from e where lang=? and key=?', (lang, key(w))).fetchall()
        if not rows:
            for (lemma,) in self.db.execute('select lemma from f where lang=? and key=?', (lang, key(w))).fetchall()[:1]:
                rows = self.db.execute('select head, pos, def, ety, chain, formof from e where lang=? and key=?', (lang, key(lemma))).fetchall()
        if exact is not None:
            rows = [r for r in rows if r[0] == exact]
        return sorted(rows, key=lambda r: (not r[3], r[0] != w))

    def with_ety(self, lang, w, exact=None, hops=2):
        """entries of w that have an etymology, following 'plural of' / 'participle of' to the lemma"""
        rows = self.entries(lang, w, exact)
        good = [r for r in rows if r[3]]
        if good or hops == 0:
            return good
        for r in rows:
            if r[5] and key(r[5]) != key(w):
                g = self.with_ety(lang, r[5], hops=hops - 1)
                if g:
                    return g
        return []

    def name(self, m):
        if m[2]:
            return m[2]
        r = self.db.execute('select value from n where code=?', (m[1],)).fetchone()
        return r[0] if r else m[1]

    def sense(self, code, page):
        if page.startswith('*'):
            r = self.db.execute('select gloss from r where code=? and term=?', (code, page)).fetchone()
            return r[0] if r else ''
        f = self.file_of(code)
        if not f:
            return ''
        for r in self.entries(f, page):
            d = re.sub(r'^(?:\([^)]*\)\s*)+', '', r[2]).strip()
            if d:
                return d
        return ''

    def show(self, m, sense=True, width=60):
        """Language term (transliteration) “gloss”"""
        _, code, nm, term, page, tr, gl = m
        if not gl and sense:
            gl = self.sense(code, page)
        gl = gl if len(gl) <= width else gl[:gl.rfind(' ', 0, width)] + ' …'
        return f"{nm or self.name(m)} {term}" + (f' ({tr})' if tr else '') + (f' “{gl}”' if gl else '')

    def follow(self, lang, row, limit=8):
        """the chain of origins of an entry, followed through each origin's own entry:
        [(mention, its entry's etymology text)], ending at the deepest root the data has"""
        hops, seen = [], {(lang, key(row[0]))}
        chain = json.loads(row[4]) if row[4] else []
        while chain and len(hops) < limit:
            cands = [m for m in reversed(chain) if not re.match(r'-|.*-$', m[3]) and m[1] != 'en'] + \
                    [m for m in reversed(chain) if not re.match(r'-|.*-$', m[3]) and m[1] == 'en']
            nxt = None
            for m in cands:
                f = self.file_of(m[1])
                if f and (f, key(m[4])) not in seen:
                    rs = self.with_ety(f, m[4])
                    if rs:
                        nxt = (m, f, rs[0])
                        break
            if not nxt:
                break
            m, f, r = nxt
            seen.add((f, key(m[4])))
            hops.append((m, r))
            chain = json.loads(r[4]) if r[4] else []
        return hops

    def deepest(self, row, hops):
        """the term at the end of the followed chain: a Proto-Indo-European root where the data reaches
        one, else another reconstructed root, else the last attested term"""
        best, rank = None, 0
        for chain in [json.loads(row[4]) if row[4] else []] + [json.loads(r[4]) if r[4] else [] for _, r in hops]:
            for m in chain:
                if m[1] == 'en' or m[3].lstrip('*').startswith('-') or (not m[3].startswith('*') and m[3].endswith('-')):
                    continue
                r = 3 if m[1] == 'ine-pro' else 2 if m[1].endswith('-pro') or m[3].startswith('*') else 1
                if r >= rank:
                    best, rank = m, r
        return best


def coined(ety):
    m = re.search(r'\b(?:[Cc]oined|[Ff]irst (?:used|attested|recorded)|[Ii]ntroduced)\b[^.;]*?\b(1[0-9]{3}|20[0-9]{2})\b', ety)
    return m.group(0) if m and len(m.group(0)) < 110 else ''


def gloss_wikt(E, w, row, depth=0):
    chain = json.loads(row[4]) if row[4] else []
    if len(chain) == 1 and chain[0][1] == 'en' and chain[0][4] != row[0] and not depth:
        rs = E.with_ety('en', chain[0][4], exact=chain[0][4]) or E.with_ety('en', chain[0][4])
        if rs and not (len(json.loads(rs[0][4] or '[]')) == 1 and json.loads(rs[0][4])[0][1] == 'en'):
            g, hops = gloss_wikt(E, chain[0][4], rs[0], 1)
            return chain[0][3] + ': ' + g, [(chain[0], rs[0])] + hops
    hops = E.follow('en', row)
    first = [m for m in chain if m[1] not in ('en', 'enm')] or [m for m in chain if m[1] != 'en']
    if first:
        g = E.show(first[0])
    elif chain:
        g = ' '.join((m[0] + ' ' if m[0] in ('+', '+‎') or m[0].endswith('+') else '') + m[3] for m in chain[:3]).strip()
    else:
        g = row[3].split('\n')[0]
        g = g if len(g) < 90 else g[:g.rfind(' ', 0, 90)] + ' …'
    root = E.deepest(row, hops)
    if root and (not first or root[1:5] != first[0][1:5]):
        g += ' … ← ' + E.show(root)
    c = coined(row[3])
    if c:
        g += '; ' + c[0].lower() + c[1:]
    return g, hops


def parts(E, row):
    """'= in- “not” + fāns “speaking”' for an etymology that joins parts with +"""
    chain = json.loads(row[4]) if row[4] else []
    for i, m in enumerate(chain):
        if m[0].endswith('+') and i:
            group = [chain[i - 1], m]
            for x in chain[i + 1:]:
                if not x[0].endswith('+'):
                    break
                group.append(x)
            return ' = ' + ' + '.join(E.show(x, width=40) for x in group)
    return ''


def detail_wikt(E, w, rows, hops):
    lines = []
    shown = []
    for i, r in enumerate(rows):
        if r[3] in shown or len(shown) == 3:
            continue
        shown.append(r[3])
        lines.append(f"    {('(' + r[1].lower() + ') ') if r[1] else ''}{r[2]}")
        for para in r[3].split('\n'):
            lines.append(f'    [{para[:1200]}]')
        if parts(E, r):
            lines.append('     ' + parts(E, r))
        if i == 0:
            for m, h in hops:
                first_para = h[3].split('\n')[0]
                lines.append(f'      ← {E.show(m)}: {first_para[:400]}{parts(E, h)}')
    return lines


INFLECT = [('ies', 'y'), ('ied', 'y'), ('s', ''), ('es', ''), ('ed', ''), ('ed', 'e'), ('d', ''),
           ('ing', ''), ('ing', 'e'), ('ly', ''), ('ily', 'y')]
SUFFIX = ['ization', 'ation', 'ness', 'ment', 'ity', 'ism', 'ist', 'ize', 'ise', 'ion', 'ive', 'al',
          'ic', 'ous', 'ful', 'less', 'able', 'ible', 'er', 'or', 'ship', 'hood', 'dom', 'ology', 'logy']
PREFIX = ['hyper', 'super', 'under', 'over', 'inter', 'trans', 'counter', 'anti', 'post', 'pre',
          'non', 'sub', 'mis', 'dis', 'un', 're', 'de', 'co', 'meta', 'para', 'proto', 'pseudo',
          'neuro', 'homo', 'auto', 'tele', 'micro', 'macro', 'multi', 'poly', 'mono']


def one(E, w, exact=None):
    """('wikt', rows) or ('gcide', senses) for w itself or an inflection of it, else None"""
    rows = E.with_ety('en', w, exact)
    if rows:
        return ('wikt', rows)
    if exact is None:
        for end, rep in INFLECT:
            if w.endswith(end) and len(w) - len(end) >= 3:
                rows = E.with_ety('en', w[:-len(end)] + rep)
                if rows:
                    return ('wikt', rows)
    g = [s for s in E.gcide().get(w, []) if s['ety']]
    if g:
        return ('gcide', g)
    return None


def resolve(E, w, depth=0):
    """[(part, found)] explaining w: itself, or its prefixes, base and suffixes"""
    r = one(E, w)
    if r:
        h = r[1][0][0] if r[0] == 'wikt' else w
        return [(w if depth or w.startswith(key(h)[:max(3, len(h) - 2)]) else key(h), r)]
    if depth > 2:
        return []
    for p in PREFIX:
        if w.startswith(p) and len(w) - len(p) >= 3:
            rest = resolve(E, w[len(p):], depth + 1)
            if rest:
                return [(p + '-', one(E, p + '-'))] + rest
    for s in SUFFIX:
        if w.endswith(s) and len(w) - len(s) >= 3:
            stem = w[:-len(s)]
            for b in (stem, stem + 'e', stem[:-1] if stem[-1:] == stem[-2:-1] else None, stem + 'y',
                      stem[:-1] + 'y' if stem.endswith('i') else None):
                if b:
                    base = resolve(E, b, depth + 1)
                    if base:
                        return base + [('-' + s, one(E, '-' + s))]
    return []


def augment(text_in, all_words=False):
    E = Etym()
    seen, out, detail, last = set(), [], [], 0
    for m in re.finditer(r"[A-Za-z][A-Za-z'’-]*", text_in):
        w = m.group(0)
        lw = w.lower().strip("'’-")
        if re.search(r"['’]", lw) or lw in seen or (not all_words and lw in STOP):
            continue
        seen.add(lw)
        proper = w[0].isupper() and m.start() > 0 and not re.search(r'[.!?:"“]\s*$', text_in[:m.start()])
        if proper:
            r = one(E, w, exact=w) or one(E, lw, exact=w)
            parts = [(lw, r)] if r else []
        else:
            parts = resolve(E, lw)
        if not parts:
            continue
        glosses, lines = [], [w]
        for p, found in parts:
            if p != lw:
                lines.append('  ' + p)
            if not found:
                glosses.append(p)
                continue
            kind, rows = found
            if kind == 'wikt':
                # an entry that only says 'variant of' itself yields to a homograph with more to say
                own = [r for r in rows if not (r[4] and all(m[1] == 'en' and key(m[4]) == key(p) for m in json.loads(r[4])))]
                rows = own + [r for r in rows if r not in own]
                g, hops = gloss_wikt(E, p, rows[0])
                lines += detail_wikt(E, p, rows, hops)
            else:
                g = rows[0]['ety'].split(';')[0][:80] + ' (1913)'
                for s in rows[:3]:
                    lines.append(f"    ({s['pos']}) {s['def'][:240]}")
                    lines.append(f"    [{s['ety']}]  (Webster 1913)")
            glosses.append(('' if p == lw else p + ': ') + g)
        out.append(text_in[last:m.end()] + ' ⟨' + ' + '.join(glosses) + '⟩')
        last = m.end()
        detail.append('\n'.join(lines))
    out.append(text_in[last:])
    return ''.join(out) + '\n\n' + '\n\n'.join(detail)


def main():
    args = sys.argv[1:]
    all_words = '--all' in args
    args = [a for a in args if a != '--all']
    t = ' '.join(args) if args else sys.stdin.read()
    if not t.strip():
        sys.exit(__doc__)
    print(augment(t, all_words))


if __name__ == '__main__':
    main()

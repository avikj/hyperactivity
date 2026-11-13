#!/usr/bin/env python3
"""mir2cub.py — Rust's MIR into the checker (track R, tracks/R-rust.md).

    python3 tools/mir2cub.py experiments/rust/book.rs -o experiments/rust/book.cub [--entry on_message]

Writes one .cub file whose last definition, `<stem>-crate : RCrate` (lib/lang/rust/ast.cub), is
every body reachable from the entry, with the layouts, vtables and statics those bodies use.

Where each part comes from (rustc 1.97.0; nothing here is computed by a second route):

  bodies     `rustc --edition=2021 --crate-type=lib -O -Cpanic=abort --emit=mir`. The text is MIR
             before monomorphization: a body in a generic impl is printed once with its
             parameters. `-O` runs the MIR inliner, which brings the bodies of the `language-implementation`
             functions used into the crate's own (rustc prints no MIR for another crate).
             A call that still names a function outside the crate stops the import.
  impls      a body in an impl is named by the impl's span; the impl header (generics, trait,
             self type) is read from the source text at that span, and a call
             (`Book::<8>::add`, `<T as Trait>::m`) is connected to its body by matching the
             header's self type against the call's.
  instances  walked from the entry: each call's generic arguments under the caller's. The list is
             refused unless every instance is in rustc's own `-Zprint-mono-items` list.
  layouts    a copy of the source with one function per needed type appended is compiled with
             `-g -Copt-level=0`, and `llvm-dwarfdump --debug-info` is read: size, alignment, fields with
             offsets and types, variants with their tag values.
  vtables    from the same object: the relocations of the vtable's section (`objdump -r`), its
             size and align words (`objdump -s`), symbols demangled by `llvm-cxxfilt`.
  statics    the evaluated allocation rustc prints after the bodies.

  drops      for each type a reachable Drop terminator drops, and each type inside it, the Drop
             impl in this crate if there is one (RDropImpl); a type of another crate stops the import.
  fn items   a function's own generic parameters are read from its declaration in the source.

A statement, rvalue, terminator, type, constant or layout form not handled stops the import with
a message naming it (class Unhandled); nothing is emitted as unknown. Not imported, by decision:
debug information (`debug` lines, scopes, `// DBG` notes), lifetimes, and the bodies rustc
evaluates at compile time (constants, static initializers, the CTFE copies of const fns), whose
results are in the text as values.
"""
import argparse, os, re, shutil, subprocess, sys, tempfile

RUSTC = os.environ.get('RUSTC', '/root/.cargo/bin/rustc')
RUSTC_FLAGS = ['--edition=2021', '--crate-type=lib', '-O', '-Cpanic=abort']             # the MIR
OBJECT_FLAGS = ['--edition=2021', '--crate-type=lib', '-Copt-level=0', '-Cpanic=abort']  # the object read for layouts and vtables


class Unhandled(Exception):
    pass


def stop(what, text):
    raise Unhandled('mir2cub: unhandled %s: %s' % (what, text))


INTS = ['i8', 'i16', 'i32', 'i64', 'isize', 'u8', 'u16', 'u32', 'u64', 'usize']
INT_BITS = {'i8': 8, 'i16': 16, 'i32': 32, 'i64': 64, 'isize': 64, 'u8': 8, 'u16': 16, 'u32': 32, 'u64': 64, 'usize': 64}
M64 = (1 << 64) - 1

# ------------------------------------------------------------------------------------------
# Types. Python form:
#   ('bool',) ('int',k) ('never',) ('ref',mut,T) ('ptr',mut,T) ('array',T,len) ('slice',T)
#   ('tuple',(T..)) ('adt',name,(arg..)) ('dyn',trait) ('fnptr',(T..),ret)
#   ('param',i) ('const',n) ('cparam',i)
# An ADT or trait is identified by the last segment of its path; two different paths with the
# same last segment stop the import (Paths.note).


class Paths:
    """The paths seen for each last segment: to refuse two items of one name, and to spell a
    type in Rust source (the spelling the MIR text used)."""

    def __init__(self, crate):
        self.crate = crate
        self.full = {}      # name → segments without the leading crate (std, core, alloc, or this crate's name)
        self.spelled = {}   # name → the longest spelling met in the MIR text
        self.origin = 'mir'

    def note(self, segs):
        name = segs[-1]
        if self.origin == 'mir' and len(segs) > len(self.spelled.get(name, [])):
            self.spelled[name] = list(segs)
        if segs[0] in ('std', 'core', 'alloc', self.crate):
            segs = segs[1:]
        old = self.full.get(name)
        if old is None or len(segs) > len(old):
            self.full[name] = list(segs)
        if old is not None:
            # rustc prints a path as short as it can be in context (`Option`,
            # `std::option::Option`; the debug information says `core::option::Option`), and a
            # re-export leaves out modules (`std::ptr::NonNull` is `core::ptr::non_null::NonNull`).
            # So two spellings of one item are one a subsequence of the other; anything else is
            # two items of one name, which this importer does not tell apart.
            a, b = (old, segs) if len(old) <= len(segs) else (segs, old)
            it = iter(b)
            if not all(x in it for x in a):
                stop('path (two items with the same last segment)', '%s and %s' % ('::'.join(old), '::'.join(segs)))
        return name


class P:
    """A cursor over a string."""

    def __init__(self, s, i=0):
        self.s = s
        self.i = i

    def rest(self):
        return self.s[self.i:]

    def peek(self, t):
        return self.s.startswith(t, self.i)

    def eat(self, t):
        if self.s.startswith(t, self.i):
            self.i += len(t)
            return True
        return False

    def need(self, t):
        if not self.eat(t):
            stop('syntax (wanted %r)' % t, '%s ⟨here⟩ %s' % (self.s[:self.i], self.s[self.i:]))

    def ws(self):
        while self.i < len(self.s) and self.s[self.i] == ' ':
            self.i += 1

    def m(self, rx):
        r = re.compile(rx).match(self.s, self.i)
        if r:
            self.i = r.end()
        return r

    def end(self):
        return self.i >= len(self.s)


IDENT = r'[A-Za-z_][A-Za-z0-9_]*'


class TypeParser:
    def __init__(self, paths, generics=()):
        self.paths = paths
        self.generics = list(generics)  # [(kind, name)], kind 'type' | 'const'
        self.seen = []                  # every type read, in order

    def gen_index(self, name):
        for i, (k, n) in enumerate(self.generics):
            if n == name:
                return i, k
        return None

    def ty(self, p):
        t = self.ty1(p)
        self.seen.append(t)
        return t

    def ty1(self, p):
        p.ws()
        if p.eat('&'):
            p.m(r"'" + IDENT + r" ?")
            mut = bool(p.m(r'mut '))
            return ('ref', mut, self.ty(p))
        if p.eat('*const '):
            return ('ptr', False, self.ty(p))
        if p.eat('*mut '):
            return ('ptr', True, self.ty(p))
        if p.eat('!'):
            return ('never',)
        if p.eat('['):
            t = self.ty(p)
            if p.eat(']'):
                return ('slice', t)
            p.need('; ')
            n = self.const_arg(p)
            p.need(']')
            return ('array', t, n)
        if p.eat('('):
            ts = []
            while not p.eat(')'):
                ts.append(self.ty(p))
                p.eat(',')
                p.ws()
            return ('tuple', tuple(ts))
        if p.eat('dyn '):
            segs, args = self.path(p)
            if args:
                stop('type (dyn trait with generic arguments)', p.s)
            if p.peek(' + '):
                stop('type (dyn with more than one bound)', p.s)
            return ('dyn', self.paths.note(segs))
        if p.peek('fn('):
            p.need('fn(')
            ts = []
            while not p.eat(')'):
                ts.append(self.ty(p))
                p.eat(',')
                p.ws()
            ret = ('tuple', ())
            if p.eat(' -> '):
                ret = self.ty(p)
            return ('fnptr', tuple(ts), ret)
        if p.peek('{closure') or p.peek('{coroutine'):
            stop('type (closure or coroutine type in a reachable position)', p.rest())
        if p.peek('<'):
            stop('type (qualified path / associated type)', p.rest())
        r = re.compile(IDENT).match(p.s, p.i)
        if not r:
            stop('type', '%s ⟨here⟩ %s' % (p.s[:p.i], p.rest()))
        segs, args = self.path(p)
        if len(segs) == 1 and not args:
            n = segs[0]
            if n == 'bool':
                return ('bool',)
            if n in INTS:
                return ('int', n)
            if n in ('i128', 'u128', 'f16', 'f32', 'f64', 'f128', 'char', 'str'):
                stop('type (primitive not handled yet)', n)
            g = self.gen_index(n)
            if g is not None:
                return ('param', g[0]) if g[1] == 'type' else ('cparam', g[0])
        return ('adt', self.paths.note(segs), tuple(args))

    def const_arg(self, p):
        r = p.m(r'(\d+)(?:_usize)?')
        if r:
            return ('const', int(r.group(1)))
        r = p.m(IDENT)
        if r:
            g = self.gen_index(r.group(0))
            if g is not None and g[1] == 'const':
                return ('cparam', g[0])
        stop('constant generic argument', p.s)

    def generic_args(self, p):
        """after '<': the arguments up to the matching '>', lifetimes dropped."""
        args = []
        while not p.eat('>'):
            p.ws()
            if p.m(r"'" + IDENT):
                pass
            elif re.compile(r'\d').match(p.s, p.i):
                args.append(self.const_arg(p))
            elif p.peek('-') or p.peek('true') or p.peek('false') or p.peek('{'):
                stop('constant generic argument (not a usize literal)', p.s)
            else:
                args.append(self.ty(p))
            p.eat(',')
            p.ws()
        return args

    def path(self, p):
        """A::B::<args>::C<args> → (segments, the generic arguments, which must be on one segment)."""
        segs, args, argseg = [], [], None
        while True:
            r = p.m(IDENT)
            if not r:
                stop('path', '%s ⟨here⟩ %s' % (p.s[:p.i], p.rest()))
            segs.append(r.group(0))
            if p.eat('::<') or (p.peek('<') and p.eat('<')):
                a = self.generic_args(p)
                if a and args:
                    stop('path (generic arguments on two segments)', p.s)
                if a:
                    args, argseg = a, len(segs) - 1
            if p.peek('::') and re.compile('::' + IDENT).match(p.s, p.i):
                p.need('::')
                continue
            break
        self.last_argseg = argseg
        return segs, args


def subst(t, args):
    k = t[0]
    if k in ('param', 'cparam'):
        if t[1] >= len(args):
            stop('substitution (parameter %d of %d arguments)' % (t[1], len(args)), repr(t))
        return args[t[1]]
    if k in ('ref', 'ptr'):
        return (k, t[1], subst(t[2], args))
    if k == 'array':
        return (k, subst(t[1], args), subst(t[2], args))
    if k == 'slice':
        return (k, subst(t[1], args))
    if k == 'tuple':
        return (k, tuple(subst(x, args) for x in t[1]))
    if k == 'adt':
        return (k, t[1], tuple(subst(x, args) for x in t[2]))
    if k == 'fnptr':
        return (k, tuple(subst(x, args) for x in t[1]), subst(t[2], args))
    return t


def is_closed(t):
    k = t[0]
    if k in ('param', 'cparam'):
        return False
    return all(is_closed(x) for x in subterms(t))


def subterms(t):
    k = t[0]
    if k in ('ref', 'ptr'):
        return [t[2]]
    if k == 'array':
        return [t[1], t[2]]
    if k == 'slice':
        return [t[1]]
    if k == 'tuple':
        return list(t[1])
    if k == 'adt':
        return list(t[2])
    if k == 'fnptr':
        return list(t[1]) + [t[2]]
    return []


def unify(pat, t, env):
    """match a pattern type (with parameters) against a type; env: parameter index → type."""
    if pat[0] in ('param', 'cparam'):
        if pat[1] in env:
            return env[pat[1]] == t
        env[pat[1]] = t
        return True
    if pat[0] != t[0]:
        return False
    if pat[0] in ('ref', 'ptr'):
        return pat[1] == t[1] and unify(pat[2], t[2], env)
    if pat[0] == 'adt':
        return pat[1] == t[1] and len(pat[2]) == len(t[2]) and all(unify(a, b, env) for a, b in zip(pat[2], t[2]))
    if pat[0] == 'tuple':
        return len(pat[1]) == len(t[1]) and all(unify(a, b, env) for a, b in zip(pat[1], t[1]))
    if pat[0] == 'array':
        return unify(pat[1], t[1], env) and unify(pat[2], t[2], env)
    if pat[0] == 'slice':
        return unify(pat[1], t[1], env)
    return pat == t


# ------------------------------------------------------------------------------------------
# The MIR text: items, then each reachable body.

class Item:
    pass


def split_top(s, sep=','):
    """split at separators outside (), [], <>, {} and string literals."""
    out, depth, cur, i, instr = [], 0, '', 0, False
    while i < len(s):
        c = s[i]
        if instr:
            cur += c
            if c == '\\':
                cur += s[i + 1]
                i += 1
            elif c == '"':
                instr = False
        elif c == '"':
            instr = True
            cur += c
        elif c in '([{<':
            depth += 1
            cur += c
        elif c in ')]}':
            depth -= 1
            cur += c
        elif c == '>' and not (i > 0 and s[i - 1] in '-='):
            depth -= 1
            cur += c
        elif c == sep and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += c
        i += 1
    if cur.strip():
        out.append(cur.strip())
    return out


class Mir:
    def __init__(self, text, src_path, src_text, paths):
        self.paths = paths
        self.src_path = src_path
        self.src_lines = src_text.split('\n')
        self.src_text = src_text
        # the source with comments, string and character literals blanked (same offsets), for
        # finding declarations and matching braces
        self.code_text = re.sub(r'//[^\n]*|/\*.*?\*/|b?"(?:\\.|[^"\\])*"|b?\'(?:\\.|[^\'\\])\'',
                                lambda m: re.sub(r'[^\n]', ' ', m.group(0)), src_text, flags=re.S)
        self.items = []       # every fn body of the dump (unparsed)
        self.statics = {}     # name → (mut, type text)
        self.allocs = {}      # alloc name → (static name, size, align, [byte|None])
        self.impls = {}       # span → impl header
        self.split(text)

    def split(self, text):
        lines = text.split('\n')
        i, ctfe = 0, False
        while i < len(lines):
            l = lines[i]
            if l.startswith('// MIR FOR CTFE'):
                ctfe = True
            elif l.startswith('fn '):
                j = i
                while lines[j] != '}':
                    j += 1
                if not ctfe:   # the body the compiler evaluates at compile time is not the one that runs
                    it = Item()
                    it.header, it.lines = l, lines[i + 1:j]
                    self.items.append(it)
                ctfe = False
                i = j
            elif l.startswith('static '):
                r = re.match(r'static (mut )?(.*?): (.*) = \{$', l)
                if not r:
                    stop('static header', l)
                self.statics[r.group(2)] = (bool(r.group(1)), r.group(3))
                while lines[i] != '}':
                    i += 1
            elif l.startswith('const ') or l.startswith('promoted'):
                if l.endswith('{'):
                    while lines[i] != '}':
                        i += 1
            elif l.startswith('alloc'):
                r = re.match(r'(alloc\d+) \((?:static: (.*?), )?size: (\d+), align: (\d+)\) \{(.*)$', l)
                if not r:
                    stop('allocation header', l)
                body = [r.group(5)]
                if not l.rstrip().endswith('}'):
                    i += 1
                    while lines[i] != '}':
                        body.append(lines[i])
                        i += 1
                bs = []
                for b in body:
                    b = b.strip().rstrip('}').strip()
                    if not b:
                        continue
                    parts = b.split('│')
                    data = parts[1] if len(parts) >= 3 else parts[0]
                    for tok in data.split():
                        if tok == '__':
                            bs.append(None)
                        elif re.fullmatch(r'[0-9a-f]{2}', tok):
                            bs.append(int(tok, 16))
                        else:
                            stop('allocation contents (a pointer or other non-byte)', l + ' … ' + tok)
                if len(bs) != int(r.group(3)):
                    stop('allocation (read %d bytes of %s)' % (len(bs), r.group(3)), l)
                self.allocs[r.group(1)] = (r.group(2), int(r.group(3)), int(r.group(4)), bs)
            elif l.startswith('//') or l == '':
                pass
            else:
                stop('top-level item of the MIR dump', l)
            i += 1
        for it in self.items:
            self.read_header(it)

    # -- impl headers, read from the source at the span rustc names the impl by

    def impl_at(self, span):
        if span in self.impls:
            return self.impls[span]
        r = re.fullmatch(r'(.*):(\d+):(\d+): (\d+):(\d+)', span)
        if not r:
            stop('impl span', span)
        l1, c1, l2, c2 = (int(r.group(k)) for k in (2, 3, 4, 5))
        if os.path.abspath(r.group(1)) != os.path.abspath(self.src_path):
            stop('impl span in another file', span)
        ls = self.src_lines[l1 - 1:l2]
        ls[-1] = ls[-1][:c2 - 1]
        ls[0] = ls[0][c1 - 1:]
        text = ' '.join(x.strip() for x in ls)
        imp = Item()
        imp.span, imp.text = span, text
        imp.end_offset = sum(len(x) + 1 for x in self.src_lines[:l2 - 1]) + c2 - 1
        if text.startswith('impl'):
            p = P(text, 4)
            imp.generics = self.generics_decl(p) if p.peek('<') else []
            p.ws()
            tp = TypeParser(self.paths, imp.generics)
            if ' where ' in text:
                stop('impl header (where clause)', text)
            rest = p.rest()
            parts = rest.split(' for ')
            if len(parts) == 2:
                q = P(parts[0])
                segs, args = tp.path(q)
                if args or not q.end():
                    stop('impl header (trait with generic arguments)', text)
                imp.trait = self.paths.note(segs)
                q = P(parts[1])
            elif len(parts) == 1:
                imp.trait = None
                q = P(parts[0])
            else:
                stop('impl header', text)
            imp.self_ty = tp.ty(q)
            if not q.end():
                stop('impl header (after the self type)', text)
        else:
            # a derive: the span is the trait's name inside #[derive(..)]; the impl is for the
            # item the attribute is on, which is the next struct/enum/union in the source.
            if not re.fullmatch(IDENT, text):
                stop('impl span text (neither an impl header nor a derived trait)', text)
            off = sum(len(x) + 1 for x in self.src_lines[:l2 - 1]) + c2 - 1
            r2 = re.compile(r'\b(struct|enum|union)\s+(' + IDENT + r')\s*(<[^>{(;]*>)?').search(self.src_text, off)
            if not r2:
                stop('derive (no item after the attribute)', span)
            imp.generics = self.generics_decl(P(r2.group(3))) if r2.group(3) else []
            imp.trait = text
            imp.self_ty = ('adt', self.paths.note([r2.group(2)]),
                           tuple(('param', i) if k == 'type' else ('cparam', i) for i, (k, n) in enumerate(imp.generics)))
        self.impls[span] = imp
        return imp

    def generics_decl(self, p):
        p.need('<')
        depth, j = 1, p.i
        while depth:
            c = p.s[j]
            depth += c == '<'
            depth -= c == '>' and p.s[j - 1] != '-'
            j += 1
        inner, p.i = p.s[p.i:j - 1], j
        gens = []
        for g in split_top(inner):
            if g.startswith("'"):
                continue
            r = re.fullmatch(r'const (' + IDENT + r')\s*:\s*usize', g)
            if r:
                gens.append(('const', r.group(1)))
                continue
            if g.startswith('const '):
                stop('generic parameter (a constant not of type usize)', g)
            r = re.match(r'(' + IDENT + r')\s*(:.*)?$', g)
            if not r:
                stop('generic parameter', g)
            gens.append(('type', r.group(1)))
        return gens

    def read_header(self, it):
        h = it.header
        r = re.match(r'fn (.*?)\((_1: .*)?\) -> (.*) \{$', h)
        if not r:
            stop('body header', h)
        it.name = r.group(1)
        it.arg_texts = [a.split(': ', 1)[1] for a in split_top(r.group(2))] if r.group(2) else []
        it.ret_text = r.group(3)
        it.impl = None
        r = re.match(r'<impl at (.*?:\d+:\d+: \d+:\d+)>::(.*)$', it.name)
        if r:
            it.span, it.method = r.group(1), r.group(2)
        else:
            it.span, it.method = None, it.name

    def generics_of(self, it):
        """the body's generic parameters: the impl's, or for a free function those of its
        declaration in the source (the MIR text prints neither)."""
        if it.span is None:
            name = it.name.split('::')[-1]
            if not re.fullmatch(IDENT, name):
                return []
            ds = [m for m in re.finditer(r'\bfn\s+' + name + r'\b\s*(<)?', self.code_text)
                  if not any(a <= m.start() < b for (a, b) in self.item_blocks())]
            if len(ds) != 1:
                stop('function declaration (%d declarations of this name outside impls in the source)' % len(ds), it.name)
            if not ds[0].group(1):
                return []
            return self.generics_decl(P(self.code_text, ds[0].end() - 1))
        imp = self.impl_at(it.span)
        if imp.text.startswith('impl'):
            # a method's own parameters come after the impl's; they are not read yet
            a, b = self.block_after(imp.end_offset)
            m = re.compile(r'\bfn\s+' + re.escape(it.method.split('::')[0]) + r'\b\s*(<)?').search(self.code_text, a, b)
            if m is None:
                stop('method declaration (not found in the impl block)', it.name)
            if m.group(1) and [g for g in split_top(self.code_text[m.end():self.code_text.index('>', m.end())]) if not g.strip().startswith("'")]:
                stop('generic parameters on a method of an impl (only the impl\'s and a free function\'s are read)', it.name)
        return imp.generics

    def block_after(self, off):
        """(start, end) of the {...} block that opens first after off, in the source with
        comments and literals blanked."""
        t = self.code_text
        a = t.index('{', off)
        depth, j = 0, a
        while True:
            depth += t[j] == '{'
            depth -= t[j] == '}'
            j += 1
            if depth == 0:
                return a, j

    def item_blocks(self):
        if not hasattr(self, '_blocks'):
            self._blocks = []
            for m in re.finditer(r'\b(impl|trait)\b[^;{]*\{', self.code_text):
                if not any(a <= m.start() < b for (a, b) in self._blocks):
                    self._blocks.append(self.block_after(m.start()))
        return self._blocks


class BodyParser:
    """One body's text into Python data (variants still by name)."""

    def __init__(self, mir, it):
        self.mir, self.it = mir, it
        self.tp = TypeParser(mir.paths, mir.generics_of(it))

    def ty_text(self, s):
        p = P(s)
        t = self.tp.ty(p)
        if not p.end():
            stop('type (trailing text)', s)
        return t

    def run(self):
        it = self.it
        it.generics = self.tp.generics
        if '{closure' in it.name or '{coroutine' in it.name:
            stop('body kind (a closure body reached by a call)', it.name)
        locals_ = {}
        for i, a in enumerate(it.arg_texts):
            locals_[i + 1] = ('arg', self.ty_text(a))
        it.argc = len(it.arg_texts)
        blocks, cur = {}, None
        for raw in it.lines:
            l = raw.strip()
            if not l:
                continue
            if cur is None:
                r = re.fullmatch(r'let (mut )?_(\d+): (.*);', l)
                if r:
                    locals_[int(r.group(2))] = ('mut' if r.group(1) else 'not', self.ty_text(r.group(3)))
                    continue
                if l.startswith('debug ') or l.startswith('scope ') or l == '}':
                    continue
                r = re.fullmatch(r'bb(\d+)( \(cleanup\))?: \{', l)
                if r:
                    if r.group(2):
                        stop('block kind (cleanup block)', it.name + ' ' + l)
                    cur = int(r.group(1))
                    blocks[cur] = []
                    continue
                stop('line in a body', it.name + ': ' + l)
            if l == '}':
                cur = None
                continue
            if l.startswith('// DBG: '):
                continue   # debug information attached to a statement (which variable a value is); not code
            blocks[cur].append(l)
        n = max(locals_) + 1
        if sorted(locals_) != list(range(n)):
            stop('locals (not numbered 0..n)', it.name)
        it.locals = [locals_[i] for i in range(n)]
        if it.locals[0][1] != self.ty_text(it.ret_text):
            stop('return type (differs from local 0)', it.name)
        nb = max(blocks) + 1
        if sorted(blocks) != list(range(nb)):
            stop('blocks (not numbered 0..n)', it.name)
        it.blocks = []
        it.types = self.tp.seen
        for b in range(nb):
            ls = blocks[b]
            stmts = [self.stmt(x) for x in ls[:-1]]
            it.blocks.append((stmts, self.term(ls[-1])))

    # places, operands, constants

    def place(self, p):
        if p.eat('('):
            if p.eat('*'):
                base = self.place(p)
                p.need(')')
                base = (base[0], base[1] + [('deref',)])
            else:
                base = self.place(p)
                if p.eat(' as '):
                    v = p.m(IDENT).group(0)
                    p.need(')')
                    base = (base[0], base[1] + [('downcast', v)])
                else:
                    r = p.m(r'\.(\d+): ')
                    if not r:
                        stop('place', p.s)
                    t = self.tp.ty(p)
                    p.need(')')
                    base = (base[0], base[1] + [('field', int(r.group(1)), t)])
        else:
            r = p.m(r'_(\d+)')
            if not r:
                stop('place', '%s ⟨here⟩ %s' % (p.s[:p.i], p.rest()))
            base = (int(r.group(1)), [])
        while p.peek('['):
            r = p.m(r'\[_(\d+)\]')
            if r:
                base = (base[0], base[1] + [('index', int(r.group(1)))])
                continue
            r = p.m(r'\[(-?)(\d+) of (\d+)\]')
            if r:
                base = (base[0], base[1] + [('cindex', int(r.group(2)), int(r.group(3)), bool(r.group(1)))])
                continue
            stop('place projection', p.rest())
        return base

    def operand(self, p):
        if p.eat('copy '):
            return ('copy', self.place(p))
        if p.eat('move '):
            return ('move', self.place(p))
        if p.eat('const '):
            return ('const', self.const(p))
        stop('operand', '%s ⟨here⟩ %s' % (p.s[:p.i], p.rest()))

    def int_lit(self, p):
        r = p.m(r'(-?\d+)_(' + '|'.join(INTS) + r')\b')
        if r:
            return ('int', int(r.group(1)), r.group(2))
        return None

    def const(self, p):
        c = self.int_lit(p)
        if c:
            return c
        if re.compile(r'-?\d').match(p.s, p.i):
            stop('constant (numeric literal of a type not handled)', p.rest())
        r = p.m(r'(' + '|'.join(INTS) + r')::(MIN|MAX)\b')
        if r:
            bits, signed = INT_BITS[r.group(1)], r.group(1).startswith('i')
            if r.group(2) == 'MIN':
                return ('int', -(1 << (bits - 1)) if signed else 0, r.group(1))
            return ('int', (1 << (bits - (1 if signed else 0))) - 1, r.group(1))
        if p.m(r'true\b'):
            return ('bool', True)
        if p.m(r'false\b'):
            return ('bool', False)
        if p.eat('{'):
            r = p.m(r'(alloc\d+): ')
            if not r:
                stop('constant', p.s)
            t = self.tp.ty(p)
            p.need('}')
            return ('alloc', r.group(1), t)
        if p.peek('"') or p.peek("'") or p.peek('b"') or p.peek('<') or p.peek('('):
            stop('constant', p.rest())
        save = p.i
        r = p.m(IDENT)
        if r and not p.peek('::') and not p.peek('<') and not p.peek(' {{'):
            g = self.tp.gen_index(r.group(0))
            if g is not None and g[1] == 'const':
                return ('cparam', g[0])
        p.i = save
        segs, args = self.tp.path(p)
        argseg = self.tp.last_argseg
        fields = None
        if p.eat(' {{ '):
            fields = []
            while not p.eat('}}'):
                p.m(IDENT + ': ')
                fields.append(self.const(p))
                p.eat(',')
                p.ws()
        elif p.peek('(') or p.peek(' {') or p.peek('['):
            stop('constant (form not handled)', p.s)
        return ('path', segs, tuple(args), argseg, fields)

    # statements

    def stmt(self, l):
        if not l.endswith(';'):
            stop('statement', l)
        l = l[:-1]
        r = re.fullmatch(r'Storage(Live|Dead)\(_(\d+)\)', l)
        if r:
            return ('live' if r.group(1) == 'Live' else 'dead', int(r.group(2)))
        p = P(l)
        if not (l.startswith('_') or l.startswith('(')):
            stop('statement', l)
        dest = self.place(p)
        if not p.eat(' = '):
            stop('statement', l)
        rv = self.rvalue(p)
        if not p.end():
            stop('rvalue (trailing text)', l)
        return ('assign', dest, rv)

    BINOPS = {'Add': 'add', 'AddUnchecked': 'add-unchecked', 'AddWithOverflow': 'add-with-overflow',
              'Sub': 'sub', 'SubUnchecked': 'sub-unchecked', 'SubWithOverflow': 'sub-with-overflow',
              'Mul': 'mul', 'MulUnchecked': 'mul-unchecked', 'MulWithOverflow': 'mul-with-overflow',
              'Div': 'div', 'Rem': 'rem', 'BitXor': 'bit-xor', 'BitAnd': 'bit-and', 'BitOr': 'bit-or',
              'Shl': 'shl', 'ShlUnchecked': 'shl-unchecked', 'Shr': 'shr', 'ShrUnchecked': 'shr-unchecked',
              'Eq': 'eq', 'Lt': 'lt', 'Le': 'le', 'Ne': 'ne', 'Ge': 'ge', 'Gt': 'gt', 'Cmp': 'cmp', 'Offset': 'offset'}
    UNOPS = {'Not': 'not', 'Neg': 'neg', 'PtrMetadata': 'ptr-metadata'}
    CASTS = {'IntToInt': 'int-to-int', 'Transmute': 'transmute', 'PtrToPtr': 'ptr-to-ptr',
             'PointerCoercion(Unsize, Implicit)': 'unsize', 'PointerCoercion(Unsize, AsCast)': 'unsize',
             'PointerCoercion(ReifyFnPointer(Safe), Implicit)': 'reify-fn-pointer',
             'PointerCoercion(ReifyFnPointer(Safe), AsCast)': 'reify-fn-pointer'}

    def rvalue(self, p):
        if p.eat('&raw const '):
            return ('rawptr', False, self.place(p))
        if p.eat('&raw mut '):
            return ('rawptr', True, self.place(p))
        if p.eat('&mut '):
            return ('ref', 'mut', self.place(p))
        if p.peek('&fake') or p.peek('&uniq') or p.peek('&two_phase'):
            stop('rvalue (borrow kind)', p.s)
        if p.eat('&'):
            return ('ref', 'shared', self.place(p))
        if p.eat('['):
            first = self.operand(p)
            if p.eat('; '):
                n = self.tp.const_arg(p)
                p.need(']')
                return ('repeat', first, n)
            ops = [first]
            while not p.eat(']'):
                p.need(', ')
                ops.append(self.operand(p))
            return ('aggregate', ('array',), ops)
        if p.peek('*const ') or p.peek('*mut '):
            t = self.tp.ty(p)
            p.need(' from (')
            a = self.operand(p)
            p.need(', ')
            b = self.operand(p)
            p.need(')')
            return ('aggregate', ('rawptr', t), [a, b])
        if p.eat('('):
            ops = []
            while not p.eat(')'):
                ops.append(self.operand(p))
                p.eat(',')
                p.ws()
            return ('aggregate', ('tuple',), ops)
        retag = True
        if p.eat('no_retag '):
            retag = False
        if p.peek('copy ') or p.peek('move ') or p.peek('const '):
            op = self.operand(p)
            if p.eat(' as '):
                if not retag:
                    stop('rvalue (no_retag on a cast)', p.s)
                t = self.tp.ty(p)
                p.need(' (')
                kind = p.rest()[:-1]
                if not p.rest().endswith(')') or kind not in self.CASTS:
                    stop('cast kind', p.rest())
                p.i = len(p.s)
                return ('cast', self.CASTS[kind], op, t)
            return ('use', op, retag)
        if not retag:
            stop('rvalue (no_retag)', p.s)
        if p.eat('discriminant('):
            pl = self.place(p)
            p.need(')')
            return ('discriminant', pl)
        r = re.compile(r'(' + IDENT + r')\(').match(p.s, p.i)
        if r and r.group(1) in self.BINOPS:
            p.i = r.end()
            a = self.operand(p)
            p.need(', ')
            b = self.operand(p)
            p.need(')')
            return ('binop', self.BINOPS[r.group(1)], a, b)
        if r and r.group(1) in self.UNOPS:
            p.i = r.end()
            a = self.operand(p)
            p.need(')')
            return ('unop', self.UNOPS[r.group(1)], a)
        if not re.compile(IDENT).match(p.s, p.i):
            stop('rvalue', p.s)
        # an ADT aggregate: Path { f: op, .. } | Path(op, ..) | Path; or a function item cast to a pointer
        segs, args = self.tp.path(p)
        argseg = self.tp.last_argseg
        if p.eat(' as '):
            t = self.tp.ty(p)
            p.need(' (')
            kind = p.rest()[:-1]
            if not p.rest().endswith(')') or self.CASTS.get(kind) != 'reify-fn-pointer':
                stop('cast kind (of a function item)', p.rest())
            p.i = len(p.s)
            return ('cast', 'reify-fn-pointer', ('const', ('fnitem', ('path', segs, tuple(args), argseg))), t)
        ops = []
        if p.eat(' { '):
            while not p.eat('}'):
                if not p.m(IDENT + ': '):
                    stop('rvalue (aggregate field)', p.s)
                ops.append(self.operand(p))
                p.eat(',')
                p.ws()
        elif p.eat('('):
            while not p.eat(')'):
                ops.append(self.operand(p))
                p.eat(',')
                p.ws()
        elif not p.end():
            stop('rvalue', p.s)
        return ('aggregate', ('adt', segs, tuple(args), argseg), ops)

    # terminators

    def unwind(self, s):
        if s == 'unwind continue':
            return ('continue',)
        if s == 'unwind unreachable':
            return ('unreachable',)
        if s.startswith('unwind terminate'):
            return ('terminate',)
        stop('unwind action', s)

    ASSERTS = {'"index out of bounds: the length is {} but the index is {}"': ('bounds-check', 2),
               '"attempt to divide `{}` by zero"': ('division-by-zero', 1),
               '"attempt to calculate the remainder of `{}` with a divisor of zero"': ('remainder-by-zero', 1),
               '"attempt to compute `{} / {}`, which would overflow"': ('overflow rbin-div', 2)}

    def term(self, l):
        if not l.endswith(';'):
            stop('terminator', l)
        l = l[:-1]
        if l == 'return':
            return ('return',)
        if l == 'unreachable':
            return ('unreachable',)
        r = re.fullmatch(r'goto -> bb(\d+)', l)
        if r:
            return ('goto', int(r.group(1)))
        if l.startswith('switchInt('):
            i = l.rindex(') -> [')
            p = P(l[len('switchInt('):i])
            op = self.operand(p)
            if not p.end():
                stop('terminator', l)
            arms, other = [], None
            for a in l[i + 6:-1].split(', '):
                r = re.fullmatch(r'(-?\d+): bb(\d+)', a)
                if r:
                    arms.append((int(r.group(1)), int(r.group(2))))
                    continue
                r = re.fullmatch(r'otherwise: bb(\d+)', a)
                if not r or other is not None:
                    stop('switchInt targets', l)
                other = int(r.group(1))
            if other is None or not l.endswith(']'):
                stop('switchInt targets', l)
            return ('switch', op, arms, other)
        if l.startswith('assert('):
            r = re.fullmatch(r'assert\((.*)\) -> \[success: bb(\d+), (unwind [^\]]*)\]', l)
            if not r:
                stop('terminator', l)
            parts = split_top(r.group(1))
            cond, expected = parts[0], True
            if cond.startswith('!'):
                cond, expected = cond[1:], False
            if parts[1] not in self.ASSERTS:
                stop('assert message', parts[1])
            kind, n = self.ASSERTS[parts[1]]
            if len(parts) != 2 + n:
                stop('assert operands', l)
            ops = []
            for x in [cond] + parts[2:]:
                p = P(x)
                ops.append(self.operand(p))
                if not p.end():
                    stop('assert operand', x)
            return ('assert', ops[0], expected, kind, ops[1:], int(r.group(2)), self.unwind(r.group(3)))
        r = re.fullmatch(r'drop\((.*)\) -> \[return: bb(\d+), (unwind [^\]]*)\]', l)
        if r:
            p = P(r.group(1))
            pl = self.place(p)
            if not p.end():
                stop('terminator', l)
            return ('drop', pl, int(r.group(2)), self.unwind(r.group(3)))
        r = re.fullmatch(r'(.*) -> \[return: bb(\d+), (unwind [^\]]*)\]', l)
        if r and (l.startswith('_') or l.startswith('(')):
            p = P(r.group(1))
            dest = self.place(p)
            p.need(' = ')
            callee = self.callee(p)
            p.need('(')
            args = []
            while not p.eat(')'):
                args.append(self.operand(p))
                p.eat(',')
                p.ws()
            if not p.end():
                stop('call', l)
            return ('call', callee, args, dest, int(r.group(2)), self.unwind(r.group(3)))
        stop('terminator', l)

    def callee(self, p):
        if p.peek('copy ') or p.peek('move '):
            return ('ptr', self.operand(p))
        if p.eat('<'):
            t = self.tp.ty(p)
            trait = None
            if p.eat(' as '):
                segs, args = self.tp.path(p)
                if args:
                    stop('call (trait with generic arguments)', p.s)
                trait = self.mir.paths.note(segs)
            p.need('>::')
            r = p.m(IDENT)
            margs = ()
            if p.eat('::<'):
                margs = tuple(self.tp.generic_args(p))
            if margs:
                stop('call (generic arguments on the method)', p.s)
            return ('qual', t, trait, r.group(0))
        segs, args = self.tp.path(p)
        return ('path', segs, tuple(args), self.tp.last_argseg)


# ------------------------------------------------------------------------------------------
# Debug information: layouts.

class Die:
    __slots__ = ('off', 'tag', 'attrs', 'kids', 'parent')


def read_dwarf(obj):
    """llvm-dwarfdump's listing as a tree; a DIE's children are the DIEs indented under it."""
    out = subprocess.run(['llvm-dwarfdump', '--debug-info', obj], capture_output=True, text=True, check=True).stdout
    root = Die()
    root.off, root.tag, root.attrs, root.kids, root.parent = -1, 'root', {}, [], None
    stack, cur, by_off = [(-1, root)], None, {}
    for l in out.split('\n'):
        r = re.match(r'(0x[0-9a-f]+):( +)(DW_TAG_\w+|NULL)$', l)
        if r:
            cur = None
            if r.group(3) == 'NULL':
                continue
            d, ind = Die(), len(r.group(2))
            d.off, d.tag, d.attrs, d.kids = int(r.group(1), 16), r.group(3)[7:], {}, []
            while stack[-1][0] >= ind:
                stack.pop()
            d.parent = stack[-1][1]
            d.parent.kids.append(d)
            stack.append((ind, d))
            by_off[d.off] = d
            cur = d
            continue
        r = re.match(r' +DW_AT_(\w+)\t\((.*)\)$', l)
        if r and cur is not None:
            cur.attrs[r.group(1)] = r.group(2)
    return root, by_off


def at_str(d, a):
    v = d.attrs.get(a)
    if v is None:
        return None
    r = re.fullmatch(r'"(.*)"', v)
    return r.group(1) if r else v


def at_int(d, a):
    v = d.attrs.get(a)
    if v is None:
        return None
    r = re.fullmatch(r'(-?(?:0x[0-9a-f]+|\d+))', v)
    if not r:
        stop('debug information attribute DW_AT_%s' % a, v)
    return int(v, 0)


def at_ref(d, a, by_off):
    v = d.attrs.get(a)
    if v is None:
        return None
    return by_off[int(re.match(r'(0x[0-9a-f]+)', v).group(1), 16)]


class Layouts:
    def __init__(self, obj, paths):
        self.paths = paths
        self.root, self.by_off = read_dwarf(obj)
        self.tp = TypeParser(paths)
        self.index = {}      # type → [die]
        self.adt_kind = {}   # adt name → 'struct' | 'enum'
        self.variants = {}   # enum adt name → [variant names]
        self.walk(self.root)

    def walk(self, d):
        for k in d.kids:
            if k.tag in ('structure_type', 'enumeration_type', 'union_type'):
                name = at_str(k, 'name')
                if name and d.tag != 'structure_type':
                    try:
                        p = P(name)
                        t = self.tp.ty(p)
                        if not p.end():
                            t = None
                    except Unhandled:
                        t = None
                    if t is not None:
                        self.index.setdefault(t, []).append(k)
                        if t[0] == 'adt':
                            kind = 'enum' if (k.tag == 'enumeration_type' or any(x.tag == 'variant_part' for x in k.kids)) else 'struct'
                            if k.tag == 'union_type':
                                kind = 'union'
                            if self.adt_kind.setdefault(t[1], kind) != kind:
                                stop('debug information (one name, two kinds of type)', t[1])
                            if kind == 'enum':
                                vs = self.variant_names(k)
                                if self.variants.setdefault(t[1], vs) != vs:
                                    stop('debug information (one enum name, two variant lists)', t[1])
            self.walk(k)

    def variant_names(self, d):
        if d.tag == 'enumeration_type':
            return [at_str(e, 'name') for e in d.kids if e.tag == 'enumerator']
        vp = [x for x in d.kids if x.tag == 'variant_part'][0]
        return [at_str([m for m in v.kids if m.tag == 'member'][0], 'name') for v in vp.kids if v.tag == 'variant']

    def die_of(self, t):
        ds = self.index.get(t)
        if not ds:
            return None
        if len(set(self.qual(d) for d in ds)) > 1:
            stop('layout (two types of this name in the debug information)', show_ty(t))
        return ds[0]

    def qual(self, d):
        segs = []
        while d is not None and d.tag != 'root':
            if d.tag in ('namespace', 'structure_type', 'enumeration_type', 'subprogram'):
                segs.append(at_str(d, 'name') or '?')
            d = d.parent
        return '::'.join(reversed(segs))

    def ty_of_die(self, d):
        if d.tag == 'array_type':
            sub = [x for x in d.kids if x.tag == 'subrange_type']
            if len(sub) != 1 or at_int(sub[0], 'count') is None:
                stop('debug information (array without one counted range)', hex(d.off))
            return ('array', self.ty_of_die(at_ref(d, 'type', self.by_off)), ('const', at_int(sub[0], 'count')))
        name = at_str(d, 'name')
        if name is None:
            stop('debug information (unnamed %s)' % d.tag, hex(d.off))
        if d.tag == 'base_type' and name == '()':
            return ('tuple', ())
        p = P(name)
        t = self.tp.ty(p)
        if not p.end():
            stop('debug information type name', name)
        return t

    def members(self, d):
        fs = []
        for m in d.kids:
            if m.tag == 'member':
                fs.append((at_str(m, 'name'), at_int(m, 'data_member_location'), self.ty_of_die(at_ref(m, 'type', self.by_off))))
        return fs

    def layout(self, t):
        """→ (size, align, tag, variants); tag None | ('direct'|'niche', offset, size);
        variants [(name, discr, tagvalue|None, [(field name, offset, type)])]."""
        d = self.die_of(t)
        if d is None:
            return None
        size, align = at_int(d, 'byte_size'), at_int(d, 'alignment')
        if size is None or align is None:
            stop('debug information (type without size or alignment)', show_ty(t))
        if d.tag == 'union_type':
            stop('layout (union)', show_ty(t))
        if d.tag == 'enumeration_type':
            under = at_str(at_ref(d, 'type', self.by_off), 'name')
            vs = []
            for e in d.kids:
                if e.tag == 'enumerator':
                    v = at_int(e, 'const_value')
                    bits = INT_BITS[under]
                    v &= (1 << bits) - 1
                    discr = v - (1 << bits) if under.startswith('i') and v >> (bits - 1) else v
                    vs.append((at_str(e, 'name'), discr, v, []))
            return (size, align, ('direct', 0, size), vs)
        vps = [x for x in d.kids if x.tag == 'variant_part']
        if not vps:
            return (size, align, None, [(t[1] if t[0] == 'adt' else '', 0, None, self.members(d))])
        if len(vps) != 1:
            stop('debug information (two variant parts)', show_ty(t))
        vp = vps[0]
        tagm = at_ref(vp, 'discr', self.by_off)
        tagty = at_ref(tagm, 'type', self.by_off)
        tname, tsize, toff = at_str(tagty, 'name'), at_int(tagty, 'byte_size'), at_int(tagm, 'data_member_location')
        if tname not in INT_BITS:
            stop('layout (tag of type %s)' % tname, show_ty(t))
        raw = []
        for v in vp.kids:
            if v.tag != 'variant':
                continue
            m = [x for x in v.kids if x.tag == 'member']
            if len(m) != 1:
                stop('debug information (variant without one member)', show_ty(t))
            if at_int(m[0], 'data_member_location') != 0:
                stop('debug information (variant not at offset 0)', show_ty(t))
            dv = at_int(v, 'discr_value')
            if dv is not None:
                dv &= (1 << (8 * tsize)) - 1
            raw.append((at_str(m[0], 'name'), dv, self.members(at_ref(m[0], 'type', self.by_off))))
        untagged = [x for x in raw if x[1] is None]
        if len(untagged) > 1:
            stop('debug information (two untagged variants)', show_ty(t))
        vs = []
        for i, (name, dv, fs) in enumerate(raw):
            if untagged:
                discr = i
            else:
                bits = 8 * tsize
                discr = dv - (1 << bits) if tname.startswith('i') and dv >> (bits - 1) else dv
            vs.append((name, discr, dv, fs))
        return (size, align, ('niche' if untagged else 'direct', toff, tsize), vs)


def show_ty(t):
    k = t[0]
    if k == 'bool':
        return 'bool'
    if k == 'int':
        return t[1]
    if k == 'never':
        return '!'
    if k == 'ref':
        return '&' + ('mut ' if t[1] else '') + show_ty(t[2])
    if k == 'ptr':
        return '*' + ('mut ' if t[1] else 'const ') + show_ty(t[2])
    if k == 'array':
        return '[%s; %s]' % (show_ty(t[1]), show_ty(t[2]))
    if k == 'slice':
        return '[%s]' % show_ty(t[1])
    if k == 'tuple':
        return '(' + ''.join(show_ty(x) + ', ' for x in t[1]).rstrip(' ' if len(t[1]) != 1 else '').rstrip(',' if len(t[1]) != 1 else '') + ')'
    if k == 'adt':
        return t[1] + ('<' + ', '.join(show_ty(x) for x in t[2]) + '>' if t[2] else '')
    if k == 'dyn':
        return 'dyn ' + t[1]
    if k == 'fnptr':
        return 'fn(' + ', '.join(show_ty(x) for x in t[1]) + ') -> ' + show_ty(t[2])
    if k == 'const':
        return str(t[1])
    return '%s#%d' % (k, t[1])


# ------------------------------------------------------------------------------------------
# The import.

class Import:
    def __init__(self, src, entry):
        self.src = os.path.relpath(src)
        self.src_text = open(src).read()
        self.entry = entry
        self.paths = Paths(os.path.splitext(os.path.basename(src))[0])
        self.tmp = tempfile.mkdtemp(prefix='mir2cub-')
        self.kinds = {'statement': {}, 'rvalue': {}, 'terminator': {}, 'type': {}, 'operand': {}, 'constant': {},
                      'projection': {}, 'cast': {}, 'binop': {}, 'unop': {}, 'aggregate': {}}

    def met(self, cls, k):
        self.kinds[cls][k] = self.kinds[cls].get(k, 0) + 1

    def rustc(self, args, flags=RUSTC_FLAGS):
        r = subprocess.run([RUSTC] + flags + args, capture_output=True, text=True)
        if r.returncode != 0:
            sys.stderr.write(r.stderr)
            raise SystemExit('mir2cub: rustc failed: ' + ' '.join(args))

    def run(self):
        mirf = os.path.join(self.tmp, 'crate.mir')
        self.rustc(['--emit=mir', self.src, '-o', mirf])
        self.mir_text = open(mirf).read()
        self.mir = Mir(self.mir_text, self.src, self.src_text, self.paths)
        self.by_name = {}
        for it in self.mir.items:
            self.by_name.setdefault((it.span, it.method), []).append(it)
        self.bodies = []          # reachable items, in order of first reach: the body ids
        self.instances = []       # (body id, closed args)
        self.inst_seen = set()
        self.vtables_wanted = []  # (closed type, trait)
        self.drops_wanted = []    # closed types a reachable Drop terminator drops
        self.drop_seen = set()
        self.drop_impls = []      # (closed type, body id, args): the type's own Drop impl
        self.statics = []         # alloc names, in order: the static ids
        self.walk_from_entry()
        # the walk of vtable methods needs the object file, and the object file's appended
        # functions need the types the walk met: alternate until nothing is added.
        self.obj = None
        self.vtables = {}         # (type, trait) → (size, align, drop, [(method name, body id, args)])
        while True:
            need = self.needed_types()
            if self.obj is None or not set(need) <= self.obj_types:
                self.compile_object(need)
                self.paths.origin = 'object'
                self.layouts = Layouts(self.obj, self.paths)
                self.paths.origin = 'mir'
            n = len(self.instances)
            for key in list(self.vtables_wanted):
                if key not in self.vtables:
                    self.read_vtable(key)
            for t in list(self.drops_wanted):
                self.drop_glue(t)
            self.drain()
            if len(self.instances) == n and set(self.needed_types()) <= self.obj_types:
                break
        self.check_instances()
        self.paths.origin = 'object'

    def check_instances(self):
        """Refuse an instance list rustc's own does not contain: `-Zprint-mono-items` (an
        unstable flag, hence RUSTC_BOOTSTRAP) prints every function instance the compiler
        generates code for with the MIR's flags; each (body, arguments) found by the walk must
        be one of them. (rustc's list may have more: public functions the entry does not reach.)"""
        r = subprocess.run([RUSTC] + RUSTC_FLAGS + ['-Zprint-mono-items=yes', '--emit=obj', self.src, '-o', os.path.join(self.tmp, 'mono.o')],
                           capture_output=True, text=True, env=dict(os.environ, RUSTC_BOOTSTRAP='1'))
        if r.returncode != 0:
            sys.stderr.write(r.stderr)
            raise SystemExit('mir2cub: rustc -Zprint-mono-items failed')
        theirs, self.mono_items = set(), 0
        for l in r.stdout.split('\n'):
            m = re.match(r'MONO_ITEM fn (.*?) @@ ', l)
            if not m:
                continue
            self.mono_items += 1
            name = m.group(1)
            if ' - shim(' in name:
                continue
            try:
                p = P(name)
                bp = BodyParser.__new__(BodyParser)
                bp.mir, bp.tp = self.mir, TypeParser(self.paths)
                c = bp.callee(p)
                if not p.end():
                    continue
                c = self.resolve_callee(c, lookup_only=True)
            except Unhandled:
                continue   # an instance of another crate's function, or of a body the walk never parsed
            if c is not None:
                theirs.add((c[1], c[2]))
        for (b, args) in self.instances:
            if (b, args) not in theirs:
                stop('instance (reached by the importer\'s walk but not in rustc\'s -Zprint-mono-items list)',
                     '%s at <%s>' % (self.bodies[b].name, ', '.join(show_ty(a) for a in args)))

    def drop_glue(self, t):
        """what dropping a value of the closed type t runs: the Drop impl of each ADT in it that
        has one (a body of this crate), found through the layouts."""
        if t in self.drop_seen:
            return
        self.drop_seen.add(t)
        k = t[0]
        if k in ('bool', 'int', 'never', 'ref', 'ptr', 'fnptr'):
            return
        if k == 'array':
            return self.drop_glue(t[1])
        if k == 'tuple':
            for x in t[1]:
                self.drop_glue(x)
            return
        if k != 'adt':
            stop('drop (of an unsized or unhandled type)', show_ty(t))
        d = self.layouts.die_of(t)
        if d is None:
            stop('layout (the debug information does not describe this type)', show_ty(t))
        if not self.layouts.qual(d).startswith(self.paths.crate + '::'):
            stop('drop (of a type of another crate: its Drop impl, if it has one, is not in the MIR text)', show_ty(t))
        f = self.find_method(t, 'Drop', 'drop')
        if f is not None:
            b = self.body_id(f[0])
            self.drop_impls.append((t, b, f[1]))
            self.add_instance(b, f[1])
        self.paths.origin = 'object'
        lay = self.layouts.layout(t)
        self.paths.origin = 'mir'
        for (_, _, _, fs) in lay[3]:
            for (_, _, ft) in fs:
                self.drop_glue(ft)

    # -- bodies and instances

    def body_id(self, it):
        if it not in self.bodies:
            BodyParser(self.mir, it).run()
            self.bodies.append(it)
        return self.bodies.index(it)

    def find_free(self, segs):
        its = self.by_name.get((None, '::'.join(segs)), [])
        if len(its) > 1:
            stop('call (two bodies named so)', '::'.join(segs))
        return its[0] if its else None

    def find_method(self, self_ty, trait, method):
        """the body of `method` in the impl of `trait` (None: inherent) whose self type matches."""
        found = []
        for it in self.mir.items:
            if it.span is None or it.method != method:
                continue
            imp = self.mir.impl_at(it.span)
            if imp.trait != trait:
                continue
            env = {}
            if unify(imp.self_ty, self_ty, env):
                if sorted(env) != list(range(len(imp.generics))):
                    stop('impl (a parameter the self type does not determine)', imp.text)
                found.append((it, tuple(env[i] for i in range(len(imp.generics)))))
        if len(found) > 1:
            stop('call (two impls match)', '%s::%s' % (show_ty(self_ty), method))
        return found[0] if found else None

    def known_body(self, it, lookup_only):
        if lookup_only and it not in self.bodies:
            stop('instance (of a body the walk did not reach)', it.name)
        return self.body_id(it)

    def resolve_callee(self, c, lookup_only=False):
        """→ ('fn', body id, args in the caller's parameters) | ('dyn', trait, method name) | ('ptr', operand)"""
        if c[0] == 'ptr':
            return c
        if c[0] == 'qual':
            _, t, trait, method = c
            if t[0] == 'dyn':
                if trait != t[1]:
                    stop('call (a supertrait method through dyn)', '<dyn %s as %s>::%s' % (t[1], trait, method))
                return ('dyn', trait, method)
            f = self.find_method(t, trait, method)
            if f is None:
                stop('call (no body in this crate: a function of another crate that rustc did not inline)',
                     '<%s as %s>::%s' % (show_ty(t), trait, method))
            return ('fn', self.known_body(f[0], lookup_only), f[1])
        _, segs, args, argseg = c
        it = self.find_free(segs)
        if it is not None:
            b = self.known_body(it, lookup_only)
            if len(args) != len(it.generics) or (args and argseg != len(segs) - 1):
                stop('call (generic arguments do not match the function\'s parameters)', '::'.join(segs))
            return ('fn', b, tuple(args))
        if len(segs) >= 2 and (argseg is None or argseg == len(segs) - 2):
            self_ty = ('adt', self.paths.note(segs[:-1]), tuple(args))
            f = self.find_method(self_ty, None, segs[-1])
            if f is not None:
                return ('fn', self.known_body(f[0], lookup_only), f[1])
        stop('call (no body in this crate: a function of another crate that rustc did not inline)', '::'.join(segs))

    def walk_from_entry(self):
        it = self.find_free([self.entry])
        if it is None:
            stop('entry (no body of this name)', self.entry)
        self.work = []
        self.add_instance(self.body_id(it), ())
        self.drain()

    def add_instance(self, b, args):
        if not all(is_closed(a) for a in args):
            stop('instance (arguments not closed)', self.bodies[b].name)
        if (b, args) not in self.inst_seen:
            self.inst_seen.add((b, args))
            self.instances.append((b, args))
            self.work.append((b, args))

    def drain(self):
        while self.work:
            b, args = self.work.pop()
            it = self.bodies[b]
            if not hasattr(it, 'resolved'):
                self.resolve_body(it)
            for (cb, cargs) in it.calls:
                self.add_instance(cb, tuple(subst(a, args) for a in cargs))
            for (t, trait) in it.unsizes:
                key = (subst(t, args), trait)
                if key not in self.vtables_wanted:
                    self.vtables_wanted.append(key)
            for t in it.drops:
                t = subst(t, args)
                if t not in self.drops_wanted:
                    self.drops_wanted.append(t)

    def place_ty(self, it, pl):
        t = it.locals[pl[0]][1]
        for pr in pl[1]:
            if pr[0] == 'deref':
                if t[0] not in ('ref', 'ptr'):
                    stop('place (deref of a non-pointer)', show_ty(t))
                t = t[2]
            elif pr[0] == 'field':
                t = pr[2]
            elif pr[0] in ('index', 'cindex'):
                if t[0] not in ('array', 'slice'):
                    stop('place (index of a non-array)', show_ty(t))
                t = t[1]
        return t

    def resolve_body(self, it):
        it.resolved = True
        it.calls, it.unsizes, it.drops = [], [], []
        for bi, (stmts, term) in enumerate(it.blocks):
            for si, s in enumerate(stmts):
                if s[0] == 'assign' and s[2][0] == 'cast' and s[2][1] == 'reify-fn-pointer':
                    c = self.resolve_callee(s[2][2][1][1])
                    stmts[si] = ('assign', s[1], ('cast', 'reify-fn-pointer', ('const', ('fnitem', c)), s[2][3]))
                    it.calls.append((c[1], c[2]))
                if s[0] == 'assign' and s[2][0] == 'cast' and s[2][1] == 'unsize':
                    to = s[2][3]
                    if to[0] in ('ref', 'ptr') and to[2][0] == 'dyn':
                        op = s[2][2]
                        if op[0] == 'const':
                            stop('unsize cast of a constant', it.name)
                        frm = self.place_ty(it, op[1])
                        if frm[0] not in ('ref', 'ptr'):
                            stop('unsize cast to dyn from a non-pointer', show_ty(frm))
                        it.unsizes.append((frm[2], to[2][1]))
                self.scan_statics(s)
            if term[0] == 'call':
                c = self.resolve_callee(term[1])
                it.blocks[bi] = (stmts, ('call', c) + term[2:])
                if c[0] == 'fn':
                    it.calls.append((c[1], c[2]))
            if term[0] == 'drop':
                it.drops.append(self.place_ty(it, term[1]))
            self.scan_statics(term)

    def scan_statics(self, x):
        if isinstance(x, (tuple, list)):
            if len(x) == 3 and x[0] == 'alloc' and isinstance(x[1], str):
                a = self.mir.allocs.get(x[1])
                if a is None or a[0] is None:
                    stop('constant (an allocation that is not a static)', x[1])
                if x[1] not in self.statics:
                    self.statics.append(x[1])
                return
            for y in x:
                self.scan_statics(y)

    # -- the types whose layouts are needed: every ADT instance and tuple in a reachable instance

    def body_types(self, it):
        """every type the body's text mentions (locals, projections, casts, generic arguments)."""
        return it.types

    def needed_types(self):
        need = []

        def add(t):
            if t[0] in ('adt', 'tuple') and t not in need and not (t[0] == 'tuple' and not t[1]):
                need.append(t)
            for s in subterms(t):
                add(s)

        for (b, args) in self.instances:
            for t in self.body_types(self.bodies[b]):
                add(subst(t, args))
        for a in self.statics:
            add(self.static_ty(a))
        for (t, trait) in self.vtables_wanted:
            add(t)
        for t in self.drops_wanted:
            add(t)
        return need

    def static_ty(self, alloc):
        name = self.mir.allocs[alloc][0]
        p = P(self.mir.statics[name][1])
        t = TypeParser(self.paths).ty(p)
        if not p.end():
            stop('static type', self.mir.statics[name][1])
        return t

    # -- the object file with debug information

    def rust_ty(self, t):
        """the type as Rust source, for the appended functions."""
        k = t[0]
        if k == 'adt':
            if t[1] not in self.paths.spelled:
                stop('type (never spelled in the MIR text, so it cannot be named in the appended functions)', show_ty(t))
            return '::'.join(self.paths.spelled[t[1]]) + ('<' + ', '.join(self.rust_ty(x) for x in t[2]) + '>' if t[2] else '')
        if k == 'ref':
            return '&' + ('mut ' if t[1] else '') + self.rust_ty(t[2])
        if k == 'ptr':
            return '*' + ('mut ' if t[1] else 'const ') + self.rust_ty(t[2])
        if k == 'array':
            return '[%s; %s]' % (self.rust_ty(t[1]), self.rust_ty(t[2]))
        if k == 'slice':
            return '[%s]' % self.rust_ty(t[1])
        if k == 'tuple':
            return '(' + ''.join(self.rust_ty(x) + ', ' for x in t[1]) + ')'
        if k == 'dyn':
            return 'dyn ' + '::'.join(self.paths.spelled[t[1]])
        if k == 'fnptr':
            return 'fn(' + ', '.join(self.rust_ty(x) for x in t[1]) + ') -> ' + self.rust_ty(t[2])
        return show_ty(t)

    def compile_object(self, need):
        copy = os.path.join(self.tmp, os.path.basename(self.src))
        text = self.src_text + '\n// appended by mir2cub: one static per type whose layout is read\n'
        for i, t in enumerate(need):
            # a static whose type mentions the type: rustc describes the types of statics, and (unlike
            # functions with equal bodies) statics are not merged
            text += '#[no_mangle] #[used] #[allow(private_interfaces)]\n'
            text += 'pub static __MIR2CUB_LAYOUT_%d: core::marker::PhantomData<fn(*const %s)> = core::marker::PhantomData;\n' % (i, self.rust_ty(t))
        open(copy, 'w').write(text)
        self.obj = os.path.join(self.tmp, 'crate.o')
        # without optimization: LLVM removes a vtable (and a type's description) it finds unused,
        # and neither layouts nor vtable contents depend on the optimization level
        self.rustc(['-g', '-Ccodegen-units=1', '-Csymbol-mangling-version=v0', '--emit=obj', copy, '-o', self.obj], OBJECT_FLAGS)
        self.obj_types = set(need)
        self.relocs = None

    def read_relocs(self):
        """section → [(offset, symbol)] for the data sections, and section → bytes."""
        out = subprocess.run(['objdump', '-r', self.obj], capture_output=True, text=True, check=True).stdout
        self.relocs, sec = {}, None
        for l in out.split('\n'):
            r = re.match(r'RELOCATION RECORDS FOR \[(.*)\]:', l)
            if r:
                sec = r.group(1)
                continue
            r = re.match(r'([0-9a-f]{16}) (R_X86_64_\w+) +(\S+)$', l)
            if r and sec and sec.startswith('.data.rel.ro'):
                self.relocs.setdefault(sec, []).append((int(r.group(1), 16), r.group(2), r.group(3)))

    def section_bytes(self, sec):
        out = subprocess.run(['objdump', '-s', '-j', sec, self.obj], capture_output=True, text=True, check=True).stdout
        bs = b''
        for l in out.split('\n'):
            r = re.match(r' ([0-9a-f]{4,}) ((?:[0-9a-f]{2,8} ){1,4})', l)
            if r:
                bs += bytes.fromhex(r.group(2).replace(' ', ''))
        return bs

    def read_vtable(self, key):
        t, trait = key
        if self.relocs is None:
            self.read_relocs()
        tp = TypeParser(self.paths)
        found = []
        for sec, rs in self.relocs.items():
            if any(k != 'R_X86_64_64' for (_, k, _) in rs):
                continue
            syms = [s for (_, _, s) in rs if s.startswith('_R')]
            if len(syms) != len(rs) or not syms:
                continue
            dem = subprocess.run(['llvm-cxxfilt'] + syms, capture_output=True, text=True, check=True).stdout.split('\n')[:len(syms)]
            slots = []
            self.paths.origin = 'object'
            for (off, _, sym), d in zip(rs, dem):
                r = re.fullmatch(r'<(.*) as (.*)>::(' + IDENT + ')', d)
                if not r:
                    slots = None
                    break
                p = P(r.group(1))
                st = tp.ty(p)
                q = P(r.group(2))
                segs, a = tp.path(q)
                if not p.end() or not q.end() or a:
                    slots = None
                    break
                slots.append((off, st, self.paths.note(segs), r.group(3)))
            self.paths.origin = 'mir'
            if not slots:
                continue
            if all(s[1] == t and s[2] == trait for s in slots if s[0] >= 24) and all(s[0] >= 24 for s in slots):
                found.append((sec, slots))
        if len(found) != 1:
            stop('vtable (%d sections of the object file hold methods of this impl; a vtable with no methods, or with drop glue, is not read yet)' % len(found),
                 '<%s as %s>' % (show_ty(t), trait))
        sec, slots = found[0]
        bs = self.section_bytes(sec)
        if len(bs) != 24 + 8 * len(slots) or [s[0] for s in slots] != [24 + 8 * i for i in range(len(slots))]:
            stop('vtable (slots are not drop, size, align and then one method each)', sec)
        if int.from_bytes(bs[0:8], 'little') != 0:
            stop('vtable (drop glue)', sec)
        size, align = int.from_bytes(bs[8:16], 'little'), int.from_bytes(bs[16:24], 'little')
        methods = []
        for (_, st, tr, m) in slots:
            f = self.find_method(st, tr, m)
            if f is None:
                stop('vtable method (no body in this crate; a provided method of the trait is not read yet)', m)
            b = self.body_id(f[0])
            methods.append((m, b, f[1]))
            self.add_instance(b, f[1])
        self.vtables[key] = (size, align, methods)


# ------------------------------------------------------------------------------------------
# Writing the .cub file.

class Emit:
    def __init__(self, imp, prefix):
        self.imp, self.px = imp, prefix
        self.out = []
        self.ty_names, self.name_names = {}, {}
        self.adts, self.traits = [], []
        self.layout_order, self.layout_of = [], {}
        self.count = {'definitions': 0}

    def define(self, name, ty, body):
        self.out.append('%s : %s\n%s = %s\n' % (name, ty, name, body))
        self.count['definitions'] += 1

    def w(self, n):
        if n < 0 or n > M64:
            stop('integer (does not fit 64 bits)', str(n))
        return '#%d' % n

    def int(self, v):
        """a mathematical integer as RInt"""
        return '(rint-neg %s)' % self.w(-v) if v < 0 else '(rint-pos %s)' % self.w(v)

    def lst(self, xs):
        s = 'nil'
        for x in reversed(xs):
            s = '(cons %s %s)' % (x, s)
        return s

    def name(self, s):
        if s not in self.name_names:
            n = '%s-nm-%d' % (self.px, len(self.name_names))
            self.name_names[s] = n
            self.out.append('-- "%s"' % s)
            self.define(n, 'RName', self.lst([self.w(b) for b in s.encode()]))
        return self.name_names[s]

    def adt_id(self, name):
        if name not in self.adts:
            self.adts.append(name)
        return self.adts.index(name)

    def trait_id(self, name):
        if name not in self.traits:
            self.traits.append(name)
        return self.traits.index(name)

    def tys(self, ts):
        s = 'rtys-nil'
        for t in reversed(ts):
            s = '(rtys-cons %s %s)' % (self.ty(t), s)
        return s

    def ty(self, t):
        """the name of the definition of this type (each distinct type is defined once)."""
        if t in self.ty_names:
            return self.ty_names[t]
        k = t[0]
        self.imp.met('type', {'ref': 'Ref', 'ptr': 'RawPtr', 'array': 'Array', 'slice': 'Slice', 'tuple': 'Tuple', 'adt': 'Adt',
                              'dyn': 'Dynamic', 'fnptr': 'FnPtr', 'param': 'Param', 'bool': 'Bool', 'int': 'Int/Uint', 'never': 'Never',
                              'const': 'Const (generic argument)', 'cparam': 'ConstParam (generic argument)'}[k])
        if k == 'bool':
            b = 'rty-bool'
        elif k == 'int':
            b = 'rty-int ri-%s' % t[1]
        elif k == 'never':
            b = 'rty-never'
        elif k in ('ref', 'ptr'):
            b = 'rty-%s %s %s' % (k, 'rmut-mut' if t[1] else 'rmut-not', self.ty(t[2]))
        elif k == 'array':
            b = 'rty-array %s %s' % (self.ty(t[1]), self.ty(t[2]))
        elif k == 'slice':
            b = 'rty-slice %s' % self.ty(t[1])
        elif k == 'tuple':
            b = 'rty-tuple %s' % self.tys(t[1])
        elif k == 'adt':
            if t[1] not in self.imp.layouts.adt_kind:
                stop('type (a name that is neither a generic parameter of the impl nor a type the debug information describes; '
                     'generic parameters of a function, as opposed to an impl, are not read yet)', t[1])
            b = 'rty-adt %s %s' % (self.w(self.adt_id(t[1])), self.tys(t[2]))
        elif k == 'dyn':
            b = 'rty-dyn %s' % self.w(self.trait_id(t[1]))
        elif k == 'fnptr':
            b = 'rty-fn-ptr %s %s' % (self.tys(t[1]), self.ty(t[2]))
        elif k == 'param':
            b = 'rty-param %s' % self.w(t[1])
        elif k == 'const':
            b = 'rty-const %s' % self.w(t[1])
        elif k == 'cparam':
            b = 'rty-const-param %s' % self.w(t[1])
        else:
            stop('type', repr(t))
        n = '%s-ty-%d' % (self.px, len(self.ty_names))
        self.ty_names[t] = n
        self.out.append('-- %s' % show_ty(t))
        self.define(n, 'RTy', b)
        return n

    # -- bodies

    def variant_index(self, adt, v):
        vs = self.imp.layouts.variants.get(adt)
        if vs is None or v not in vs:
            stop('variant (not in the debug information of the enum)', '%s::%s' % (adt, v))
        return vs.index(v)

    def place(self, it, pl):
        prs, t = [], it.locals[pl[0]][1]
        for pr in pl[1]:
            if pr[0] == 'deref':
                self.imp.met('projection', 'Deref')
                prs.append('rproj-deref')
                t = t[2]
            elif pr[0] == 'field':
                self.imp.met('projection', 'Field')
                prs.append('(rproj-field %s %s)' % (self.w(pr[1]), self.ty(pr[2])))
                t = pr[2]
            elif pr[0] == 'index':
                self.imp.met('projection', 'Index')
                prs.append('(rproj-index %s)' % self.w(pr[1]))
                t = t[1]
            elif pr[0] == 'cindex':
                self.imp.met('projection', 'ConstantIndex')
                prs.append('(rproj-const-index %s %s %s)' % (self.w(pr[1]), self.w(pr[2]), 'true' if pr[3] else 'false'))
                t = t[1]
            elif pr[0] == 'downcast':
                self.imp.met('projection', 'Downcast')
                if t[0] != 'adt':
                    stop('place (downcast of a non-ADT)', show_ty(t))
                prs.append('(rproj-downcast %s)' % self.w(self.variant_index(t[1], pr[1])))
        return '(rplace %s %s)' % (self.w(pl[0]), self.lst(prs))

    def adt_path(self, segs, args, argseg, what):
        """a path naming a struct, or an enum's variant → (adt name, variant index, is_enum)"""
        L = self.imp.layouts
        if len(segs) >= 2 and L.adt_kind.get(segs[-2]) == 'enum' and segs[-1] in L.variants[segs[-2]] \
                and (argseg is None or argseg == len(segs) - 2):
            return self.imp.paths.note(segs[:-1]), L.variants[segs[-2]].index(segs[-1])
        if L.adt_kind.get(segs[-1]) == 'struct' and (argseg is None or argseg == len(segs) - 1):
            return self.imp.paths.note(segs), 0
        stop(what + ' (a path that is neither a struct nor an enum variant the debug information describes)', '::'.join(segs))

    def const(self, c):
        k = c[0]
        if k == 'int':
            self.imp.met('constant', 'integer')
            return '(rc-int %s ri-%s)' % (self.int(c[1]), c[2])
        if k == 'bool':
            self.imp.met('constant', 'bool')
            return '(rc-bool %s)' % ('true' if c[1] else 'false')
        if k == 'cparam':
            self.imp.met('constant', 'generic constant parameter')
            return '(rc-param %s)' % self.w(c[1])
        if k == 'fnitem':
            self.imp.met('constant', 'function item')
            return '(rc-fn %s %s)' % (self.w(c[1][1]), self.tys(c[1][2]))
        if k == 'alloc':
            self.imp.met('constant', 'address of a static')
            return '(rc-static %s %s)' % (self.w(self.imp.statics.index(c[1])), self.ty(c[2]))
        if k == 'path':
            _, segs, args, argseg, fields = c
            adt, v = self.adt_path(segs, args, argseg, 'constant')
            t = ('adt', adt, args)
            if fields is None and self.imp.layouts.adt_kind[adt] == 'struct':
                if not is_closed(t):
                    stop('constant (zero-sized value of an open type)', show_ty(t))
                lay = self.imp.layouts.layout(t)
                if lay is None or lay[0] != 0:
                    stop('constant (a struct named without fields that is not zero-sized)', show_ty(t))
                self.imp.met('constant', 'zero-sized value')
                return '(rc-zst %s)' % self.ty(t)
            self.imp.met('constant', 'ADT value')
            fs = 'rcs-nil'
            for f in reversed(fields or []):
                fs = '(rcs-cons %s %s)' % (self.const(f), fs)
            return '(rc-adt %s %s %s)' % (self.ty(t), self.w(v), fs)
        stop('constant', repr(c))

    def operand(self, it, o):
        if o[0] == 'const':
            self.imp.met('operand', 'Constant')
            return '(rop-const %s)' % self.const(o[1])
        self.imp.met('operand', 'Copy' if o[0] == 'copy' else 'Move')
        return '(rop-%s %s)' % (o[0], self.place(it, o[1]))

    def rvalue(self, it, rv):
        k = rv[0]
        if k == 'use':
            self.imp.met('rvalue', 'Use' if rv[2] else 'Use (no_retag)')
            return '(rrv-use %s %s)' % (self.operand(it, rv[1]), 'true' if rv[2] else 'false')
        if k == 'repeat':
            self.imp.met('rvalue', 'Repeat')
            return '(rrv-repeat %s %s)' % (self.operand(it, rv[1]), self.ty(rv[2]))
        if k == 'ref':
            self.imp.met('rvalue', 'Ref (%s)' % rv[1])
            return '(rrv-ref rborrow-%s %s)' % (rv[1], self.place(it, rv[2]))
        if k == 'rawptr':
            self.imp.met('rvalue', 'RawPtr (&raw %s)' % ('mut' if rv[1] else 'const'))
            return '(rrv-raw-ptr %s %s)' % ('rmut-mut' if rv[1] else 'rmut-not', self.place(it, rv[2]))
        if k == 'cast':
            self.imp.met('rvalue', 'Cast')
            self.imp.met('cast', rv[1])
            return '(rrv-cast rcast-%s %s %s)' % (rv[1], self.operand(it, rv[2]), self.ty(rv[3]))
        if k == 'binop':
            self.imp.met('rvalue', 'BinaryOp')
            self.imp.met('binop', rv[1])
            return '(rrv-binop rbin-%s %s %s)' % (rv[1], self.operand(it, rv[2]), self.operand(it, rv[3]))
        if k == 'unop':
            self.imp.met('rvalue', 'UnaryOp')
            self.imp.met('unop', rv[1])
            return '(rrv-unop run-%s %s)' % (rv[1], self.operand(it, rv[2]))
        if k == 'discriminant':
            self.imp.met('rvalue', 'Discriminant')
            return '(rrv-discriminant %s)' % self.place(it, rv[1])
        if k == 'aggregate':
            self.imp.met('rvalue', 'Aggregate')
            a = rv[1]
            if a[0] == 'array':
                self.imp.met('aggregate', 'Array')
                ak = 'rag-array'
            elif a[0] == 'tuple':
                self.imp.met('aggregate', 'Tuple')
                ak = 'rag-tuple'
            elif a[0] == 'rawptr':
                self.imp.met('aggregate', 'RawPtr')
                if a[1][0] != 'ptr':
                    stop('aggregate (pointer)', show_ty(a[1]))
                ak = '(rag-raw-ptr %s %s)' % (self.ty(a[1][2]), 'rmut-mut' if a[1][1] else 'rmut-not')
            else:
                self.imp.met('aggregate', 'Adt')
                adt, v = self.adt_path(a[1], a[2], a[3], 'aggregate')
                ak = '(rag-adt %s %s %s)' % (self.w(self.adt_id(adt)), self.w(v), self.tys(a[2]))
            return '(rrv-aggregate %s %s)' % (ak, self.lst([self.operand(it, o) for o in rv[2]]))
        stop('rvalue', repr(rv))

    def stmt(self, it, s):
        if s[0] == 'live':
            self.imp.met('statement', 'StorageLive')
            return '(rs-storage-live %s)' % self.w(s[1])
        if s[0] == 'dead':
            self.imp.met('statement', 'StorageDead')
            return '(rs-storage-dead %s)' % self.w(s[1])
        self.imp.met('statement', 'Assign')
        return '(rs-assign %s %s)' % (self.place(it, s[1]), self.rvalue(it, s[2]))

    def unwind(self, u):
        return 'ru-' + u[0]

    def callee(self, it, c):
        if c[0] == 'ptr':
            return '(rcallee-ptr %s)' % self.operand(it, c[1])
        if c[0] == 'dyn':
            ms = self.trait_methods(c[1])
            if c[2] not in ms:
                stop('call (a method that is not in the trait\'s vtable)', '%s::%s' % (c[1], c[2]))
            return '(rcallee-dyn %s %s)' % (self.w(self.trait_id(c[1])), self.w(ms.index(c[2])))
        return '(rcallee-fn %s %s)' % (self.w(c[1]), self.tys(c[2]))

    def trait_methods(self, trait):
        lists = [[m for (m, _, _) in v[2]] for (t, tr), v in self.imp.vtables.items() if tr == trait]
        if not lists:
            stop('trait (called through dyn, but no vtable of it is made in a reachable body)', trait)
        if any(l != lists[0] for l in lists):
            stop('trait (two vtables with different methods)', trait)
        return lists[0]

    def term(self, it, t):
        k = t[0]
        if k == 'goto':
            self.imp.met('terminator', 'Goto')
            return '(rt-goto %s)' % self.w(t[1])
        if k == 'return':
            self.imp.met('terminator', 'Return')
            return 'rt-return'
        if k == 'unreachable':
            self.imp.met('terminator', 'Unreachable')
            return 'rt-unreachable'
        if k == 'switch':
            self.imp.met('terminator', 'SwitchInt')
            # rustc prints an arm's value as the unsigned bits at the scrutinee's width; here it
            # is the integer those bits are at the scrutinee's type
            if t[1][0] == 'const':
                stop('switchInt (on a constant)', it.name)
            st = self.imp.place_ty(it, t[1][1])
            if st[0] == 'bool':
                bits, signed = 1, False
            elif st[0] == 'int':
                bits, signed = INT_BITS[st[1]], st[1].startswith('i')
            else:
                stop('switchInt (scrutinee of type %s)' % show_ty(st), it.name)
            vals = []
            for (v, b) in t[2]:
                if v < 0 or v >> bits:
                    stop('switchInt (arm value outside the scrutinee\'s width)', '%d in %s' % (v, it.name))
                vals.append((v - (1 << bits) if signed and v >> (bits - 1) else v, b))
            arms = self.lst(['(rarm %s %s)' % (self.int(v), self.w(b)) for (v, b) in vals])
            return '(rt-switch-int %s %s %s)' % (self.operand(it, t[1]), arms, self.w(t[3]))
        if k == 'call':
            self.imp.met('terminator', {'dyn': 'Call (dyn)', 'ptr': 'Call (function pointer)', 'fn': 'Call'}[t[1][0]])
            return '(rt-call %s %s %s %s %s)' % (self.callee(it, t[1]), self.lst([self.operand(it, o) for o in t[2]]),
                                                 self.place(it, t[3]), self.w(t[4]), self.unwind(t[5]))
        if k == 'assert':
            self.imp.met('terminator', 'Assert (%s)' % t[3])
            return '(rt-assert %s %s (rassert-%s %s) %s %s)' % (
                self.operand(it, t[1]), 'true' if t[2] else 'false', t[3], ' '.join(self.operand(it, o) for o in t[4]),
                self.w(t[5]), self.unwind(t[6]))
        if k == 'drop':
            self.imp.met('terminator', 'Drop')
            return '(rt-drop %s %s %s)' % (self.place(it, t[1]), self.w(t[2]), self.unwind(t[3]))
        stop('terminator', repr(t))

    def body(self, b, it):
        px = '%s-b%d' % (self.px, b)
        self.out.append('\n-- body %d: %s' % (b, it.name))
        locs = []
        for (m, t) in it.locals:
            locs.append('(rlocal-arg %s)' % self.ty(t) if m == 'arg' else '(rlocal %s %s)' % (self.ty(t), 'rmut-mut' if m == 'mut' else 'rmut-not'))
        self.define(px + '-locals', 'List RLocal', self.lst(locs))
        self.count['locals'] = self.count.get('locals', 0) + len(locs)
        names = []
        for i, (stmts, term) in enumerate(it.blocks):
            ss = [self.stmt(it, s) for s in stmts]
            tm = self.term(it, term)
            self.count['statements'] = self.count.get('statements', 0) + len(ss)
            # a block's statements in runs of 16, so no one definition is large
            chunks = [ss[j:j + 16] for j in range(0, len(ss), 16)]
            tail = 'nil'
            for ci in range(len(chunks) - 1, 0, -1):
                n = '%s-bb%d-s%d' % (px, i, ci)
                body = tail
                for x in reversed(chunks[ci]):
                    body = '(cons %s %s)' % (x, body)
                self.define(n, 'List RStmt', body)
                tail = n
            first = tail
            for x in reversed(chunks[0] if chunks else []):
                first = '(cons %s %s)' % (x, first)
            n = '%s-bb%d' % (px, i)
            self.define(n, 'RBlock', 'rblock %s %s' % (first, tm))
            names.append(n)
        self.count['blocks'] = self.count.get('blocks', 0) + len(names)
        gens = self.lst(['(rgen-%s %s)' % (k, self.name(n)) for (k, n) in it.generics])
        self.define(px, 'RBody', 'rbody %s %s %s %s-locals %s' % (self.name(it.name), gens, self.w(it.argc), px, self.lst(names)))
        return px

    # -- layouts, vtables, statics

    def want_layout(self, t):
        if t[0] in ('adt', 'tuple') and not (t[0] == 'tuple' and not t[1]) and t not in self.layout_of:
            lay = self.imp.layouts.layout(t)
            if lay is None:
                stop('layout (the debug information does not describe this type)', show_ty(t))
            self.layout_of[t] = lay
            self.layout_order.append(t)
            for (_, _, _, fs) in lay[3]:
                for (_, _, ft) in fs:
                    self.want_layout_deep(ft)
        return

    def want_layout_deep(self, t):
        self.want_layout(t)
        for s in subterms(t):
            self.want_layout_deep(s)

    def layout(self, i, t):
        size, align, tag, vs = self.layout_of[t]
        if tag is None:
            tg = 'rtag-none'
        else:
            tg = '(rtag-%s %s %s)' % (tag[0], self.w(tag[1]), self.w(tag[2]))
        vnames = []
        for vi, (name, discr, tv, fs) in enumerate(vs):
            fl = self.lst(['(rfield %s %s %s)' % (self.name(fn), self.w(off), self.ty(ft)) for (fn, off, ft) in fs])
            vn = '%s-layout-%d-v%d' % (self.px, i, vi)
            self.define(vn, 'RVariantLayout', 'rvariant %s %s %s %s' % (
                self.name(name), self.int(discr), 'rtv-none' if tv is None else '(rtv-some %s)' % self.w(tv), fl))
            vnames.append(vn)
        n = '%s-layout-%d' % (self.px, i)
        self.out.append('-- layout of %s: size %d, align %d' % (show_ty(t), size, align))
        self.define(n, 'RLayoutEntry', 'rlayout-of %s (rlayout %s %s %s %s)' % (self.ty(t), self.w(size), self.w(align), tg, self.lst(vnames)))
        return n

    def static(self, i, alloc):
        name, size, align, bs = self.imp.mir.allocs[alloc]
        mut, _ = self.imp.mir.statics[name]
        runs, j, k = [], 0, 0
        while j < len(bs):
            e = j
            while e < len(bs) and (bs[e] is None) == (bs[j] is None) and (bs[j] is None or e - j < 32):
                e += 1
            if bs[j] is None:
                runs.append('(rbytes-uninit %s)' % self.w(e - j))
            else:
                n = '%s-static-%d-bytes-%d' % (self.px, i, k)
                k += 1
                self.define(n, 'List Word', self.lst([self.w(x) for x in bs[j:e]]))
                runs.append('(rbytes-init %s)' % n)
            j = e
        n = '%s-static-%d' % (self.px, i)
        self.define(n, 'RStatic', 'rstatic %s %s %s %s %s %s' % (
            self.name(name), self.ty(self.imp.static_ty(alloc)), 'rmut-mut' if mut else 'rmut-not', self.w(size), self.w(align), self.lst(runs)))
        return n

    def run(self):
        imp = self.imp
        head = ['-- %s.cub — generated by tools/mir2cub.py from %s; do not edit.' % (self.px, imp.src),
                '-- %s' % subprocess.run([RUSTC, '--version'], capture_output=True, text=True).stdout.strip(),
                '-- MIR: rustc %s --emit=mir (before monomorphization; generic arguments are data)' % ' '.join(RUSTC_FLAGS),
                '-- load after the library and lib/lang/rust/ast.cub; the crate is %s-crate : RCrate.' % self.px, '']
        bodies = [self.body(b, it) for b, it in enumerate(imp.bodies)]
        for (b, args) in imp.instances:
            for t in imp.body_types(imp.bodies[b]):
                self.want_layout_deep(subst(t, args))
        for a in imp.statics:
            self.want_layout_deep(imp.static_ty(a))
        for (t, trait) in imp.vtables:
            self.want_layout_deep(t)
        for t in imp.drops_wanted:
            self.want_layout_deep(t)
        self.out.append('\n-- layouts')
        i, lnames = 0, []
        while i < len(self.layout_order):
            lnames.append(self.layout(i, self.layout_order[i]))
            i += 1
        self.out.append('\n-- vtables')
        vnames = []
        for i, ((t, trait), (size, align, methods)) in enumerate(imp.vtables.items()):
            n = '%s-vtable-%d' % (self.px, i)
            self.out.append('-- <%s as %s>' % (show_ty(t), trait))
            self.define(n, 'RVtable', 'rvtable %s %s %s %s rdrop-none %s' % (
                self.ty(t), self.w(self.trait_id(trait)), self.w(size), self.w(align),
                self.lst(['(rcallee-fn %s %s)' % (self.w(b), self.tys(a)) for (_, b, a) in methods])))
            vnames.append(n)
        self.out.append('\n-- statics')
        snames = [self.static(i, a) for i, a in enumerate(imp.statics)]
        self.out.append('\n-- Drop impls of the types a reachable Drop terminator drops (or contains)')
        self.define(self.px + '-drops', 'List RDropImpl', self.lst(
            ['(rdrop-impl %s %s %s)' % (self.ty(t), self.w(b), self.tys(a)) for (t, b, a) in imp.drop_impls]))
        self.out.append('\n-- instances: every (body, generic arguments) reachable from the entry')
        inames = []
        for i, (b, args) in enumerate(imp.instances):
            inames.append('(rinstance %s %s)' % (self.w(b), self.tys(args)))
        self.define(self.px + '-instances', 'List RInstance', self.lst(inames))
        self.out.append('\n-- ADTs and traits by id')
        # ids were handed out while writing; the lists are complete only now
        done = False
        while not done:
            na, nt = len(self.adts), len(self.traits)
            adts = self.lst(['(radt %s radt-%s)' % (self.name(a), imp.layouts.adt_kind[a]) for a in self.adts])
            traits = self.lst(['(rtrait %s %s)' % (self.name(t), self.lst([self.name(m) for m in self.trait_methods(t)])) for t in self.traits])
            done = na == len(self.adts) and nt == len(self.traits)
        self.define(self.px + '-adts', 'List RAdt', adts)
        self.define(self.px + '-traits', 'List RTrait', traits)
        self.define(self.px + '-crate', 'RCrate', 'rcrate %s %s %s %s-adts %s-traits %s %s %s %s-drops %s-instances' % (
            self.name(os.path.basename(imp.src)), self.w(0), self.lst(bodies), self.px, self.px,
            self.lst(lnames), self.lst(vnames), self.lst(snames), self.px, self.px))
        return '\n'.join(head + self.out) + '\n'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('source')
    ap.add_argument('-o', '--output', required=True)
    ap.add_argument('--entry', default='on_message')
    a = ap.parse_args()
    imp = Import(a.source, a.entry)
    try:
        imp.run()
        em = Emit(imp, os.path.splitext(os.path.basename(a.output))[0])
        text = em.run()
    except Unhandled as e:
        raise SystemExit(str(e))
    open(a.output, 'w').write(text)
    shutil.rmtree(imp.tmp)
    rep = sys.stderr
    rep.write('mir2cub: %s → %s (%d bytes, %d lines)\n' % (imp.src, a.output, len(text), text.count('\n')))
    rep.write('  instances all among the %d function instances rustc -Zprint-mono-items lists\n' % imp.mono_items)
    rep.write('  bodies %d, instances %d, blocks %d, statements %d, locals %d, definitions %d\n' % (
        len(imp.bodies), len(imp.instances), em.count.get('blocks', 0), em.count.get('statements', 0),
        em.count.get('locals', 0), em.count['definitions']))
    rep.write('  types %d, ADTs %d (%s), traits %d, layouts %d, vtables %d, statics %d, drop impls %d\n' % (
        len(em.ty_names), len(em.adts), ', '.join(em.adts), len(em.traits), len(em.layout_order), len(imp.vtables), len(imp.statics), len(imp.drop_impls)))
    for cls in ('statement', 'rvalue', 'cast', 'binop', 'unop', 'aggregate', 'terminator', 'operand', 'constant', 'projection', 'type'):
        ks = imp.kinds[cls]
        rep.write('  %s kinds met: %s\n' % (cls, ', '.join('%s %d' % (k, ks[k]) for k in sorted(ks))))


if __name__ == '__main__':
    main()

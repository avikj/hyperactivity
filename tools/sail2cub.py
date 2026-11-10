#!/usr/bin/env python3
"""sail2cub: the Sail x86 model (rems-project/sail-x86-from-acl2, model/) read into .cub.

  python3 tools/sail2cub.py MODEL_DIR --parse          per-file counts of what was parsed
  python3 tools/sail2cub.py MODEL_DIR --out lib/isa/x86   write one .cub per .sail and ORDER

The Sail subset is the one the model uses (it is generated from ACL2's x86isa): val, function
(clauses, scattered), type synonyms, bitfield, union, enum, struct, register, overload, let, if,
match, blocks with assignment, foreach, try/catch, throw, return, assert, tuples, lists,
bitvector literals, slices and updates. A construct outside it stops the importer with the
file, line and token: it is then added here, never skipped.
"""
import os
import re
import sys

# ----------------------------------------------------------------------------- lexer
TOKEN = re.compile(r'''
    (?P<ws>\s+)
  | (?P<lc>//[^\n]*)
  | (?P<bc>/\*.*?\*/)
  | (?P<dir>\$[a-z_]+[^\n]*)
  | (?P<str>"(?:[^"\\]|\\.)*")
  | (?P<hex>0x[0-9A-Fa-f_]+)
  | (?P<bin>0b[01_]+)
  | (?P<num>[0-9]+)
  | (?P<tv>'[A-Za-z_][A-Za-z0-9_?]*)
  | (?P<id>[A-Za-z_][A-Za-z0-9_?']*)
  | (?P<op>\[\||\|\]|\{\||\|\}|=>|->|==|!=|<=|>=|<<|>>|::|\.\.|[-+*/%^@&|<>=(){}\[\],;:.~!])
''', re.X | re.S)

KEYWORDS = {'val', 'function', 'clause', 'scattered', 'end', 'type', 'bitfield', 'union', 'enum',
            'struct', 'register', 'overload', 'operator', 'infix', 'infixl', 'infixr', 'default',
            'let', 'in', 'if', 'then', 'else', 'match', 'foreach', 'from', 'to', 'by', 'try',
            'catch', 'throw', 'return', 'assert', 'forall', 'with', 'true', 'false', 'undefined',
            'bitzero', 'bitone', 'var', 'as', 'pure', 'monadic', 'downto', 'while', 'do'}


class Tok:
    __slots__ = ('k', 'v', 'file', 'line')

    def __init__(self, k, v, file, line):
        self.k, self.v, self.file, self.line = k, v, file, line

    def __repr__(self):
        return '%s:%d %s %r' % (self.file, self.line, self.k, self.v)


class SailError(Exception):
    pass


def lex(text, fname):
    out, pos, line = [], 0, 1
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m:
            raise SailError('%s:%d: cannot lex %r' % (fname, line, text[pos:pos + 20]))
        k, v = m.lastgroup, m.group()
        if k not in ('ws', 'lc', 'bc'):
            if k == 'id' and v in KEYWORDS:
                k = 'kw'
            out.append(Tok(k, v, fname, line))
        line += v.count('\n')
        pos = m.end()
    out.append(Tok('eof', '', fname, line))
    return out


# ----------------------------------------------------------------------------- syntax
class N:
    """A syntax node: kind k, fields by name; ty is filled by the type pass."""

    def __init__(self, k, tok=None, **kw):
        self.k = k
        self.file = tok.file if tok else None
        self.line = tok.line if tok else None
        self.ty = None
        self.__dict__.update(kw)

    def __repr__(self):
        d = {a: b for a, b in self.__dict__.items() if a not in ('k', 'file', 'line', 'ty')}
        return '%s%r' % (self.k, d)


BINOPS = {  # operator: (precedence, associativity)
    '|': (2, 'r'), '&': (3, 'r'),
    '==': (4, 'n'), '!=': (4, 'n'), '<': (4, 'n'), '<=': (4, 'n'), '>': (4, 'n'), '>=': (4, 'n'),
    '@': (5, 'r'), '::': (5, 'r'),
    '+': (6, 'l'), '-': (6, 'l'),
    '*': (7, 'l'), '/': (7, 'l'), '%': (7, 'l'), '<<': (7, 'n'), '>>': (7, 'n'),
    '^': (8, 'r'),
}


class Parser:
    def __init__(self, toks):
        self.t, self.i = toks, 0

    # -- tokens
    def peek(self, o=0):
        return self.t[self.i + o]

    def at(self, v, o=0):
        t = self.t[self.i + o]
        return t.v == v and t.k in ('op', 'kw')

    def next(self):
        t = self.t[self.i]
        self.i += 1
        return t

    def fail(self, what):
        t = self.peek()
        raise SailError('%s:%d: %s, found %s %r' % (t.file, t.line, what, t.k, t.v))

    def eat(self, v):
        if not self.at(v):
            self.fail('expected %r' % v)
        return self.next()

    def opt(self, v):
        if self.at(v):
            self.next()
            return True
        return False

    def ident(self):
        t = self.peek()
        if t.k != 'id':
            self.fail('expected a name')
        return self.next().v

    # -- types.  A type is a node: tid(name), tapp(name,args), ttuple(items), tfn(arg,ret),
    #    and numeric expressions tnum(n), tvar(name), tbin(op,a,b), tset(items), texist(body)
    def typ(self):
        a = self.typ_arith()
        if self.at('->'):
            t = self.next()
            return N('tfn', t, arg=a, ret=self.typ())
        return a

    def typ_arith(self, minp=6):
        a = self.typ_atom()
        while True:
            t = self.peek()
            if t.k == 'op' and t.v in ('+', '-', '*', '^') and BINOPS[t.v][0] >= minp:
                p, assoc = BINOPS[t.v]
                self.next()
                b = self.typ_arith(p if assoc == 'r' else p + 1)
                a = N('tbin', t, op=t.v, a=a, b=b)
            else:
                return a

    def typ_atom(self):
        t = self.peek()
        if t.k == 'num':
            self.next()
            return N('tnum', t, n=int(t.v))
        if t.k == 'op' and t.v == '-' and self.peek(1).k == 'num':
            self.next()
            return N('tnum', t, n=-int(self.next().v))
        if t.k == 'tv':
            self.next()
            return N('tvar', t, name=t.v)
        if self.at('('):
            self.next()
            if self.at(')'):
                self.next()
                return N('tid', t, name='unit')
            items = [self.typ()]
            while self.opt(','):
                items.append(self.typ())
            self.eat(')')
            return items[0] if len(items) == 1 else N('ttuple', t, items=items)
        if self.at('{|'):
            self.next()
            items = []
            while not self.at('|}'):
                neg = self.opt('-')
                n = int(self.next().v)
                items.append(-n if neg else n)
                self.opt(',')
            self.eat('|}')
            return N('tset', t, items=items)
        if self.at('{'):  # existential: {'j, constraint. typ} or {'j 'k. typ}
            self.next()
            depth = 0
            while not (depth == 0 and self.at('.')):
                if self.peek().v in ('(', '{', '['):
                    depth += 1
                if self.peek().v in (')', '}', ']'):
                    depth -= 1
                self.next()
            self.eat('.')
            body = self.typ()
            self.eat('}')
            return N('texist', t, body=body)
        if t.k == 'id':
            self.next()
            if self.at('('):
                self.next()
                args = []
                while not self.at(')'):
                    args.append(self.typ())
                    self.opt(',')
                self.eat(')')
                return N('tapp', t, name=t.v, args=args)
            return N('tid', t, name=t.v)
        self.fail('expected a type')

    def typschm(self):
        """forall 'n ('m : Int), constraint. typ   ->   (quantified variables, typ)"""
        tvs = []
        if self.opt('forall'):
            depth = 0
            while not (depth == 0 and (self.at(',') or self.at('.'))):
                t = self.next()
                if t.v == '(':
                    depth += 1
                elif t.v == ')':
                    depth -= 1
                elif t.k == 'tv':
                    kind = 'Int'
                    if depth == 1 and self.at(':'):
                        self.next()
                        kind = self.next().v
                    tvs.append((t.v, kind))
            if self.opt(','):  # the constraint: skipped (Sail has proved it)
                depth = 0
                while not (depth == 0 and self.at('.')):
                    if self.peek().v in ('(', '{', '{|'):
                        depth += 1
                    if self.peek().v in (')', '}', '|}'):
                        depth -= 1
                    self.next()
            self.eat('.')
        return tvs, self.typ()

    # -- patterns: pwild, pid(name), plit(lit), ptuple(items), papp(name,args), pcons(h,t),
    #    pnil, ptyped(p,typ)
    def pat(self):
        p = self.pat1()
        if self.at('::'):
            t = self.next()
            p = N('pcons', t, h=p, t=self.pat())
        while self.at(':'):  # the model writes `x : int : int` in places
            t = self.next()
            p = N('ptyped', t, p=p, typ=self.typ())
        return p

    def pat1(self):
        t = self.peek()
        if t.k in ('num', 'hex', 'bin', 'str') or (t.k == 'kw' and t.v in ('true', 'false', 'bitzero', 'bitone')):
            return N('plit', t, lit=self.literal())
        if self.at('-') and self.peek(1).k == 'num':
            self.next()
            lit = self.literal()
            lit.n = -lit.n
            return N('plit', t, lit=lit)
        if self.at('[|'):
            self.next()
            self.eat('|]')
            return N('pnil', t)
        if self.at('('):
            self.next()
            if self.at(')'):
                self.next()
                return N('plit', t, lit=N('unit', t))
            items = [self.pat()]
            while self.opt(','):
                items.append(self.pat())
            self.eat(')')
            return items[0] if len(items) == 1 else N('ptuple', t, items=items)
        if t.k == 'id':
            self.next()
            if t.v == '_':
                return N('pwild', t)
            if self.at('('):
                self.next()
                args = []
                while not self.at(')'):
                    args.append(self.pat())
                    self.opt(',')
                self.eat(')')
                return N('papp', t, name=t.v, args=args)
            return N('pid', t, name=t.v)
        self.fail('expected a pattern')

    # -- expressions
    def literal(self):
        t = self.next()
        if t.k == 'num':
            return N('int', t, n=int(t.v))
        if t.k == 'hex':
            d = t.v[2:].replace('_', '')
            return N('bitsl', t, n=int(d, 16), w=4 * len(d))
        if t.k == 'bin':
            d = t.v[2:].replace('_', '')
            return N('bitsl', t, n=int(d, 2), w=len(d))
        if t.k == 'str':
            return N('str', t, s=bytes(t.v[1:-1], 'utf-8').decode('unicode_escape'))
        if t.v in ('true', 'false'):
            return N('bool', t, b=t.v == 'true')
        if t.v in ('bitzero', 'bitone'):
            return N('bit', t, b=t.v == 'bitone')
        self.i -= 1
        self.fail('expected a literal')

    def exp(self):
        """An expression, with a trailing `: typ` annotation and assignment `lhs = e`."""
        t = self.peek()
        if self.at('let') or self.at('var'):
            self.next()
            p = self.pat()
            self.eat('=')
            e = self.exp()
            if self.opt('in'):
                return N('let', t, pat=p, e=e, body=self.exp())
            return N('letstmt', t, pat=p, e=e)  # scope: the rest of the block
        if self.at('if'):
            self.next()
            c = self.exp()
            self.eat('then')
            a = self.exp()
            b = self.exp() if self.opt('else') else None
            return N('if', t, c=c, a=a, b=b)
        if self.at('return'):
            self.next()
            return N('return', t, e=self.exp())
        if self.at('foreach'):
            self.next()
            self.eat('(')
            v = self.ident()
            self.eat('from')
            lo = self.exp()
            down = False
            if self.at('downto'):
                self.next()
                down = True
            else:
                self.eat('to')
            hi = self.exp()
            if self.at('by'):
                self.fail('foreach with a step')
            self.eat(')')
            return N('foreach', t, v=v, lo=lo, hi=hi, down=down, body=self.exp())
        if self.at('try'):
            self.next()
            e = self.exp()
            self.eat('catch')
            return N('try', t, e=e, cases=self.cases())
        e = self.binary(0)
        if self.at('='):
            tk = self.next()
            if e.k == 'cast':  # x : typ = e   declares a mutable variable
                return N('assign', tk, lhs=e.e, e=self.exp(), typ=e.typ)
            return N('assign', tk, lhs=e, e=self.exp(), typ=None)
        return e

    def cases(self):
        self.eat('{')
        cs = []
        while not self.at('}'):
            p = self.pat()
            if self.at('if'):
                self.fail('a guarded match case')
            self.eat('=>')
            cs.append((p, self.exp()))
            if not self.opt(','):
                break
        self.eat('}')
        return cs

    def binary(self, minp):
        a = self.unary()
        while True:
            t = self.peek()
            if t.k == 'op' and t.v in BINOPS and BINOPS[t.v][0] >= minp:
                p, assoc = BINOPS[t.v]
                self.next()
                b = self.binary(p if assoc == 'r' else p + 1)
                a = N('binop', t, op=t.v, a=a, b=b)
            else:
                return a

    def unary(self):
        t = self.peek()
        if self.at('-'):
            self.next()
            if self.peek().k == 'num':
                e = self.postfix()
                if e.k == 'int':
                    e.n = -e.n
                    return e
                return N('call', t, f='negate', args=[e])
            return N('call', t, f='negate', args=[self.unary()])
        return self.postfix()

    def postfix(self):
        e = self.atom()
        while True:
            t = self.peek()
            if self.at('.'):
                self.next()
                e = N('field', t, e=e, f=self.ident())
            elif self.at('['):
                self.next()
                i = self.exp()
                if self.opt('..'):
                    lo = self.exp()
                    self.eat(']')
                    e = N('slice', t, e=e, hi=i, lo=lo)
                else:
                    self.eat(']')
                    e = N('index', t, e=e, i=i)
            elif self.at(':'):  # an annotation binds as an atom: [..] : vector(..)[i]
                self.next()
                e = N('cast', t, e=e, typ=self.typ())
            else:
                return e

    def atom(self):
        t = self.peek()
        if t.k in ('num', 'hex', 'bin', 'str') or (t.k == 'kw' and t.v in ('true', 'false', 'bitzero', 'bitone')):
            return self.literal()
        if self.at('undefined'):
            self.next()
            return N('undefined', t)
        if self.at('('):
            self.next()
            if self.at(')'):
                self.next()
                return N('unit', t)
            items = [self.exp()]
            while self.opt(','):
                items.append(self.exp())
            self.eat(')')
            return items[0] if len(items) == 1 else N('tuple', t, items=items)
        if self.at('{'):
            self.next()
            ss = []
            while not self.at('}'):
                ss.append(self.exp())
                if not self.opt(';'):
                    break
            self.eat('}')
            return N('block', t, ss=ss)
        if self.at('[|'):
            self.next()
            items = []
            while not self.at('|]'):
                items.append(self.exp())
                self.opt(',')
            self.eat('|]')
            return N('list', t, items=items)
        if self.at('['):
            self.next()
            first = self.exp()
            if self.opt('with'):  # [e with f = v, g, hi .. lo = v]
                ups = []
                while True:
                    a = self.binary(0)
                    if self.opt('..'):
                        lo = self.binary(0)
                        self.eat('=')
                        ups.append(('slice', a, lo, self.exp()))
                    elif self.opt('='):
                        ups.append(('at', a, None, self.exp()))
                    else:  # [x with f]: the field f from the variable f
                        ups.append(('at', a, None, a))
                    if not self.opt(','):
                        break
                self.eat(']')
                return N('update', t, e=first, ups=ups)
            items = [first]
            while self.opt(','):
                items.append(self.exp())
            self.eat(']')
            return N('vec', t, items=items)
        if self.at('struct'):  # struct { f = e, g = e }
            self.next()
            self.eat('{')
            fs = []
            while not self.at('}'):
                f = self.ident()
                self.eat('=')
                fs.append((f, self.exp()))
                self.opt(',')
            self.eat('}')
            return N('structlit', t, fs=fs)
        if self.at('match'):
            self.next()
            e = self.exp()
            return N('match', t, e=e, cases=self.cases())
        if self.at('throw'):
            self.next()
            self.eat('(')
            e = self.exp()
            self.eat(')')
            return N('throw', t, e=e)
        if self.at('assert'):
            self.next()
            self.eat('(')
            c = self.exp()
            msg = self.exp() if self.opt(',') else None
            self.eat(')')
            return N('assert', t, c=c, msg=msg)
        if self.at('return'):
            self.next()
            return N('return', t, e=self.exp())
        if self.at('if') or self.at('let'):
            return self.exp()
        if t.k == 'tv':  # a type variable used as a value: 'n
            self.next()
            return N('tvval', t, name=t.v)
        if t.k == 'id':
            self.next()
            if self.at('('):
                self.next()
                args = []
                while not self.at(')'):
                    args.append(self.exp())
                    if not self.opt(','):
                        break
                self.eat(')')
                return N('call', t, f=t.v, args=args)
            return N('id', t, name=t.v)
        self.fail('expected an expression')

    # -- top level
    def top(self):
        """One definition, or None at the end of the file."""
        t = self.peek()
        if t.k == 'eof':
            return None
        if t.k == 'dir':
            self.next()
            w = t.v.split(None, 1)
            return N('dir', t, d=w[0], arg=w[1].strip() if len(w) > 1 else '')
        if self.at('default'):
            self.next()
            self.next()
            self.next()
            return N('default', t)
        if self.at('val'):
            self.next()
            name = self.ident()
            ext = None
            if self.opt('='):
                if self.at('pure') or self.at('monadic'):
                    self.next()
                ext = self.next().v[1:-1]
            self.eat(':')
            tvs, ty = self.typschm()
            return N('val', t, name=name, tvs=tvs, typ=ty, ext=ext)
        if self.at('scattered'):
            self.next()
            self.eat('function')
            return N('scattered', t, name=self.ident())
        if self.at('end'):
            self.next()
            return N('endscattered', t, name=self.ident())
        if self.at('function'):
            self.next()
            clause = self.opt('clause')
            name = self.ident()
            if self.at('('):
                p = self.pat()
            else:  # function f x = e
                p = self.pat1()
            self.eat('=')
            return N('function', t, name=name, pat=p, body=self.exp(), clause=clause)
        if self.at('type'):
            self.next()
            name = self.ident()
            params = []
            if self.at('('):
                self.next()
                while not self.at(')'):
                    tk = self.next()
                    if tk.k == 'tv':
                        params.append(tk.v)
                self.eat(')')
            while not self.at('='):  # a constraint on the parameters
                self.next()
            self.eat('=')
            return N('type', t, name=name, params=params, typ=self.typ())
        if self.at('bitfield'):
            self.next()
            name = self.ident()
            self.eat(':')
            ty = self.typ()
            self.eat('=')
            self.eat('{')
            fields = []
            while not self.at('}'):
                f = self.ident()
                self.eat(':')
                hi = int(self.next().v)
                lo = hi
                if self.opt('..'):
                    lo = int(self.next().v)
                fields.append((f, hi, lo))
                self.opt(',')
            self.eat('}')
            return N('bitfield', t, name=name, typ=ty, fields=fields)
        if self.at('union'):
            self.next()
            name = self.ident()
            self.eat('=')
            self.eat('{')
            cons = []
            while not self.at('}'):
                c = self.ident()
                self.eat(':')
                cons.append((c, self.typ()))
                self.opt(',')
            self.eat('}')
            return N('union', t, name=name, cons=cons)
        if self.at('enum'):
            self.next()
            name = self.ident()
            self.eat('=')
            self.eat('{')
            items = []
            while not self.at('}'):
                items.append(self.ident())
                self.opt(',')
            self.eat('}')
            return N('enum', t, name=name, items=items)
        if self.at('struct'):
            self.next()
            name = self.ident()
            self.eat('=')
            self.eat('{')
            fields = []
            while not self.at('}'):
                f = self.ident()
                self.eat(':')
                fields.append((f, self.typ()))
                self.opt(',')
            self.eat('}')
            return N('struct', t, name=name, fields=fields)
        if self.at('register'):
            self.next()
            name = self.ident()
            self.eat(':')
            return N('register', t, name=name, typ=self.typ())
        if self.at('overload'):
            self.next()
            if self.opt('operator'):
                name = self.next().v
            else:
                name = self.ident()
            self.eat('=')
            self.eat('{')
            alts = []
            while not self.at('}'):
                alts.append(self.ident())
                self.opt(',')
            self.eat('}')
            return N('overload', t, name=name, alts=alts)
        if self.at('infix') or self.at('infixl') or self.at('infixr'):
            assoc = {'infix': 'n', 'infixl': 'l', 'infixr': 'r'}[self.next().v]
            p = int(self.next().v)
            op = self.next().v
            if BINOPS.get(op) != (p, assoc):
                self.fail('operator %s declared at a fixity the parser does not have' % op)
            return N('fixity', t)
        self.fail('expected a definition')


# ----------------------------------------------------------------------------- the model's files
class Model:
    """The files in $include order, each a list of definitions. A file is read once, where it
    is first included (the model guards its files with $ifndef for exactly this)."""

    def __init__(self, root, tops):
        self.root = root
        self.files = []   # (name, [definitions]) in load order
        self.seen = set()
        for f in tops:
            self.load(f)

    def load(self, fname):
        fname = os.path.normpath(fname)
        if fname in self.seen:
            return
        self.seen.add(fname)
        text = open(os.path.join(self.root, fname)).read()
        p = Parser(lex(text, fname))
        defs = []
        while True:
            d = p.top()
            if d is None:
                break
            if d.k == 'dir':
                if d.d == '$include':
                    if d.arg.startswith('<'):
                        continue  # Sail's own library: lib/bits.cub
                    self.load(d.arg.strip('"'))
                elif d.d not in ('$ifndef', '$define', '$endif'):
                    raise SailError('%s:%d: directive %s' % (d.file, d.line, d.d))
                continue
            defs.append(d)
        self.files.append((fname, defs))


# the files the model's Makefile gives the Sail compiler, in its order
TOPS = ['prelude.sail', 'register_types.sail', 'registers.sail', 'register_accessors.sail',
        'opcode_ext.sail', 'memory_accessors.sail', 'init.sail', 'config.sail', 'logging.sail',
        'step.sail', 'main.sail']


def parse_report(model):
    tot = {}
    for fname, defs in model.files:
        c = {}
        for d in defs:
            c[d.k] = c.get(d.k, 0) + 1
            tot[d.k] = tot.get(d.k, 0) + 1
        print('%-36s %s' % (fname, ' '.join('%s=%d' % kv for kv in sorted(c.items()))))
    print('%-36s %s' % ('total (%d files)' % len(model.files), ' '.join('%s=%d' % kv for kv in sorted(tot.items()))))


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    root = argv[1]
    try:
        model = Model(root, TOPS)
        if argv[2] == '--parse':
            parse_report(model)
            return 0
    except SailError as e:
        print('sail2cub: %s' % e, file=sys.stderr)
        return 1
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))

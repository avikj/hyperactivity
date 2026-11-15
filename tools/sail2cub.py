#!/usr/bin/env python3
"""sail2cub: the Sail x86 model (rems-project/sail-x86-from-acl2, model/) read into .cub.

  python3 tools/sail2cub.py MODEL_DIR --parse          per-file counts of what was parsed
  python3 tools/sail2cub.py MODEL_DIR --out lib/languages/isa/x86   write one .cub per .sail and all.cub

The Sail subset is the one the model uses (it is generated from ACL2's x86isa): val, function
(clauses, scattered), type synonyms, bitfield, union, enum, struct, register, overload, let, if,
match, blocks with assignment, foreach, try/catch, throw, return, assert, tuples, lists,
bitvector literals, slices and updates. A construct outside it stops the importer with the
file, line and token: it is then added here, never skipped.
"""
import os
import re
import sys


# every declaration written carries its reason (2026-11-12): the model's functions, types and
# registers are part of x86-64 (task-graph/hyperactivity.cub: every instruction translated), stated above each so
# tools/task-graph.py finds it whether or not the declaration is used
REASON = '-- (part of language-support)'
DECL_START = re.compile(r"^(data|record|codata) |^[^\s\-(][^\s]* :( |$)")
def with_reasons(text):
    out = []
    for line in text.split('\n'):
        if DECL_START.match(line) and not (out and out[-1] == REASON):
            out.append(REASON)
        out.append(line)
    return '\n'.join(out)

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
            if self.opt(','):  # the constraint: Sail has proved it; kept for choosing overloads
                depth = 0
                while not (depth == 0 and self.at('.')):
                    if self.peek().v in ('(', '{', '{|'):
                        depth += 1
                    if self.peek().v in (')', '}', '|}'):
                        depth -= 1
                    self.constraint.append(self.next().v)
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
            self.constraint = []
            tvs, ty = self.typschm()
            return N('val', t, name=name, tvs=tvs, typ=ty, ext=ext, constraint=self.constraint)
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
                        continue  # Sail's own library: lib/languages/bits.cub
                    self.load(d.arg.strip('"'))
                elif d.d not in ('$ifndef', '$define', '$endif'):
                    raise SailError('%s:%d: directive %s' % (d.file, d.line, d.d))
                continue
            defs.append(d)
        self.files.append((fname, defs))


# ----------------------------------------------------------------------------- types
# A Sail type here: ('bits', w) | ('int',) | ('bool',) | ('unit',) | ('string',) | ('bit',)
# | ('tuple', (..)) | ('option', T) | ('list', T) | ('vector', T) | ('enum', n) | ('union', n)
# | ('struct', n) | ('bitfield', n) | ('tv', n) | ('bot',).
# A width w is a Python int or a cub term of type Word (a string, an atom or parenthesized).
INT, BOOL, UNIT, STRING, BIT, BOT = ('int',), ('bool',), ('unit',), ('string',), ('bit',), ('bot',)


def w_code(w):
    return '#%d' % w if isinstance(w, int) else w


def w_add(a, b):
    if isinstance(a, int) and isinstance(b, int):
        return a + b
    if a == 0:
        return b
    if b == 0:
        return a
    return '(wadd %s %s)' % (w_code(a), w_code(b))


def w_sub(a, b):
    if isinstance(a, int) and isinstance(b, int):
        return max(a - b, 0)
    if b == 0:
        return a
    return '(wsub %s %s)' % (w_code(a), w_code(b))


def w_mul(a, b):
    if isinstance(a, int) and isinstance(b, int):
        return a * b
    if a == 1:
        return b
    if b == 1:
        return a
    return '(wmul %s %s)' % (w_code(a), w_code(b))


def w_pow2(a):
    if isinstance(a, int):
        return 1 << a
    return '(bv-pow2 %s)' % a


def z_lit(n):
    return '(zpos #%d)' % n if n >= 0 else '(zneg #%d)' % -n


def str_lit(s):
    return '#%d' % int.from_bytes(s.encode('utf-8'), 'big')


CUB_KEYWORDS = set('''data codata record where of let in mutual postulate case return if then else
    forall primitive foreign module open import instance variable infix infixl infixr deriving
    world the Type Level calc by using decreasing as do I comp transp hcomp fill glue unglue
    Glue Path PathP true false unit zero suc nil cons fst snd'''.split())

CLS = {'P': 0, 'F': 1, 'S': 2}


def cmax(a, b):
    return a if CLS[a] >= CLS[b] else b


# ----------------------------------------------------------------------------- documents
# Code is a list of lines; a one-line expression is an atom or parenthesized.
def ind(lines, n):
    return [' ' * n + l for l in lines]


def paren(lines):
    if len(lines) == 1:
        l = lines[0]
        if re.fullmatch(r"[#A-Za-z_][A-Za-z0-9_?'-]*", l) or (l.startswith('(') and balanced_outer(l)):
            return [l]
        return ['(' + l + ')']
    out = ['(' + lines[0]] + ind(lines[1:], 1)
    out[-1] += ')'
    return out


def balanced_outer(l):
    """Is l one parenthesized group?"""
    d = 0
    for i, c in enumerate(l):
        if c == '(':
            d += 1
        elif c == ')':
            d -= 1
            if d == 0 and i != len(l) - 1:
                return False
    return d == 0


def prefix(s, lines):
    return [s + lines[0]] + ind(lines[1:], len(s))


class Bind:
    """Something that happens before an expression's value is read: a let (cls L), an
    outcome (F: code : Outc exception T), a state step (S: code : MS T)."""

    def __init__(self, cls, var, ty, code, cond=None):
        self.cls, self.var, self.ty, self.code = cls, var, ty, code  # code: lines
        self.cond = cond  # an effectful conditional: (kind, head, arms, default), arms not yet code


class X:
    """A translated expression: what runs first, the value's code (one line), its type."""

    def __init__(self, code, ty, pre=None, nat=None, bot=False, lit=None):
        self.code, self.ty, self.pre, self.nat, self.bot, self.lit = code, ty, pre or [], nat, bot, lit

    @property
    def cls(self):
        c = 'P'
        for b in self.pre:
            if b.cls != 'L':
                c = cmax(c, b.cls)
        return c


class Importer:
    def __init__(self, model, reserved):
        self.model = model
        self.reserved = set(reserved) | CUB_KEYWORDS
        self.syn, self.bitfields, self.enums, self.enum_of = {}, {}, {}, {}
        self.unions, self.ctor, self.structs, self.registers = {}, {}, {}, {}
        self.vals, self.funs, self.overloads, self.constraints = {}, {}, {}, {}
        self.fun_order, self.file_of, self.decl_file = [], {}, {}
        self.notes = []          # what the importer decided, for the report
        self.undefined_uses = []
        for fname, defs in model.files:
            for d in defs:
                self.declare(fname, d)
        self.globals = set(self.funs) | set(self.vals) | set(self.syn) | set(self.bitfields) | \
            set(self.enums) | set(self.enum_of) | set(self.unions) | set(self.ctor) | set(self.structs)
        self.effects()

    # ---- declarations
    def declare(self, fname, d):
        k = d.k
        if k == 'type':
            self.syn[d.name] = (d.params, d.typ)
            self.decl_file[d.name] = fname
        elif k == 'bitfield':
            self.bitfields[d.name] = (d.typ, {f: (hi, lo) for f, hi, lo in d.fields}, d.fields)
        elif k == 'enum':
            self.enums[d.name] = d.items
            for it in d.items:
                self.enum_of[it] = d.name
        elif k == 'union':
            self.unions[d.name] = d.cons
            for c, t in d.cons:
                self.ctor[c] = (d.name, t)
        elif k == 'struct':
            self.structs[d.name] = d.fields
        elif k == 'register':
            self.registers[d.name] = d.typ
        elif k == 'val':
            self.vals[d.name] = (d.tvs, d.typ, d.ext)
            self.constraints[d.name] = d.constraint
        elif k == 'function':
            if d.name not in self.funs:
                self.funs[d.name] = []
                self.fun_order.append(d.name)
            self.funs[d.name].append(d)
            self.file_of[d.name] = fname  # a scattered function lives where its last clause is
            if d.name not in self.vals and d.pat.k == 'ptyped' and d.pat.typ.k == 'tfn':
                self.vals[d.name] = ([], d.pat.typ, None)  # function f(x) : t -> u = e
                d.pat = d.pat.p
        elif k == 'overload':
            self.overloads.setdefault(d.name, []).extend(d.alts)

    def err(self, n, msg):
        raise SailError('%s:%s: %s' % (getattr(n, 'file', '?'), getattr(n, 'line', '?'), msg))

    # ---- names
    def gname(self, name):
        """A Sail global's name here: its own, with _ appended if the library has it."""
        return name + '_' if name in self.reserved else name

    def fresh(self, base='t'):
        self.ntemp += 1
        return "%s'%d" % (base, self.ntemp)

    def bind_local(self, env, name, ty, nested=False):
        """Bind Sail local `name`; returns its cub name. A local never has a global's name;
        one that would capture (it leaves a nested scope, or a live width mentions the name
        it shadows) gets a fresh name."""
        cub = name
        if cub in self.reserved or cub in self.globals or cub in self.registers or cub == '_':
            cub = name + "'"
        if name in env['loc'] and (nested or cub in self.wnames):
            cub = self.fresh(name)
        env['loc'][name] = (ty, cub)
        self.note_width(ty)
        return cub

    def note_width(self, ty):
        if ty[0] == 'bits' and isinstance(ty[1], str):
            self.wnames.update(re.findall(r"[A-Za-z_][A-Za-z0-9_?'-]*", ty[1]))
        elif ty[0] in ('tuple',):
            for t in ty[1]:
                self.note_width(t)
        elif ty[0] in ('option', 'list', 'vector'):
            self.note_width(ty[1])

    # ---- types
    def nexp(self, t, tv):
        """A numeric type expression as a width (None if it mentions an unbound variable)."""
        if t.k == 'tnum':
            return t.n
        if t.k == 'tvar':
            return tv.get(t.name)
        if t.k == 'tbin':
            a, b = self.nexp(t.a, tv), self.nexp(t.b, tv)
            if a is None or b is None:
                return None
            if t.op == '+':
                return w_add(a, b)
            if t.op == '-':
                return w_sub(a, b)
            if t.op == '*':
                return w_mul(a, b)
            if t.op == '^' and a == 2:
                return w_pow2(b)
        if t.k == 'tid' and t.name in tv:
            return tv[t.name]
        self.err(t, 'numeric type expression %r' % t)

    def ty(self, t, tv):
        k = t.k
        if k == 'tid':
            n = t.name
            if n in ('int', 'nat'):
                return INT
            if n in ('bool', 'unit', 'string', 'bit'):
                return (n,)
            if n in self.syn:
                return self.ty(self.syn[n][1], {})
            if n in self.bitfields:
                return ('bitfield', n)
            if n in self.enums:
                return ('enum', n)
            if n in self.unions:
                return ('union', n)
            if n in self.structs:
                return ('struct', n)
            self.err(t, 'type %s' % n)
        if k == 'tapp':
            n = t.name
            if n in ('bits', 'sbits', 'bitvector'):
                return ('bits', self.nexp(t.args[0], tv))
            if n in ('int', 'range', 'atom', 'implicit'):
                return INT
            if n == 'option':
                return ('option', self.ty(t.args[0], tv))
            if n == 'list':
                return ('list', self.ty(t.args[0], tv))
            if n == 'vector':
                return ('vector', self.ty(t.args[-1], tv))
            if n in self.syn:
                ps, body = self.syn[n]
                return self.ty(body, {p: self.nexp(a, tv) for p, a in zip(ps, t.args)})
            self.err(t, 'type constructor %s' % n)
        if k == 'ttuple':
            return ('tuple', tuple(self.ty(x, tv) for x in t.items))
        if k in ('tset', 'tnum', 'tbin'):
            return INT
        if k == 'texist':
            return self.ty(t.body, tv)
        if k == 'tvar':
            if isinstance(tv.get(t.name), tuple):
                return tv[t.name]
            return ('tv', t.name)
        self.err(t, 'type %r' % t)

    def width(self, T, n=None):
        """The width of a bitvector-like type."""
        if T[0] == 'bits':
            if T[1] is None:
                self.err(n, 'a bitvector whose width is not known')
            return T[1]
        if T[0] == 'bit':
            return 1
        if T[0] == 'bitfield':
            return self.nexp(self.bitfields[T[1]][0].args[0], {})
        self.err(n, 'not a bitvector: %r' % (T,))

    def cty(self, T):
        """The cub type."""
        k = T[0]
        if k == 'bits':
            return 'Bits %s' % w_code(self.width(T))
        if k == 'bit':
            return 'Bits #1'
        if k == 'int':
            return 'ZI'
        if k == 'bool':
            return 'Bool'
        if k in ('unit', 'bot'):
            return 'Unit'
        if k == 'string':
            return 'Text'
        if k == 'tuple':
            return '(' + ' × '.join(self.cty_a(x) for x in T[1]) + ')'
        if k == 'option':
            return 'Option %s' % self.cty_a(T[1])
        if k in ('list', 'vector'):
            return 'List %s' % self.cty_a(T[1])
        if k in ('enum', 'union', 'struct', 'bitfield'):
            return self.gname(T[1])
        if k == 'tv':
            return 'T' + T[1][1:]
        raise SailError('type %r' % (T,))

    def cty_a(self, T):
        s = self.cty(T)
        return s if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_?'-]*|\(.*\)", s) and (not s.startswith('(') or balanced_outer(s)) else '(' + s + ')'

    def mty(self, cls, T):
        if cls == 'P':
            return self.cty(T)
        return '%s %s' % ('MF' if cls == 'F' else 'MS', self.cty_a(T))

    def default(self, T, n=None):
        """A value of the type: what an unwritten vector element or a bound that ran out reads."""
        k = T[0]
        if k in ('bits', 'bit', 'bitfield', 'string'):
            return '#0'
        if k == 'int':
            return '(zpos #0)'
        if k == 'bool':
            return 'false'
        if k in ('unit', 'bot'):
            return 'unit'
        if k == 'tuple':
            return self.tuple_code([self.default(x, n) for x in T[1]])
        if k == 'option':
            return 'None'
        if k in ('list', 'vector'):
            return 'nil'
        if k == 'enum':
            return self.gname(self.enums[T[1]][0])
        self.err(n, 'no default value of type %r' % (T,))

    def tuple_code(self, cs):
        return '(' + ' , '.join(cs) + ')'

    def proj(self, code, i, n):
        """Component i of an n-tuple (right-nested pairs)."""
        for _ in range(i):
            code = '(snd %s)' % code
        return code if i == n - 1 else '(fst %s)' % code

    # ---- effects: P pure, F outcome only, S state.  The least fixed point over the calls.
    def effects(self):
        direct, calls = {}, {}
        for f, clauses in self.funs.items():
            bound, c, cs = set(), ['P'], set()

            def binders(n):
                if n.k == 'pid':
                    bound.add(n.name)

            def visit(n):
                if n.k == 'id' and n.name in self.registers and n.name not in bound:
                    c[0] = 'S'
                elif n.k in ('throw', 'assert'):
                    c[0] = cmax(c[0], 'F')
                elif n.k == 'foreach':
                    c[0] = 'S'
                elif n.k == 'call':
                    if n.f in ('__read_mem', '__write_mem', '__write_mem_ea'):
                        c[0] = 'S'
                    for g in self.overloads.get(n.f, [n.f]):
                        cs.add(g)
            for d in clauses:
                walk(d.pat, binders)
                walk(d.body, binders)
            for d in clauses:
                walk(d.body, visit)
            direct[f], calls[f] = c[0], cs
        self.calls = calls
        cls = dict(direct)
        changed = True
        while changed:
            changed = False
            for f in self.funs:
                c = cls[f]
                for g in calls[f]:
                    if g in cls:
                        c = cmax(c, cls[g])
                if c != cls[f]:
                    cls[f], changed = c, True
        self.cls = cls


def walk(n, f):
    if isinstance(n, N):
        f(n)
        for a, b in n.__dict__.items():
            if a != 'ty':
                walk(b, f)
    elif isinstance(n, (list, tuple)):
        for x in n:
            walk(x, f)


# ----------------------------------------------------------------------------- expressions
BUILTIN_SHAPES = {  # for choosing among an overload's alternatives
    'and_vec': ['bits', 'bits'], 'or_vec': ['bits', 'bits'], 'xor_vec': ['bits', 'bits'],
    'not_vec': ['bits'], 'concat_str': ['string', 'string'], 'and_bool': ['bool', 'bool'],
    'or_bool': ['bool', 'bool'], 'sail_shiftleft': ['bits', 'int'], 'sail_shiftright': ['bits', 'int'],
    'add_atom': ['int', 'int'], 'add_bits': ['bits', 'bits'], 'add_bits_int': ['bits', 'int'],
    'sub_atom': ['int', 'int'], 'sub_bits': ['bits', 'bits'], 'sub_bits_int': ['bits', 'int'],
    'mult_atom': ['int', 'int'], 'append': ['bits', 'bits'], 'int_power': ['int', 'int'],
    'lt_int': ['int', 'int'], 'lteq_int': ['int', 'int'], 'gt_int': ['int', 'int'], 'gteq_int': ['int', 'int'],
}
OPERATORS = {  # Sail's library overloads, then the model's (prelude.sail) are appended
    '+': ['add_atom', 'add_bits', 'add_bits_int'], '-': ['sub_atom', 'sub_bits', 'sub_bits_int'],
    '*': ['mult_atom'], '&': ['and_bool', 'and_vec'], '|': ['or_bool', 'or_vec'],
    '@': ['append'], '^': ['xor_vec', 'int_power', 'concat_str'],
    '<': ['lt_int'], '<=': ['lteq_int'], '>': ['gt_int'], '>=': ['gteq_int'],
    '<<': [], '>>': [],
}
SIMPLE = re.compile(r"#?[A-Za-z0-9_][A-Za-z0-9_?'-]*$")


def shape(T):
    return 'bits' if T[0] == 'bit' else T[0]


class Translator(Importer):
    # ---- helpers
    def scope(self, env):
        e = dict(env)
        e['loc'] = dict(env['loc'])
        return e

    def nat(self, x):
        """An integer expression as a Word (a width, a shift count, an index)."""
        if x.nat is not None:
            return x.nat
        return '(z-nat %s)' % x.code

    def atomize(self, x, base='t'):
        """Make x's code a name or a literal (binding it if it is not)."""
        if SIMPLE.match(x.code):
            return x
        v = self.fresh(base)
        return X(v, x.ty, x.pre + [Bind('L', v, x.ty, [x.code])], x.nat, x.bot)

    def ret(self, cls, T, code):
        if cls == 'P':
            return code
        return '%s {%s} %s' % ('retF' if cls == 'F' else 'retS', self.cty(T), code)

    # ---- wrap: an expression with what runs before it, as code of class cls and type T
    # An effectful conditional is not bound like a call: what follows it is a function k (a
    # let, written once) applied at the end of every branch, and in state code the state is
    # passed into the branches. So a test on a variable leaves a tree whose leaves have run
    # to their end on the state they had, instead of everything after it run on an unknown state.
    def render_cond(self, cond, cls, T, armfn):
        kind, head, arms, default = cond
        # the state's name: σ, which no Sail identifier can be (they are ASCII), so it never captures
        # a model variable (a Sail local s is s' here)
        sfx = " σ" if cls == 'S' else ''

        def arm(x):
            d = paren(armfn(x))
            if sfx:  # the branch at its type (an applied term's head has no expected type), then the state
                d = ['the (%s)' % self.mty('S', T)] + ind(d, 2)  # (on its own line: × and → before a block would shift its column)
                d[-1] += sfx
            return d
        if kind == 'if':
            arms = list(arms)
            while True:  # else if: one chain
                lx = arms[-1][1]
                if len(lx.pre) == 1 and lx.pre[0].cond and lx.pre[0].cond[0] == 'if' and lx.pre[0].var == lx.code:
                    arms = arms[:-1] + list(lx.pre[0].cond[2])
                else:
                    break
            lines = []
            for k, (c, x) in enumerate(arms):
                if c is None:
                    lines.append('else')
                else:
                    lines.append(('if %s then' if k == 0 else 'else if %s then') % c)
                lines.extend(ind(arm(x), 2))
        elif kind == 'case':
            lines = ['case %s of' % head]
            for pt, x in arms:
                d = arm(x)
                if len(d) == 1 and len(d[0]) < 120:
                    lines.append('  %s → %s' % (pt, d[0]))
                else:
                    lines.append('  %s →' % pt)
                    lines.extend(ind(d, 4))
        else:  # 'tree': arms sorted (key, X); head (eq format, lt format)
            eq, lt = head
            dflt = arm(default)

            def build(items):
                if len(items) == 1:
                    kv, x = items[0]
                    return ['if %s then' % (eq % kv)] + ind(arm(x), 2) + ['else'] + ind(dflt, 2)
                mid = len(items) // 2
                return ['if %s then' % (lt % items[mid][0])] + ind(paren(build(items[:mid])), 2) + ['else'] + ind(paren(build(items[mid:])), 2)
            lines = build(arms)
        if cls == 'S':
            lines = ["λ σ →"] + ind(lines, 2)
        return lines

    def wrap(self, x, cls, T):
        pre = list(x.pre)
        tail = None
        tyT = self.cty(T)
        if pre and pre[-1].cls in ('F', 'S') and pre[-1].var == x.code:
            last = pre.pop()
            if last.cond:
                tail = self.render_cond(last.cond, cls, T, lambda a: self.wrap(a, cls, T))
            elif last.cls == cls:
                tail = last.code
            elif last.cls == 'F' and cls == 'S':
                tail = ['liftS {%s} %s' % (tyT, paren(last.code)[0])]
            else:
                raise SailError('class %s code inside a class %s function' % (last.cls, cls))
        if tail is None:
            tail = [self.ret(cls, T, x.code)]
        i = len(pre)
        while i > 0:
            b = pre[i - 1]
            if b.cls == 'L':
                j = i
                while j > 0 and pre[j - 1].cls == 'L':
                    j -= 1
                tail = self.let_block([(c.var, getattr(c, 'tystr', None) or self.cty(c.ty), c.code) for c in pre[j:i]], tail)
                i = j
                continue
            if cls == 'P':
                raise SailError('class %s code inside a pure function' % b.cls)
            if b.cls == 'S' and cls != 'S':
                raise SailError('state code inside a class F function')
            A = self.cty(b.ty)
            v = '_' if b.var.startswith('_') else b.var
            if b.cond:
                k = self.fresh('k')

                def then_k(a, k=k):
                    t = self.fresh()
                    return self.wrap(X(t, T, a.pre + [Bind(cls, t, T, ['%s %s' % (k, a.code)])]), cls, T)
                body = self.render_cond(b.cond, cls, T, then_k)
                tail = self.let_block([(k, '%s → %s' % (self.cty_a(b.ty), self.mty(cls, T)), ['λ %s →' % v] + ind(tail, 2))], body)
            else:
                op = 'bindS' if b.cls == 'S' else 'bindF' if cls == 'F' else 'onS'
                if len(b.code) != 1:
                    raise SailError('a multi-line effect that is not a conditional')
                head = '%s {%s} {%s} %s (λ %s →' % (op, A, tyT, paren(b.code)[0], v)
                tail = [head] + ind(tail, 2)
                tail[-1] += ')'
            i -= 1
        return tail

    def let_block(self, items, body):
        out = []
        for k, (v, t, code) in enumerate(items):
            lead = 'let ' if k == 0 else '    '
            out.append('%s%s : %s' % (lead, v, t))
            if len(code) == 1 and len(code[0]) < 160:
                out.append('    %s = %s' % (v, code[0]))
            else:
                out.append('    %s =' % v)
                out.extend(ind(code, 6))
        return out + prefix('in ', body)

    # ---- conditionals: arms [(condition code or None, X)]; the value joins at type T
    def join_type(self, xs, exp):
        if exp is not None:
            return exp
        for x in xs:
            if not x.bot and x.ty != BOT:
                return x.ty
        return None

    def branches(self, thunks, exp):
        """Translate alternatives (functions of the expected type) to one type."""
        xs = [t(exp) for t in thunks]
        T = self.join_type(xs, exp)
        if T is None:
            return xs, UNIT, True
        if exp is None:
            xs = [t(T) if (x.bot or x.ty == BOT) else x for t, x in zip(thunks, xs)]
        return xs, T, False

    def cond_result(self, pre, arms, T, bot):
        cls = 'P'
        for _, x in arms:
            cls = cmax(cls, x.cls)
        if cls == 'P' and all(not x.pre for _, x in arms) and sum(len(x.code) + len(c or '') for c, x in arms) < 110:
            code = arms[-1][1].code
            for c, x in reversed(arms[:-1]):
                code = '(if %s then %s else %s)' % (c, x.code, code)
            return X(code, T, pre, bot=bot)
        v = self.fresh()
        cond = ('if', None, arms, None)
        if cls == 'P':
            return X(v, T, pre + [Bind('L', v, T, self.render_cond(cond, 'P', T, lambda a: self.wrap(a, 'P', T)))], bot=bot)
        return X(v, T, pre + [Bind(cls, v, T, None, cond)], bot=bot)

    def case_result(self, pre, scrut, alts, T, bot):
        """alts [(pattern text, X)] of a `case scrut of`."""
        cls = 'P'
        for _, x in alts:
            cls = cmax(cls, x.cls)
        if cls == 'P' and all(not x.pre for _, x in alts) and sum(len(x.code) + len(p) for p, x in alts) < 100:
            return X('(case %s of %s)' % (scrut, '; '.join('%s → %s' % (p, x.code) for p, x in alts)), T, pre, bot=bot)
        v = self.fresh()
        cond = ('case', scrut, alts, None)
        if cls == 'P':
            return X(v, T, pre + [Bind('L', v, T, self.render_cond(cond, 'P', T, lambda a: self.wrap(a, 'P', T)))], bot=bot)
        return X(v, T, pre + [Bind(cls, v, T, None, cond)], bot=bot)

    # ---- the expression forms
    def tr(self, e, env, exp=None, tail=False, nested=True):
        m = getattr(self, 'tr_' + e.k, None)
        if m is None:
            self.err(e, 'expression form %s' % e.k)
        return m(e, env, exp, tail, nested)

    def tr_int(self, e, env, exp, tail, nested):
        return X(z_lit(e.n), INT, nat=e.n if e.n >= 0 else None, lit=e.n)

    def tr_bitsl(self, e, env, exp, tail, nested):
        return X('#%d' % e.n, ('bits', e.w))

    def tr_str(self, e, env, exp, tail, nested):
        return X(str_lit(e.s), STRING)

    def tr_bool(self, e, env, exp, tail, nested):
        return X('true' if e.b else 'false', BOOL)

    def tr_bit(self, e, env, exp, tail, nested):
        return X('#1' if e.b else '#0', BIT)

    def tr_unit(self, e, env, exp, tail, nested):
        return X('unit', UNIT)

    def tr_undefined(self, e, env, exp, tail, nested):
        if exp is None:
            self.err(e, 'undefined at an unknown type')
        self.undefined_uses.append('%s:%d' % (e.file, e.line))
        return X(self.default(exp, e), exp)

    def tr_tvval(self, e, env, exp, tail, nested):
        w = env['tv'].get(e.name)
        if w is None:
            self.err(e, 'type variable %s has no value here' % e.name)
        return X('(zpos %s)' % w_code(w), INT, nat=w if isinstance(w, int) else None)

    def reg_type(self, name):
        return self.ty(self.registers[name], {})

    def tr_id(self, e, env, exp, tail, nested):
        n = e.name
        if n in env['loc']:
            T, cub = env['loc'][n]
            return X(cub, T)
        if n in self.registers:
            T = self.reg_type(n)
            v = self.fresh(n)
            return X(v, T, [Bind('S', v, T, ['getS {%s} x86-%s' % (self.cty(T), n)])])
        if n in self.enum_of:
            return X(self.gname(n), ('enum', self.enum_of[n]))
        if n in self.ctor:
            return X(self.gname(n), ('union', self.ctor[n][0]))
        self.err(e, 'unbound name %s' % n)

    def tr_cast(self, e, env, exp, tail, nested):
        T = self.ty(e.typ, env['tv'])
        x = self.tr(e.e, env, T, tail, nested)
        return X(x.code, T, x.pre, x.nat, x.bot)

    def tr_tuple(self, e, env, exp, tail, nested):
        exps = exp[1] if exp and exp[0] == 'tuple' else [None] * len(e.items)
        xs = [self.tr(a, env, t) for a, t in zip(e.items, exps)]
        pre = [b for x in xs for b in x.pre]
        return X(self.tuple_code([x.code for x in xs]), ('tuple', tuple(x.ty for x in xs)), pre)

    def tr_list(self, e, env, exp, tail, nested):
        et = exp[1] if exp and exp[0] == 'list' else None
        xs = [self.tr(a, env, et) for a in e.items]
        if et is None:
            if not xs:
                self.err(e, 'an empty list at an unknown type')
            et = xs[0].ty
        code = 'nil'
        for x in reversed(xs):
            code = '(cons %s %s)' % (x.code, code)
        return X(code, ('list', et), [b for x in xs for b in x.pre])

    def tr_vec(self, e, env, exp, tail, nested):
        et = exp[1] if exp and exp[0] == 'vector' else None
        xs = [self.tr(a, env, et) for a in e.items]
        pre = [b for x in xs for b in x.pre]
        if xs[0].ty[0] in ('bit', 'bits'):  # a bitvector, highest bit first
            code, w = xs[0].code, self.width(xs[0].ty, e)
            for x in xs[1:]:
                wx = self.width(x.ty, e)
                code = '(bv-append %s %s %s %s)' % (w_code(w), w_code(wx), code, x.code)
                w = w_add(w, wx)
            return X(code, ('bits', w), pre)
        code = 'nil'  # dec order: the last written is index 0
        for x in xs:
            code = '(cons %s %s)' % (x.code, code)
        return X(code, ('vector', xs[0].ty), pre)

    def tr_structlit(self, e, env, exp, tail, nested):
        if not exp or exp[0] != 'struct':
            self.err(e, 'a struct literal at an unknown type')
        given = dict(e.fs)
        xs = [self.tr(given[f], env, self.ty(t, {})) for f, t in self.structs[exp[1]]]
        return X('(%s-mk %s)' % (self.gname(exp[1]), ' '.join(x.code for x in xs)), exp, [b for x in xs for b in x.pre])

    def tr_field(self, e, env, exp, tail, nested):
        x = self.tr(e.e, env)
        if x.ty[0] == 'bitfield' and e.f == 'bits':
            return X(x.code, ('bits', self.width(x.ty)), x.pre)
        if x.ty[0] == 'struct':
            ft = dict(self.structs[x.ty[1]])[e.f]
            return X('(%s-%s %s)' % (self.gname(x.ty[1]), e.f, x.code), self.ty(ft, {}), x.pre)
        self.err(e, 'field %s of %r' % (e.f, x.ty))

    def bf_field(self, T, i):
        """The field a bitfield index names, or None."""
        if T[0] == 'bitfield' and i.k == 'id' and i.name in self.bitfields[T[1]][1]:
            return i.name
        return None

    def tr_index(self, e, env, exp, tail, nested):
        x = self.tr(e.e, env)
        f = self.bf_field(x.ty, e.i)
        if f:
            hi, lo = self.bitfields[x.ty[1]][1][f]
            return X('(%s-%s %s)' % (self.gname(x.ty[1]), f, x.code), ('bits', hi - lo + 1), x.pre)
        i = self.tr(e.i, env, INT)
        pre = x.pre + i.pre
        if x.ty[0] in ('bits', 'bitfield'):
            return X('(bv-bit %s %s %s)' % (w_code(self.width(x.ty, e)), x.code, w_code(self.nat(i))), BIT, pre)
        if x.ty[0] == 'vector':
            et = x.ty[1]
            return X('(vec-get {%s} %s %s %s)' % (self.cty(et), self.default(et, e), x.code, w_code(self.nat(i))), et, pre)
        self.err(e, 'index into %r' % (x.ty,))

    def tr_slice(self, e, env, exp, tail, nested):
        x = self.tr(e.e, env)
        hi, lo = self.tr(e.hi, env, INT), self.tr(e.lo, env, INT)
        h, l = self.nat(hi), self.nat(lo)
        w = w_add(w_sub(h, l), 1)
        return X('(bv-slice %s %s %s %s)' % (w_code(self.width(x.ty, e)), x.code, w_code(h), w_code(l)), ('bits', w),
                 x.pre + hi.pre + lo.pre)

    def tr_update(self, e, env, exp, tail, nested):
        x = self.tr(e.e, env, exp)
        code, pre, T = x.code, list(x.pre), x.ty
        for kind, a, lo, v in e.ups:
            if kind == 'slice':
                hi, lo = self.tr(a, env, INT), self.tr(lo, env, INT)
                h, l = self.nat(hi), self.nat(lo)
                vx = self.tr(v, env, ('bits', w_add(w_sub(h, l), 1)))
                pre += hi.pre + lo.pre + vx.pre
                code = '(bv-update %s %s %s %s %s)' % (w_code(self.width(T, e)), code, w_code(h), w_code(l), vx.code)
                continue
            f = self.bf_field(T, a)
            if f:
                hi, lo = self.bitfields[T[1]][1][f]
                vx = self.tr(v, env, ('bits', hi - lo + 1))
                pre += vx.pre
                code = '(%s-set-%s %s %s)' % (self.gname(T[1]), f, code, vx.code)
            elif T[0] == 'vector':
                ix, vx = self.tr(a, env, INT), self.tr(v, env, T[1])
                pre += ix.pre + vx.pre
                code = '(vec-set {%s} %s %s %s)' % (self.cty(T[1]), code, w_code(self.nat(ix)), vx.code)
            elif T[0] in ('bits', 'bitfield'):
                ix, vx = self.tr(a, env, INT), self.tr(v, env, BIT)
                pre += ix.pre + vx.pre
                n = w_code(self.nat(ix))
                code = '(bv-update %s %s %s %s %s)' % (w_code(self.width(T, e)), code, n, n, vx.code)
            else:
                self.err(e, 'update of %r' % (T,))
        return X(code, T, pre)

    # ---- sequencing
    def tr_block(self, e, env, exp, tail, nested):
        env = self.scope(env)
        pre, res = [], X('unit', UNIT)
        for i, s in enumerate(e.ss):
            last = i == len(e.ss) - 1
            if s.k == 'letstmt':
                self.do_let(s.pat, s.e, env, pre, nested)
                res = X('unit', UNIT)
            elif s.k == 'assign':
                self.do_assign(s, env, pre, nested)
                res = X('unit', UNIT)
            else:
                x = self.tr(s, env, exp if last else UNIT, tail and last, nested if last else True)
                pre += x.pre
                res = X(x.code, x.ty, [], x.nat, x.bot)
        return X(res.code, res.ty, pre, res.nat, res.bot)

    def tr_letstmt(self, e, env, exp, tail, nested):  # a let that ends its block
        pre = []
        self.do_let(e.pat, e.e, self.scope(env), pre, nested)
        return X('unit', UNIT, pre)

    def tr_assign(self, e, env, exp, tail, nested):
        pre = []
        self.do_assign(e, env, pre, nested)
        return X('unit', UNIT, pre)

    def tr_let(self, e, env, exp, tail, nested):
        env = self.scope(env)
        pre = []
        self.do_let(e.pat, e.e, env, pre, nested)
        x = self.tr(e.body, env, exp, tail, nested)
        return X(x.code, x.ty, pre + x.pre, x.nat, x.bot)

    def pat_type(self, p, env):
        if p.k == 'ptyped':
            return self.ty(p.typ, env['tv'])
        if p.k == 'ptuple':
            ts = [self.pat_type(q, env) for q in p.items]
            if all(t is not None for t in ts):
                return ('tuple', tuple(ts))
        return None

    def do_let(self, pat, e, env, pre, nested):
        T = self.pat_type(pat, env)
        x = self.tr(e, env, T, False, True)
        pre += x.pre
        self.bind_pat(pat, x.code, T or x.ty, env, pre, nested)

    def bind_pat(self, p, code, T, env, pre, nested):
        if p.k == 'ptyped':
            return self.bind_pat(p.p, code, self.ty(p.typ, env['tv']), env, pre, nested)
        if p.k == 'pwild' or (p.k == 'plit' and p.lit.k == 'unit'):
            return
        if p.k == 'pid':
            cub = self.bind_local(env, p.name, T, nested)
            if pre and pre[-1].var == code and "'" in code and code != cub and not pre[-1].var.startswith('_'):
                pre[-1].var, pre[-1].ty = cub, T  # the temporary takes the let's name
            elif code != cub:
                pre.append(Bind('L', cub, T, [code]))
            return
        if p.k == 'ptuple':
            if T[0] != 'tuple' or len(T[1]) != len(p.items):
                self.err(p, 'a tuple pattern against %r' % (T,))
            if not SIMPLE.match(code):
                v = self.fresh()
                pre.append(Bind('L', v, T, [code]))
                code = v
            for i, q in enumerate(p.items):
                self.bind_pat(q, self.proj(code, i, len(p.items)), T[1][i], env, pre, nested)
            return
        self.err(p, 'pattern %s in a let' % p.k)

    def do_assign(self, s, env, pre, nested):
        lhs = s.lhs
        if lhs.k == 'id':
            n = lhs.name
            if n in env['loc'] or n not in self.registers:
                T = self.ty(s.typ, env['tv']) if s.typ else (env['loc'][n][0] if n in env['loc'] else None)
                x = self.tr(s.e, env, T, False, True)
                pre += x.pre
                if n in env.get('frozen', ()):
                    self.err(s, 'assignment to %s from inside a branch' % n)
                self.bind_pat(N('pid', None, name=n), x.code, T or x.ty, env, pre, False)
                return
            T = self.reg_type(n)
            x = self.tr(s.e, env, T, False, True)
            pre += x.pre
            pre.append(Bind('S', self.fresh('_'), UNIT, ['putS (x86-set-%s %s)' % (n, x.code)]))
            return
        if lhs.k == 'index' and lhs.e.k == 'id' and lhs.e.name in self.registers and lhs.e.name not in env['loc']:
            n = lhs.e.name
            T = self.reg_type(n)
            cur = self.tr(lhs.e, env)
            ix = self.tr(lhs.i, env, INT)
            v = self.tr(s.e, env, T[1], False, True)
            pre += ix.pre + v.pre + cur.pre
            pre.append(Bind('S', self.fresh('_'), UNIT, ['putS (x86-set-%s (vec-set {%s} %s %s %s))' % (
                n, self.cty(T[1]), cur.code, w_code(self.nat(ix)), v.code)]))
            return
        if lhs.k == 'slice' and lhs.e.k == 'id' and lhs.e.name in env['loc']:
            n = lhs.e.name
            T, cub = env['loc'][n]
            hi, lo = self.tr(lhs.hi, env, INT), self.tr(lhs.lo, env, INT)
            h, l = self.nat(hi), self.nat(lo)
            v = self.tr(s.e, env, ('bits', w_add(w_sub(h, l), 1)), False, True)
            pre += hi.pre + lo.pre + v.pre
            code = '(bv-update %s %s %s %s %s)' % (w_code(self.width(T, s)), cub, w_code(h), w_code(l), v.code)
            self.bind_pat(N('pid', None, name=n), code, T, env, pre, False)
            return
        self.err(s, 'assignment to %s' % lhs.k)

    def branch(self, e, env, exp, tail):
        """A branch: its own scope; its bindings stay inside it. A branch that is not the
        function's tail may not assign a variable of the enclosing scope (nothing would carry
        the new value out)."""
        env = self.scope(env)
        if not tail:
            env['frozen'] = set(env['loc'])
        return self.tr(e, env, exp, tail, False)

    def tr_if(self, e, env, exp, tail, nested):
        c = self.tr(e.c, env, BOOL)
        thunks = [lambda t: self.branch(e.a, env, t, tail),
                  (lambda t: self.branch(e.b, env, t, tail)) if e.b is not None else (lambda t: X('unit', UNIT))]
        xs, T, bot = self.branches(thunks, exp if e.b is not None else UNIT)
        return self.cond_result(c.pre, [(c.code, xs[0]), (None, xs[1])], T, bot)

    def lit_test(self, scrut, T, lit, n):
        k = T[0]
        if lit.k == 'unit':
            return 'true'
        if k == 'int':
            if lit.k != 'int':
                self.err(n, 'pattern %s against an integer' % lit.k)
            return '(z-eq %s %s)' % (scrut, z_lit(lit.n))
        if k in ('bits', 'bit', 'bitfield'):
            v = lit.n if lit.k in ('int', 'bitsl') else (1 if lit.b else 0)
            return '(weq %s #%d)' % (scrut, v)
        if k == 'string':
            return '(str-eq %s %s)' % (scrut, str_lit(lit.s))
        if k == 'bool':
            return scrut if lit.b else '(b-not %s)' % scrut
        self.err(n, 'literal pattern against %r' % (T,))

    def pat_arm(self, p, scrut, T, env):
        """A pattern over literals, names, wildcards and tuples of them:
        (condition or None, bindings [(name, code, type)])."""
        if p.k == 'ptyped':
            return self.pat_arm(p.p, scrut, self.ty(p.typ, env['tv']), env)
        if p.k == 'pwild':
            return None, []
        if p.k == 'pid':
            if p.name in self.enum_of:
                self.err(p, 'enumeration member in a literal match')
            return None, [(p.name, scrut, T)]
        if p.k == 'plit':
            return self.lit_test(scrut, T, p.lit, p), []
        if p.k == 'ptuple' and T[0] == 'tuple':
            conds, binds = [], []
            for i, q in enumerate(p.items):
                c, b = self.pat_arm(q, self.proj(scrut, i, len(p.items)), T[1][i], env)
                if c:
                    conds.append(c)
                binds += b
            cond = None
            for c in reversed(conds):
                cond = c if cond is None else '(b-and %s %s)' % (c, cond)
            return cond, binds
        self.err(p, 'pattern %s against %r' % (p.k, T))

    def arm_body(self, body, binds, env, exp, tail):
        env = self.scope(env)
        if not tail:
            env['frozen'] = set(env['loc'])
        pre = []
        for name, code, T in binds:
            cub = self.bind_local(env, name, T, False)
            if cub != code:
                pre.append(Bind('L', cub, T, [code]))
        x = self.tr(body, env, exp, tail, False)
        return X(x.code, x.ty, pre + x.pre, x.nat, x.bot)

    def tr_match(self, e, env, exp, tail, nested):
        s = self.atomize(self.tr(e.e, env), 'm')
        T = s.ty
        cases = e.cases
        k = T[0]
        if k in ('enum', 'option', 'list', 'union'):
            alts, thunks = [], []
            for p, body in cases:
                while p.k == 'ptyped':
                    p = p.p
                binds = []
                if p.k == 'pwild':
                    pt = '_'
                elif p.k == 'pid' and (p.name in self.enum_of or p.name in self.ctor):
                    pt = self.gname(p.name)
                elif p.k == 'pid':
                    pt, binds = '_', [(p.name, s.code, T)]
                elif p.k == 'pnil':
                    pt = 'nil'
                elif p.k == 'pcons':
                    h, t = self.fresh('h'), self.fresh('r')
                    pt = 'cons %s %s' % (h, t)
                    binds = self.sub_binds(p.h, h, T[1]) + self.sub_binds(p.t, t, T)
                elif p.k == 'papp' and p.name == 'None':
                    pt = 'None'
                elif p.k == 'papp' and p.name == 'Some':
                    v = self.fresh('v')
                    pt = 'Some %s' % v
                    binds = self.sub_binds(p.args[0], v, T[1])
                elif p.k == 'papp' and p.name in self.ctor:
                    at = self.ty(self.ctor[p.name][1], {})
                    if at == UNIT:
                        pt = self.gname(p.name)
                    else:
                        v = self.fresh('v')
                        pt = '%s %s' % (self.gname(p.name), v)
                        arg = p.args[0] if len(p.args) == 1 else N('ptuple', p, items=p.args)
                        binds = self.sub_binds(arg, v, at)
                else:
                    self.err(p, 'pattern %s against %r' % (p.k, T))
                alts.append(pt)
                thunks.append(lambda t, body=body, binds=binds: self.arm_body(body, binds, env, t, tail))
            xs, RT, bot = self.branches(thunks, exp)
            return self.case_result(s.pre, s.code, list(zip(alts, xs)), RT, bot)
        arms, thunks, vals, parts = [], [], [], []
        for p, body in cases:
            c, binds = self.pat_arm(p, s.code, T, env)
            arms.append(c)
            q = p
            while q.k == 'ptyped':
                q = q.p
            vals.append(q.lit.n if q.k == 'plit' and q.lit.k in ('int', 'bitsl') and q.lit.n >= 0 else None)
            parts.append((body, binds))
            thunks.append(lambda t, body=body, binds=binds: self.arm_body(body, binds, env, t, tail))
            if c is None:
                break
        if arms[-1] is not None:
            # no wildcard: Sail accepted the match as complete, so the last case is the rest
            arms[-1] = None
        xs, RT, bot = self.branches(thunks, exp)
        if len(arms) == 1:
            x = xs[0]
            return X(x.code, x.ty, s.pre + x.pre, x.nat, x.bot)
        if len(arms) >= 8 and self.can_lift(env):
            # a large match: each case is a definition of its own, and the literals are found
            # by halving (the same case is reached; the checker meets small definitions)
            xs = [self.lift(x, RT, body, binds, env, s) for x, (body, binds) in zip(xs, parts)]
            keys = vals[:-1]
            if k in ('int', 'bits', 'bit', 'bitfield') and all(v is not None for v in keys) and len(set(keys)) == len(keys):
                return self.tree_result(s, T, sorted(zip(keys, xs[:-1]), key=lambda kv: kv[0]), xs[-1], RT, bot)
        return self.cond_result(s.pre, list(zip(arms, xs)), RT, bot)

    def can_lift(self, env):
        return not env.get('rec') and not any(isinstance(v, (int, str)) for v in env['tv'].values())

    def lift(self, x, T, body, binds, env, s):
        """The arm x as a definition of its own over the locals it mentions."""
        if not x.pre and len(x.code) < 100:
            return x
        used = []

        def v(n):
            if n.k == 'id' and n.name in env['loc'] and n.name not in used:
                used.append(n.name)
        walk(body, v)
        params = [(env['loc'][n][1], env['loc'][n][0]) for n in env['loc'] if n in used]
        if binds and SIMPLE.match(s.code) and not s.code.startswith('#') and all(c != s.code for c, _ in params):
            params.append((s.code, s.ty))
        for c, pt in params:
            if pt[0] == 'bits' and isinstance(pt[1], str):
                raise SailError('a lifted case takes %s whose width is not a constant' % c)
        cls = x.cls
        self.nhelper += 1
        name = '%s-case%d' % (self.gname(self.cur), self.nhelper)
        sig = ''.join('(%s : %s) → ' % (c, self.cty(pt)) for c, pt in params) + self.mty(cls, T)
        self.helpers += ['%s : %s' % (name, sig), ' '.join([name] + [c for c, _ in params]) + ' ='] + ind(self.wrap(x, cls, T), 2)
        call = ' '.join([name] + [c for c, _ in params])
        if cls == 'P':
            return X('(%s)' % call if params else call, T, bot=x.bot)
        t = self.fresh()
        return X(t, T, [Bind(cls, t, T, [call])], bot=x.bot)

    def tree_result(self, s, T, keyed, default, RT, bot):
        cls = default.cls
        for _, x in keyed:
            cls = cmax(cls, x.cls)
        if T[0] == 'int':
            head = ('(z-eq %s (zpos #%%d))' % s.code, '(z-lt %s (zpos #%%d))' % s.code)
        else:
            head = ('(weq %s #%%d)' % s.code, '(wlt %s #%%d)' % s.code)
        v = self.fresh()
        cond = ('tree', head, keyed, default)
        if cls == 'P':
            return X(v, RT, s.pre + [Bind('L', v, RT, self.render_cond(cond, 'P', RT, lambda a: self.wrap(a, 'P', RT)))], bot=bot)
        return X(v, RT, s.pre + [Bind(cls, v, RT, None, cond)], bot=bot)

    def sub_binds(self, p, code, T):
        while p.k == 'ptyped':
            p = p.p
        if p.k == 'pwild' or (p.k == 'plit' and p.lit.k == 'unit'):
            return []
        if p.k == 'pid':
            return [(p.name, code, T)]
        if p.k == 'ptuple' and T[0] == 'tuple':
            out = []
            for i, q in enumerate(p.items):
                out += self.sub_binds(q, self.proj(code, i, len(p.items)), T[1][i])
            return out
        self.err(p, 'pattern %s inside a constructor pattern' % p.k)

    def tr_return(self, e, env, exp, tail, nested):
        if not tail:
            self.err(e, 'return outside tail position')
        return self.tr(e.e, env, env['ret'], True, nested)

    def tr_throw(self, e, env, exp, tail, nested):
        x = self.tr(e.e, env, ('union', 'exception'))
        T = exp if exp is not None else UNIT
        v = self.fresh()
        return X(v, T, x.pre + [Bind('F', v, T, ['out-throw %s' % x.code])], bot=exp is None)

    def tr_assert(self, e, env, exp, tail, nested):
        c = self.tr(e.c, env, BOOL)
        msg = self.tr(e.msg, env, STRING) if e.msg is not None else X(str_lit('%s:%d' % (e.file, e.line)), STRING)
        return X('unit', UNIT, c.pre + msg.pre + [Bind('F', self.fresh('_'), UNIT, ['fl-assert {exception} %s %s' % (c.code, msg.code)])])

    def assigned(self, n, env):
        out = []

        def v(m):
            if m.k == 'assign':
                r = m.lhs
                while r.k in ('index', 'slice'):
                    r = r.e
                if r.k == 'id' and r.name in env['loc'] and r.name not in out:
                    out.append(r.name)
        walk(n, v)
        return out

    def tr_foreach(self, e, env, exp, tail, nested):
        if e.down:
            self.err(e, 'foreach downto')
        lo, hi = self.tr(e.lo, env, INT), self.tr(e.hi, env, INT)
        carried = self.assigned(e.body, env)
        ctypes = [env['loc'][n][0] for n in carried]
        accT = UNIT if not carried else ctypes[0] if len(carried) == 1 else ('tuple', tuple(ctypes))
        acc0 = 'unit' if not carried else env['loc'][carried[0]][1] if len(carried) == 1 else \
            self.tuple_code([env['loc'][n][1] for n in carried])
        benv = self.scope(env)
        iv = self.bind_local(benv, e.v, INT, False)
        acc = self.fresh('acc')
        bpre = []
        for i, n in enumerate(carried):
            cub = self.bind_local(benv, n, ctypes[i], False)
            bpre.append(Bind('L', cub, ctypes[i], [acc if len(carried) == 1 else self.proj(acc, i, len(carried))]))
        bx = self.tr(e.body, benv, UNIT, False, False)
        out = 'unit' if not carried else benv['loc'][carried[0]][1] if len(carried) == 1 else \
            self.tuple_code([benv['loc'][n][1] for n in carried])
        body = self.wrap(X(out, accT, bpre + bx.pre), 'S', accT)
        f = self.fresh('loop')
        res = self.fresh('acc')
        pre = lo.pre + hi.pre
        pre.append(Bind('L', f, None, ['λ %s %s →' % (iv, acc)] + ind(body, 2)))
        pre[-1].tystr = 'ZI → %s → MS %s' % (self.cty_a(accT), self.cty_a(accT))
        pre.append(Bind('S', res, accT, ['foreachS {%s} %s %s %s %s' % (self.cty(accT), lo.code, hi.code, acc0, f)]))
        for i, n in enumerate(carried):
            cub = self.bind_local(env, n, ctypes[i], True)
            pre.append(Bind('L', cub, ctypes[i], [res if len(carried) == 1 else self.proj(res, i, len(carried))]))
        return X('unit', UNIT, pre)

    def tr_try(self, e, env, exp, tail, nested):
        handlers = []
        ex = self.fresh('ex')

        def body(t):
            return self.branch(e.e, env, t, tail)
        thunks = [body]
        alts = []
        seen = set()
        for p, b in e.cases:
            if p.k == 'papp' and p.name in self.ctor:
                at = self.ty(self.ctor[p.name][1], {})
                seen.add(p.name)
                if at == UNIT:
                    pt, binds = self.gname(p.name), []
                else:
                    v = self.fresh('v')
                    pt, binds = '%s %s' % (self.gname(p.name), v), self.sub_binds(p.args[0], v, at)
            elif p.k in ('pwild', 'pid'):
                pt, binds = '_', ([(p.name, ex, ('union', 'exception'))] if p.k == 'pid' else [])
                seen.update(c for c, _ in self.unions['exception'])
            else:
                self.err(p, 'handler pattern %s' % p.k)
            alts.append(pt)
            thunks.append(lambda t, b=b, binds=binds: self.arm_body(b, binds, env, t, tail))
        xs, T, bot = self.branches(thunks, exp)
        cls = 'F'
        for x in xs:
            cls = cmax(cls, x.cls)
        hx = list(zip(alts, xs[1:]))
        if any(c not in seen for c, _ in self.unions['exception']):  # an exception no handler names goes on
            v = self.fresh()
            hx.append(('_', X(v, T, [Bind('F', v, T, ['out-throw %s' % ex])])))
        lines = ['case %s of' % ex]
        for pt, x in hx:
            lines.append('  %s →' % pt)
            lines.extend(ind(paren(self.wrap(x, cls, T)), 4))
        m, h, v = self.fresh('try'), self.fresh('handle'), self.fresh()
        pre = [Bind('L', m, None, self.wrap(xs[0], cls, T)), Bind('L', h, None, ['λ %s →' % ex] + ind(lines, 2))]
        pre[0].tystr = self.mty(cls, T)
        pre[1].tystr = 'exception → %s' % self.mty(cls, T)
        pre.append(Bind(cls, v, T, ['%s {%s} %s %s' % ('tryF' if cls == 'F' else 'tryS', self.cty(T), m, h)]))
        return X(v, T, pre, bot=bot)

    # ---- calls
    def P(self, xs):
        return [b for x in xs for b in x.pre]

    def infer_args(self, args, env):
        return [self.tr(a, env) for a in args]

    def ast_shape(self, t):
        k = t.k
        if k == 'tid':
            n = t.name
            if n in ('int', 'nat'):
                return 'int'
            if n in ('bool', 'unit', 'string'):
                return n
            if n == 'bit':
                return 'bits'
            if n in self.syn:
                return self.ast_shape(self.syn[n][1])
            if n in self.bitfields:
                return 'bitfield:' + n
            for tab, s in ((self.enums, 'enum'), (self.unions, 'union'), (self.structs, 'struct')):
                if n in tab:
                    return s
        if k == 'tapp':
            if t.name in ('bits', 'sbits', 'bitvector'):
                return 'bits'
            if t.name in ('int', 'range', 'atom', 'implicit'):
                return 'int'
            if t.name in ('option', 'list', 'vector'):
                return t.name
            if t.name in self.syn:
                return self.ast_shape(self.syn[t.name][1])
        if k == 'ttuple':
            return 'tuple'
        if k in ('tset', 'tnum', 'tbin'):
            return 'int'
        if k == 'texist':
            return self.ast_shape(t.body)
        return 'any'

    def formals_of(self, g):
        typ = self.vals[g][1]
        a = typ.arg
        if a.k == 'ttuple':
            return a.items
        if a.k == 'tid' and a.name == 'unit':
            return []
        return [a]

    def violates(self, g, xs, strict):
        """Does a call of g with these arguments break g's constraint on an int('n) parameter?
        Strictly: a constraint on a value that is not a literal is not known to hold."""
        toks = self.constraints.get(g) or []
        formals = self.formals_of(g)
        arg = {}
        for f, x in zip(formals, xs):
            if f.k == 'tapp' and f.name in ('int', 'atom', 'implicit') and len(f.args) == 1 and f.args[0].k == 'tvar':
                arg[f.args[0].name] = x
        i = 0
        while i + 2 < len(toks):
            v, op, k = toks[i], toks[i + 1], toks[i + 2]
            if v in arg and op in ('>=', '>', '<=', '<') and re.fullmatch(r'-?\d+', k):
                x, k = arg[v], int(k)
                if x.lit is None:
                    if strict:
                        return True
                elif not {'>=': x.lit >= k, '>': x.lit > k, '<=': x.lit <= k, '<': x.lit < k}[op]:
                    return True
            i += 1
        return False

    def choose(self, names, xs, e):
        """The first alternative of an overload whose parameter shapes fit the arguments and
        whose constraints hold: first among those whose constraints are known to hold (on
        literals), then among those not known to fail."""
        for strict in (True, False):
            for g in names:
                if g in BUILTIN_SHAPES:
                    shapes = BUILTIN_SHAPES[g]
                elif g in self.vals and g in self.funs:
                    shapes = [self.ast_shape(f) for f in self.formals_of(g)]
                elif g in self.BI:
                    return g
                else:
                    continue
                if len(shapes) != len(xs):
                    continue
                ok = True
                for sh, x in zip(shapes, xs):
                    a = shape(x.ty)
                    if a == 'bitfield':
                        a = 'bitfield:' + x.ty[1]
                    if not (sh == 'any' or a == 'bot' or x.bot or sh == a):
                        ok = False
                if ok and g in self.funs and self.violates(g, xs, strict):
                    ok = False
                if ok:
                    return g
        self.err(e, 'no alternative of %s fits %s' % (names, [x.ty for x in xs]))

    def closed(self, T):
        if T is None:
            return False
        if T[0] == 'bits':
            return T[1] is not None
        if T[0] == 'tv':
            return False
        if T[0] == 'tuple':
            return all(self.closed(t) for t in T[1])
        if T[0] in ('option', 'list', 'vector'):
            return self.closed(T[1])
        return True

    def closed_ty(self, ast, tv):
        T = self.ty(ast, tv)
        return T if self.closed(T) else None

    def has_bot(self, x):
        def hb(T):
            return T == BOT or (T[0] == 'tuple' and any(hb(t) for t in T[1])) or (T[0] in ('option', 'list', 'vector') and hb(T[1]))
        return x.bot or hb(x.ty)

    def unify(self, f, T, x, tv):
        """Bind the callee's type variables from an argument's type (and value)."""
        if f.k == 'tapp' and f.name in ('int', 'atom', 'implicit') and len(f.args) == 1 and f.args[0].k == 'tvar':
            v = f.args[0].name
            if v not in tv and x is not None:
                tv[v] = self.nat(x)
            return
        if f.k == 'tapp' and f.name in ('bits', 'sbits', 'bitvector'):
            aw = T[1] if T[0] == 'bits' else 1 if T[0] == 'bit' else None
            n = f.args[0]
            if aw is None:
                return
            if n.k == 'tvar':
                tv.setdefault(n.name, aw)
            elif n.k == 'tbin' and n.op == '*':
                for c, v in ((n.a, n.b), (n.b, n.a)):
                    if c.k == 'tnum' and v.k == 'tvar' and v.name not in tv:
                        tv[v.name] = aw // c.n if isinstance(aw, int) else '(wquot %s #%d)' % (aw, c.n)
            return
        if f.k == 'tvar':
            if f.name not in tv and T != BOT:
                tv[f.name] = T
            return
        if f.k == 'ttuple' and T[0] == 'tuple':
            for g, t in zip(f.items, T[1]):
                self.unify(g, t, None, tv)
            return
        if f.k == 'tapp' and f.name in ('option', 'list') and T[0] == f.name:
            self.unify(f.args[0], T[1], None, tv)

    def wparams(self, g):
        """The callee's integer type variables that no int('n) parameter gives: explicit Word
        parameters, in the order of its forall."""
        tvs = self.vals[g][0]
        given = set()
        for f in self.formals_of(g):
            if f.k == 'tapp' and f.name in ('int', 'atom', 'implicit') and len(f.args) == 1 and f.args[0].k == 'tvar':
                given.add(f.args[0].name)
        return [v for v, k in tvs if k != 'Type' and v not in given]

    def call_user(self, g, e, env, exp, xs=None):
        tvs, typ, _ = self.vals[g]
        formals = self.formals_of(g)
        args = e.args if e.k == 'call' else [e.a, e.b]
        if len(args) == 1 and not formals and args[0].k == 'unit':
            args = []
        if len(args) != len(formals):
            self.err(e, '%s takes %d arguments, given %d' % (g, len(formals), len(args)))
        tv, out = {}, []
        for i, f in enumerate(formals):
            if xs is not None and not self.has_bot(xs[i]):
                x = xs[i]
            else:
                x = self.tr(args[i], env, self.closed_ty(f, tv))
            self.unify(f, x.ty, x, tv)
            out.append(x)
        bot = False
        if exp is not None:
            self.unify(typ.ret, exp, None, tv)
        for v, k in tvs:
            if v not in tv:
                if k == 'Type':
                    tv[v] = exp if exp is not None else UNIT
                    bot = exp is None
                elif v in self.wparams(g):
                    self.err(e, 'the width %s of %s is not determined at this call' % (v, g))
        retT = self.ty(typ.ret, tv)
        parts = ["self'" if g == env.get('rec') else self.gname(g)]
        parts += ['{%s}' % self.cty(tv[v]) for v, k in tvs if k == 'Type']
        parts += [w_code(tv[v]) for v in self.wparams(g)]
        parts += [x.code for x in out]
        pre = self.P(out)
        cls = self.cls[g]
        if cls == 'P':
            return X('(%s)' % ' '.join(parts) if len(parts) > 1 else parts[0], retT, pre, bot=bot)
        v = self.fresh()
        return X(v, retT, pre + [Bind(cls, v, retT, [' '.join(parts)])], bot=bot)

    def tr_call(self, e, env, exp, tail, nested):
        f = e.f
        if f in ('__read_mem', '__write_mem', '__write_mem_ea'):
            xs = [self.tr(a, env) for a in e.args[1:]]  # the first argument is Sail's access kind
            pre = self.P(xs)
            if f == '__write_mem_ea':  # announces a write; no state
                return X('unit', UNIT, pre)
            n = self.nat(xs[2])
            if f == '__read_mem':
                T = ('bits', w_mul(8, n))
                v = self.fresh('mem')
                return X(v, T, pre + [Bind('S', v, T, ['getS {%s} (x86-read-mem %s %s)' % (self.cty(T), w_code(n), xs[1].code)])])
            return X('true', BOOL, pre + [Bind('S', self.fresh('_'), UNIT, [
                'putS (x86-write-mem %s %s %s)' % (w_code(n), xs[1].code, xs[3].code)])])
        if f.startswith('Mk_') and f[3:] in self.bitfields:
            T = ('bitfield', f[3:])
            x = self.tr(e.args[0], env, ('bits', self.width(T)))
            return X(x.code, T, x.pre)
        if f == 'Some':
            x = self.tr(e.args[0], env, exp[1] if exp and exp[0] == 'option' else None)
            return X('(Some %s)' % x.code, ('option', x.ty), x.pre)
        if f == 'None':
            if exp and exp[0] == 'option' and self.closed(exp):
                return X('None', exp)
            return X('None', ('option', BOT), bot=True)
        if f in self.ctor:
            un, at = self.ctor[f]
            at = self.ty(at, {})
            if at == UNIT:
                return X(self.gname(f), ('union', un))
            x = self.tr(e.args[0], env, at)
            return X('(%s %s)' % (self.gname(f), x.code), ('union', un), x.pre)
        if f in env['loc']:
            self.err(e, 'a call of the local %s' % f)
        names = self.overloads.get(f)
        xs = None
        g = f
        if names:
            xs = self.infer_args(e.args, env)
            g = self.choose(names, xs, e)
        if g in self.funs:
            return self.call_user(g, e, env, exp, xs)
        if g in self.BI:
            if xs is None:
                xs = self.infer_args(e.args, env)
            return self.BI[g](self, xs, e, exp)
        self.err(e, 'unknown function %s' % g)

    def eq(self, a, b, e, env):
        ka, kb = shape(a.ty), shape(b.ty)
        pre = a.pre + b.pre
        bitsy = ('bits', 'bitfield')
        if ka in bitsy and kb in bitsy:
            return X('(weq %s %s)' % (a.code, b.code), BOOL, pre)
        if ka == kb == 'int':
            return X('(z-eq %s %s)' % (a.code, b.code), BOOL, pre)
        if ka == kb == 'bool':
            return X('(b-eq %s %s)' % (a.code, b.code), BOOL, pre)
        if ka == kb == 'string':
            return X('(str-eq %s %s)' % (a.code, b.code), BOOL, pre)
        if ka == kb == 'unit':
            return X('true', BOOL, pre)
        if ka == kb == 'enum' and a.ty == b.ty:
            n = self.gname(a.ty[1])
            return X('(weq (%s-ix %s) (%s-ix %s))' % (n, a.code, n, b.code), BOOL, pre)
        g = self.choose(self.overloads.get('==', []), [a, b], e)
        return self.call_user(g, e, env, BOOL, [a, b])

    def tr_binop(self, e, env, exp, tail, nested):
        op = e.op
        if op == '::':
            a = self.tr(e.a, env, exp[1] if exp and exp[0] == 'list' else None)
            b = self.tr(e.b, env, ('list', a.ty))
            return X('(cons %s %s)' % (a.code, b.code), ('list', a.ty), a.pre + b.pre)
        a, b = self.tr(e.a, env), self.tr(e.b, env)
        if op in ('==', '!='):
            x = self.eq(a, b, e, env)
            return x if op == '==' else X('(b-not %s)' % x.code, BOOL, x.pre)
        if op in ('&', '|') and a.ty == BOOL and b.cls != 'P':
            # Sail's and_bool / or_bool read their second argument only when it decides
            bb = X(b.code, BOOL, b.pre)
            arms = [(a.code, bb), (None, X('false', BOOL))] if op == '&' else [(a.code, X('true', BOOL)), (None, bb)]
            return self.cond_result(a.pre, arms, BOOL, False)
        g = self.choose(OPERATORS[op] + self.overloads.get(op, []), [a, b], e)
        if g in self.funs:
            return self.call_user(g, e, env, exp, [a, b])
        return self.BI[g](self, [a, b], e, exp)

    # ---- Sail's built-ins, as lib/languages/bits.cub's
    def W(self, x, e):
        return w_code(self.width(x.ty, e))

    def bi_int2(name, fold=None):
        def h(self, xs, e, exp):
            a, b = xs
            nat = None
            if fold and a.nat is not None and b.nat is not None and isinstance(a.nat, int) and isinstance(b.nat, int):
                r = fold(a.nat, b.nat)
                if r >= 0:
                    return X(z_lit(r), INT, self.P(xs), nat=r)
            elif fold and a.nat is not None and b.nat is not None:
                nat = {'z-add': w_add, 'z-mul': w_mul, 'z-sub': None}[name]
                nat = nat(a.nat, b.nat) if nat else None
            return X('(%s %s %s)' % (name, a.code, b.code), INT, self.P(xs), nat=nat)
        return h

    def bi_cmp(fmt):
        def h(self, xs, e, exp):
            return X(fmt % (xs[0].code, xs[1].code), BOOL, self.P(xs))
        return h

    def bi_cmp_swapped(name):
        def h(self, xs, e, exp):
            return X('(%s %s %s)' % (name, xs[1].code, xs[0].code), BOOL, self.P(xs))
        return h

    def bi_bv2(name):
        def h(self, xs, e, exp):
            return X('(%s %s %s %s)' % (name, self.W(xs[0], e), xs[0].code, xs[1].code), ('bits', self.width(xs[0].ty, e)), self.P(xs))
        return h

    def bi_shift(name):
        def h(self, xs, e, exp):
            return X('(%s %s %s %s)' % (name, self.W(xs[0], e), xs[0].code, w_code(self.nat(xs[1]))), ('bits', self.width(xs[0].ty, e)), self.P(xs))
        return h

    def bi_ext(name, vi, ni):
        def h(self, xs, e, exp):
            n = self.nat(xs[ni])
            return X('(%s %s %s %s)' % (name, self.W(xs[vi], e), w_code(n), xs[vi].code), ('bits', n), self.P(xs))
        return h

    def bi_simple(fmt, T):
        def h(self, xs, e, exp):
            return X(fmt % tuple(x.code for x in xs), T, self.P(xs))
        return h

    def bi_mask(self, xs, e, exp):
        n = self.nat(xs[0])
        return X('(bv-mask %s %s)' % (w_code(n), xs[1].code), ('bits', n), self.P(xs))

    def bi_zeros(code):
        def h(self, xs, e, exp):
            n = self.nat(xs[0])
            return X(code % w_code(n) if '%' in code else code, ('bits', n), self.P(xs))
        return h

    def bi_not_vec(self, xs, e, exp):
        return X('(bv-not %s %s)' % (self.W(xs[0], e), xs[0].code), ('bits', self.width(xs[0].ty, e)), self.P(xs))

    def bi_sub_bits_int(self, xs, e, exp):
        return X('(bv-add-int %s %s (z-negate %s))' % (self.W(xs[0], e), xs[0].code, xs[1].code), ('bits', self.width(xs[0].ty, e)), self.P(xs))

    def bi_append(self, xs, e, exp):
        wa, wb = self.width(xs[0].ty, e), self.width(xs[1].ty, e)
        return X('(bv-append %s %s %s %s)' % (w_code(wa), w_code(wb), xs[0].code, xs[1].code), ('bits', w_add(wa, wb)), self.P(xs))

    def bi_power(self, xs, e, exp):
        if xs[0].nat != 2:
            self.err(e, 'a power of something other than 2')
        return X('(z-pow2 %s)' % xs[1].code, INT, self.P(xs))

    def bi_unsigned(name):
        def h(self, xs, e, exp):
            return X('(%s %s %s)' % (name, self.W(xs[0], e), xs[0].code), INT, self.P(xs))
        return h

    def bi_get_slice_int(self, xs, e, exp):
        n = self.nat(xs[0])
        return X('(bv-of-int %s %s %s)' % (w_code(n), xs[1].code, w_code(self.nat(xs[2]))), ('bits', n), self.P(xs))

    def bi_set_slice_int(self, xs, e, exp):
        return X('(z-set-slice %s %s %s %s)' % (w_code(self.nat(xs[0])), xs[1].code, w_code(self.nat(xs[2])), xs[3].code), INT, self.P(xs))

    def bi_replicate(self, xs, e, exp):
        w, k = self.width(xs[0].ty, e), self.nat(xs[1])
        return X('(bv-replicate %s %s %s)' % (w_code(w), xs[0].code, w_code(k)), ('bits', w_mul(w, k)), self.P(xs))

    def bi_length(self, xs, e, exp):
        w = self.width(xs[0].ty, e)
        return X('(zpos %s)' % w_code(w), INT, self.P(xs), nat=w)

    def bi_is_some(self, xs, e, exp):
        return X('(is_some {%s} %s)' % (self.cty(xs[0].ty[1]), xs[0].code), BOOL, self.P(xs))

    def bi_print(self, xs, e, exp):
        # the emulator's console is not part of the machine state: a print is unit
        self.dropped_prints.add('%s:%d' % (e.file, e.line))
        return X('unit', UNIT, self.P(xs))

    def bi_bits_str(self, xs, e, exp):
        return X('(str-of-bits %s %s)' % (self.W(xs[0], e), xs[0].code), STRING, self.P(xs))

    BI = {
        'not_bool': bi_simple('(b-not %s)', BOOL), 'and_bool': bi_simple('(b-and %s %s)', BOOL),
        'or_bool': bi_simple('(b-or %s %s)', BOOL), 'negate': bi_simple('(z-negate %s)', INT),
        'add_atom': bi_int2('z-add', lambda a, b: a + b), 'sub_atom': bi_int2('z-sub', lambda a, b: a - b),
        'mult_atom': bi_int2('z-mul', lambda a, b: a * b),
        'lt_int': bi_cmp('(z-lt %s %s)'), 'lteq_int': bi_cmp('(z-le %s %s)'),
        'gt_int': bi_cmp_swapped('z-lt'), 'gteq_int': bi_cmp_swapped('z-le'),
        'add_bits': bi_bv2('bv-add'), 'sub_bits': bi_bv2('bv-sub'), 'add_bits_int': bi_bv2('bv-add-int'),
        'sub_bits_int': bi_sub_bits_int, 'and_vec': bi_bv2('bv-and'), 'or_vec': bi_bv2('bv-or'),
        'xor_vec': bi_bv2('bv-xor'), 'not_vec': bi_not_vec, 'append': bi_append, 'int_power': bi_power,
        'concat_str': bi_simple('(str-cat %s %s)', STRING),
        'unsigned': bi_unsigned('bv-unsigned'), 'signed': bi_unsigned('bv-signed'),
        'truncate': bi_ext('bv-trunc', 0, 1), 'sail_zero_extend': bi_ext('bv-zext', 0, 1),
        'sail_sign_extend': bi_ext('bv-sext', 0, 1), 'sail_mask': bi_mask,
        'sail_zeros': bi_zeros('#0'), 'sail_ones': bi_zeros('(bv-ones %s)'),
        'sail_shiftleft': bi_shift('bv-shl'), 'sail_shiftright': bi_shift('bv-shr'),
        'sail_arith_shiftright': bi_shift('bv-sar'),
        'get_slice_int': bi_get_slice_int, 'set_slice_int': bi_set_slice_int,
        'replicate_bits': bi_replicate, 'length': bi_length,
        'tdiv_int': bi_simple('(z-tdiv %s %s)', INT), 'tmod_int': bi_simple('(z-tmod %s %s)', INT),
        'shr_int': bi_simple('(z-shr %s %s)', INT), '_shr_int': bi_simple('(z-shr %s %s)', INT),
        'shl_int': bi_simple('(z-shl %s %s)', INT), '_shl_int': bi_simple('(z-shl %s %s)', INT),
        'pow2': bi_simple('(z-pow2 %s)', INT), 'is_some': bi_is_some,
        'print_endline': bi_print, 'print_int': bi_print, 'print_bits': bi_print, 'print': bi_print,
        'bits_str': bi_bits_str, 'dec_str': bi_simple('(str-of-int %s)', STRING),
    }

    # ---- definitions
    # Sail recurses freely; here a recursion is on a bound (archive/2026-11-11/PLAN-rev9.md, revision 9, 9 Nov 2026, sha256 4d012c3a10fd…, §6 “Phases 2–8: everything that will be built”, “Phase 2 — The ISA model, and the encoder leaves the trusted base”, item 1 “The Sail importer”). The bound of
    # each recursive function of the model: a Word from its parameters that exceeds the depth
    # of its calls, and why. Past the bound a function with an outcome aborts with
    # recursion-bound and a pure one returns its type's default (never reached if the bound
    # is right).
    BOUNDS = {
        'feature_flags_fn': ('(vec-len {Text} %(features)s)', 'the list shortens'),
        'in_list': ('(vec-len {Text} %(xs)s)', 'the list shortens'),
        'logcount_int': ('(wadd (z-mag %(x)s) #2)', 'one call takes a negative to non-negative, then the magnitude halves'),
        'logand_int': ('(wadd (z-mag %(i)s) #2)', "i's magnitude halves (floor) until i is 0 or -1"),
        'bitcount8': ('#9', 'the 8-bit argument shifts right by one'),
        'get_prefixes': ('(z-nat %(cnt)s)', 'cnt decreases by one and the call stops at 0'),
        'x86_run_halt': ('(z-nat %(maxSteps)s)', 'maxSteps decreases by one and the call stops at 0'),
    }

    def emit_function(self, name):
        clauses = self.funs[name]
        if name not in self.vals:
            self.err(clauses[0], 'function %s has no val' % name)
        tvs, typ, _ = self.vals[name]
        formals = self.formals_of(name)
        self.ntemp, self.wnames = 0, set()
        self.cur, self.nhelper, self.helpers = name, 0, []
        env = {'loc': {}, 'tv': {}, 'ret': None}
        rec = name in self.calls[name]
        if rec:
            if name not in self.BOUNDS:
                self.err(clauses[0], 'recursive function %s has no bound in BOUNDS' % name)
            env['rec'] = name
        # parameter names
        d = clauses[0]
        if len(clauses) == 1:
            pats = d.pat.items if d.pat.k == 'ptuple' else [d.pat]
            if not formals:
                pats = []
            if len(pats) != len(formals):
                self.err(d, 'function %s: %d parameters against %d in its val' % (name, len(pats), len(formals)))
            names = []
            for p in pats:
                while p.k == 'ptyped':
                    p = p.p
                if p.k == 'pid':
                    names.append(p.name)
                elif p.k == 'pwild':
                    names.append(None)
                else:
                    self.err(p, 'parameter pattern %s' % p.k)
        else:
            names = [None] * len(formals)
        for v, k in tvs:
            if k == 'Type':
                env['tv'][v] = ('tv', v)
        explicit = self.wparams(name)
        for v in explicit:
            env['tv'][v] = 'w_' + v[1:]
        cubs = []
        for i, f in enumerate(formals):
            if names[i] is None:
                cubs.append(self.fresh('a'))
            else:
                cubs.append(self.bind_local(env, names[i], INT))
            if f.k == 'tapp' and f.name in ('int', 'atom', 'implicit') and len(f.args) == 1 and f.args[0].k == 'tvar':
                env['tv'].setdefault(f.args[0].name, '(z-nat %s)' % cubs[i])
        ptypes = []
        for i, f in enumerate(formals):
            T = self.ty(f, env['tv'])
            if not self.closed(T) and T[0] != 'tv':
                self.err(d, 'parameter %d of %s has a type that is not determined' % (i, name))
            ptypes.append(T)
            if names[i] is not None:
                env['loc'][names[i]] = (T, cubs[i])
                self.note_width(T)
            if T[0] == 'bits' and isinstance(T[1], str):
                for j in range(i + 1, len(formals)):
                    if re.search(r'(?<![\w?\'-])%s(?![\w?\'-])' % re.escape(cubs[j]), T[1]):
                        self.err(d, 'parameter %d of %s has a width given by a later parameter' % (i, name))
        retT = self.ty(typ.ret, env['tv'])
        env['ret'] = retT
        cls = self.cls[name]
        if len(clauses) == 1:
            x = self.tr(d.body, env, retT, True, False)
        else:
            ids = [N('id', d, name="arg'%d" % i) for i in range(len(formals))]
            for i, T in enumerate(ptypes):
                env['loc']["arg'%d" % i] = (T, cubs[i])
            scrut = ids[0] if len(ids) == 1 else N('tuple', d, items=ids)
            m = N('match', d, e=scrut, cases=[(c.pat, c.body) for c in clauses])
            x = self.tr(m, env, retT, True, False)
        body = self.wrap(x, cls, retT)
        gn = self.gname(name)
        sig = ''
        for v, k in tvs:
            if k == 'Type':
                sig += '{%s : Type} → ' % self.cty(('tv', v))
        for v in explicit:
            sig += '(w_%s : Word) → ' % v[1:]
        for c, T in zip(cubs, ptypes):
            sig += '(%s : %s) → ' % (c, self.cty(T))
        sig += self.mty(cls, retT)
        targs = ['{%s}' % self.cty(('tv', v)) for v, k in tvs if k == 'Type']
        params = ['w_' + v[1:] for v in explicit] + cubs
        out = []
        out += self.helpers
        if not rec:
            out.append('%s : %s' % (gn, sig))
            out.append(' '.join([gn] + targs + params) + ' =')
            out.extend(ind(body, 2))
            return out
        if targs:
            self.err(d, 'a recursive function with a type parameter')
        measure, why = self.BOUNDS[name]
        measure = measure % {n: c for n, c in zip(names, cubs) if n}
        bottom = self.default(retT, d) if cls == 'P' else \
            ('out-abort recursion-bound' if cls == 'F' else 'abortS {%s} recursion-bound' % self.cty(retT))
        out.append('-- %s recurses in Sail; here on the bound %s (%s)' % (name, measure, why))
        out.append('%s-body : (%s) → %s' % (gn, sig, sig))
        out.append(' '.join(["%s-body" % gn, "self'"] + params) + ' =')
        out.extend(ind(body, 2))
        out.append('%s-fuel : Word → %s' % (gn, sig))
        out.append("%s-fuel fuel' = %%welimP (λ _ → %s) fuel' (λ %s → %s) (λ _ h' → %s-body h')" % (
            gn, sig, ' '.join(params), bottom, gn))
        out.append('%s : %s' % (gn, sig))
        out.append('%s = %s-fuel (wadd %s #1) %s' % (' '.join([gn] + params), gn, measure, ' '.join(params)))
        return out

    def emit_decl(self, d):
        k = d.k
        out = []
        if k == 'bitfield':
            n = self.gname(d.name)
            w = self.width(('bitfield', d.name))
            out.append('-- bitfield %s : bits(%d)' % (d.name, w))
            out += ['%s : Type' % n, '%s = Bits #%d' % (n, w)]
            for f, hi, lo in d.fields:
                fw = hi - lo + 1
                out += ['%s-%s : %s → Bits #%d' % (n, f, n, fw),
                        '%s-%s x = bv-slice #%d x #%d #%d' % (n, f, w, hi, lo),
                        '%s-set-%s : %s → Bits #%d → %s' % (n, f, n, fw, n),
                        '%s-set-%s x v = bv-update #%d x #%d #%d v' % (n, f, w, hi, lo)]
        elif k == 'enum':
            n = self.gname(d.name)
            out.append('data %s = %s' % (n, ' | '.join(self.gname(i) for i in d.items)))
            out.append('%s-ix : %s → Word' % (n, n))
            for i, it in enumerate(d.items):
                out.append('%s-ix %s = #%d' % (n, self.gname(it), i))
        elif k == 'union':
            n = self.gname(d.name)
            cs = []
            for c, t in d.cons:
                T = self.ty(t, {})
                cs.append(self.gname(c) if T == UNIT else '%s (a : %s)' % (self.gname(c), self.cty(T)))
            out.append('data %s = %s' % (n, ' | '.join(cs)))
            if d.name == 'exception':
                out += MONAD.strip('\n').split('\n')
        elif k == 'struct':
            n = self.gname(d.name)
            fs = [(f, self.cty(self.ty(t, {}))) for f, t in d.fields]
            out.append('data %s = %s-mk %s' % (n, n, ' '.join('(%s : %s)' % ft for ft in fs)))
            vs = ' '.join("f'%d" % i for i in range(len(fs)))
            for i, (f, t) in enumerate(fs):
                out += ['%s-%s : %s → %s' % (n, f, n, t), "%s-%s (%s-mk %s) = f'%d" % (n, f, n, vs, i)]
        elif k == 'type':
            out.append('-- type %s: written out where it is used' % d.name)
        elif k == 'register':
            out.append('-- register %s: the field x86-%s of X86 (state.cub)' % (d.name, d.name))
        return out

    def raw_cty(self, T):
        """A register's type without the model's own names (state.cub precedes them)."""
        if T[0] == 'bitfield':
            return 'Bits #%d' % self.width(T)
        if T[0] == 'vector':
            return 'List (%s)' % self.raw_cty(T[1])
        return self.cty(T)

    def emit_state(self):
        regs = [('mem', 'ByteMap')] + [(r, self.raw_cty(self.reg_type(r))) for r in self.registers]
        out = ['-- lib/languages/isa/x86/state.cub — generated by tools/sail2cub.py: the machine state.',
               '-- X86 is the registers the Sail model declares (every `register` of every file, in',
               '-- load order) and the memory. Loaded after lib/languages/bits.cub.',
               'data X86 = x86-state ' + ' '.join('(%s : %s)' % rt for rt in regs)]
        vs = ["r'%d" % i for i in range(len(regs))]
        for i, (r, t) in enumerate(regs):
            out += ['x86-%s : X86 → %s' % (r, t),
                    'x86-%s (x86-state %s) = %s' % (r, ' '.join(vs), vs[i]),
                    'x86-set-%s : %s → X86 → X86' % (r, t),
                    "x86-set-%s v' (x86-state %s) = x86-state %s" % (r, ' '.join(vs), ' '.join(vs[:i] + ["v'"] + vs[i + 1:]))]
        zeros = []
        for r, ast in self.registers.items():
            T = self.reg_type(r)
            if T[0] == 'vector':
                zeros.append('(' + 'cons %s (' % self.default(T[1]) * (ast.args[0].n - 1) + 'cons %s nil' % self.default(T[1]) + ')' * (ast.args[0].n - 1) + ')')
            else:
                zeros.append(self.default(T))
        out += ['-- every register zero (false), each vector at its declared length, nothing in memory',
                'x86-reset : X86',
                'x86-reset = x86-state mem-none ' + ' '.join(zeros)]
        out += ['-- Sail\'s __read_mem and __write_mem: n bytes at an address, lowest address lowest',
                'x86-read-mem : (n : Word) → Bits #64 → X86 → Bits (wmul #8 n)',
                'x86-read-mem n a s = mem-load-n (x86-mem s) n a',
                'x86-write-mem : (n : Word) → Bits #64 → Bits (wmul #8 n) → X86 → X86',
                'x86-write-mem n a v s = x86-set-mem (mem-store-n (x86-mem s) n a v) s']
        return out

    def generate(self, outdir):
        files = [f for f, _ in self.model.files]
        fidx = {f: i for i, f in enumerate(files)}
        # each function goes to the latest of its own file and the files of what it calls
        place, order, state = {}, [], {}

        def visit(f):
            if state.get(f) == 2:
                return
            if state.get(f) == 1:
                raise SailError('mutual recursion through %s' % f)
            state[f] = 1
            at = fidx[self.file_of[f]]
            for g in sorted(self.calls[f], key=lambda g: self.fun_order.index(g) if g in self.funs else -1):
                if g in self.funs and g != f:
                    visit(g)
                    at = max(at, place[g])
            place[f] = at
            state[f] = 2
            order.append(f)
        for f in self.fun_order:
            visit(f)
        self.dropped_prints = set()
        failures, counts, moved = [], {'P': 0, 'F': 0, 'S': 0}, []
        os.makedirs(outdir, exist_ok=True)
        with open(os.path.join(outdir, 'state.cub'), 'w') as o:
            o.write(with_reasons('\n'.join(self.emit_state()) + '\n'))
        names = ['state.cub']
        for i, (fname, defs) in enumerate(self.model.files):
            base = fname[:-5] + '.cub'
            out = ['-- lib/languages/isa/x86/%s — generated by tools/sail2cub.py from model/%s of' % (base, fname),
                   '-- rems-project/sail-x86-from-acl2 at %s. Do not edit: regenerate.' % SAIL_COMMIT]
            for d in defs:
                out += self.emit_decl(d)
            for f in order:
                if place[f] != i:
                    continue
                if self.file_of[f] != fname:
                    out.append('-- %s: from %s (it calls what is defined after it there)' % (f, self.file_of[f]))
                    moved.append((f, self.file_of[f], fname))
                try:
                    out += self.emit_function(f)
                    counts[self.cls[f]] += 1
                except SailError as e:
                    failures.append((f, str(e)))
                    out.append('-- NOT TRANSLATED %s: %s' % (f, e))
            text = '\n'.join(out) + '\n'
            exn = self.gname('exception')  # the importer writes the model's exception type by its Sail name
            if exn != 'exception':
                text = re.sub(r"(?<![\w?'-])exception(?![\w?'-])", exn, text)
            with open(os.path.join(outdir, base), 'w') as o:
                o.write(with_reasons(text))
            names.append(base)
        with open(os.path.join(outdir, 'all.cub'), 'w') as o:   # the model, imported in include order
            o.write('-- lib/languages/isa/x86/all.cub — the files of the Sail x86 model, in the order the model includes them,\n'
                    '-- written by tools/sail2cub.py with them. Importing it loads the model.\n'
                    'import "../../bits.cub"\n' + ''.join('import "%s"\n' % n for n in names))
        print('functions: %d translated (%d pure, %d outcome, %d state), %d not translated' % (
            sum(counts.values()), counts['P'], counts['F'], counts['S'], len(failures)))
        print('moved to a later file: %d' % len(moved))
        print('undefined: %s' % ', '.join(self.undefined_uses))
        print('prints dropped: %d' % len(self.dropped_prints))
        seen = {}
        for f, e in failures:
            key = re.sub(r'^[^ ]+ ', '', e)
            seen.setdefault(key, []).append(f)
        for key, fs in sorted(seen.items(), key=lambda kv: -len(kv[1])):
            print('  %4d  %s   e.g. %s' % (len(fs), key[:150], [f for f in fs[:3]]))
        return len(failures)


SAIL_COMMIT = '0c6c023dccc7c20d8d9dbfc481f01d9bb49d62bf'

# what follows the model's `union exception`: the two effect types and their operations
MONAD = '''
-- the outcome of a function that asserts or throws, and of one that also reads or writes X86
MF : Type → Type
MF A = Outc exception A
MS : Type → Type
MS A = State X86 exception A
retF : {A : Type} → A → MF A
retF a = out-val a
bindF : {A B : Type} → MF A → (A → MF B) → MF B
bindF m k = fl-bind m k
tryF : {A : Type} → MF A → (exception → MF A) → MF A
tryF m h = fl-try m h
retS : {A : Type} → A → MS A
retS a s = (out-val a , s)
liftS : {A : Type} → MF A → MS A
liftS r s = (r , s)
bindS : {A B : Type} → MS A → (A → MS B) → MS B
bindS m k s = st-step k (m s)
onS : {A B : Type} → MF A → (A → MS B) → MS B
onS r k s = st-on r k s
getS : {A : Type} → (X86 → A) → MS A
getS f s = (out-val (f s) , s)
putS : (X86 → X86) → MS Unit
putS f s = (out-val unit , f s)
abortS : {A : Type} → Text → MS A
abortS w s = (out-abort w , s)
tryS : {A : Type} → MS A → (exception → MS A) → MS A
tryS m h s = st-catch h (m s)
foreachS : {A : Type} → (lo hi : ZI) → A → (ZI → A → MS A) → MS A
foreachS lo hi a f = st-foreach lo hi a f
'''


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


def reserved_names(libdir):
    """Every identifier the library's files mention: a Sail global of the same name is renamed."""
    names = set()
    for f in sorted(os.listdir(libdir)):
        if f.endswith('.cub'):
            text = re.sub(r'--[^\n]*', '', open(os.path.join(libdir, f)).read())
            names.update(re.findall(r"[A-Za-z_][A-Za-z0-9_?'-]*", text))
    return names


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
        if argv[2] == '--out':
            here = os.path.dirname(os.path.abspath(__file__))
            tr = Translator(model, reserved_names(os.path.join(here, '..', 'lib')))
            return 1 if tr.generate(argv[3]) else 0
    except SailError as e:
        print('sail2cub: %s' % e, file=sys.stderr)
        return 1
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))

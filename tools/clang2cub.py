#!/usr/bin/env python3
"""clang2cub — a C++ translation unit, after Clang's semantic analysis, as a checked value.

    python3 tools/clang2cub.py unit.cpp -o unit.cub      the unit as `<unit>-tu : CTU`
    python3 tools/clang2cub.py --emit-ast > lib/cxx/ast.cub   the AST type itself

Sources (Clang 18; no Clang headers or bindings are used):
  1. clang++ <flags> -fsyntax-only -Xclang -ast-dump=json -Xclang -fdump-record-layouts
     one process: the record layouts Sema computed, then the AST as JSON.
  2. clang++ <flags> -emit-llvm -S -o /dev/null -Xclang -fdump-vtable-layouts
     the vtables of the unit (only code generation lays them out).
  3. clang++ <flags> -fsyntax-only -Xclang -ast-dump
     the AST as text, for the one thing the JSON omits and the text has: whether a member is
     named with a qualifier (`p->Base::f()`, a call that is not virtual), and the type of an
     `alignas(type)`. The two dumps list the same nodes in the same order; the k-th
     MemberExpr (AlignedAttr) of one is the k-th of the other, and the import stops unless
     their counts, and the arrows and member names of the MemberExprs, all agree.

What is written is the AST after Sema: names are declaration ids, overloads are chosen,
templates are their instantiations, implicit casts and constructor calls are nodes, constant
expressions carry their values. Only declarations reachable from the unit's roots are written.
Types are structured; a type string the parser cannot read, a name it cannot resolve, or a node
kind without a rule stops the import with a message naming it. There is no "unknown" node.

The AST type (SCHEMA below) is the single description of the constructors: `--emit-ast` prints
it as lib/cxx/ast.cub, and every term written is built against it.
"""
import json
import os
import re
import struct
import subprocess
import sys

CLANG_FLAGS = ['-std=c++20', '-fno-exceptions', '-fno-rtti']


class Stop(Exception):
    pass


class Dependent(Exception):
    """A name that is a template parameter, or a type that only exists uninstantiated."""


class NeedsProbe(Stop):
    """A type string with a part only Clang can evaluate (decltype of an expression, noexcept
    of an expression): the type is asked of Clang by a probe, where there is a way to ask."""


class NotAType(Stop):
    """A name in a type string that no type declaration of the unit has."""


def stop(msg):
    raise Stop(msg)


# ============================================================================ the AST type
# (type, [(constructor, [(field, field type)])]) in dependency order. A field type is a schema
# type, `Word`, `Bool`, `CStr`, or `List T` for T declared earlier. The checker has no mutual
# data and no `List D` inside `data D` (archive/2026-11-11/C-clang.md, Found), so CType, CExpr and CStmt
# carry their own list constructors (`ct-nil`/`ct-cons`, `ce-nil`/`ce-cons`, `cs-nil`/`cs-cons`):
# a field holding "a list of expressions" has type CExpr and is built from those two.
INLIST = {'CType': 'ct', 'CExpr': 'ce', 'CStmt': 'cs'}

SCHEMA = [
    ('CInt', 'an integer value: its magnitude and sign (the type beside it gives the width)', [
        ('cint-pos', [('n', 'Word')]),
        ('cint-neg', [('n', 'Word')]),
    ]),
    ('COptId', 'a declaration id that may be absent', [
        ('coi-none', []),
        ('coi-some', [('id', 'Word')]),
    ]),
    ('CBuiltin', "Clang's BuiltinType kinds met", [
        ('ctb-void', []), ('ctb-bool', []), ('ctb-char', []), ('ctb-schar', []), ('ctb-uchar', []),
        ('ctb-short', []), ('ctb-ushort', []), ('ctb-int', []), ('ctb-uint', []), ('ctb-long', []),
        ('ctb-ulong', []), ('ctb-llong', []), ('ctb-ullong', []), ('ctb-int128', []),
        ('ctb-uint128', []), ('ctb-float', []), ('ctb-double', []), ('ctb-ldouble', []),
        ('ctb-wchar', []), ('ctb-char8', []), ('ctb-char16', []), ('ctb-char32', []),
        ('ctb-nullptr', []),
        ('ctb-bound-member', []), ('ctb-builtin-fn', []),
    ]),
    ('CRefQual', 'the ref-qualifier of a function type', [
        ('crq-none', []), ('crq-lvalue', []), ('crq-rvalue', []),
    ]),
    ('CType', 'a type, structured; ct-nil/ct-cons build the parameter list of ct-function', [
        ('ct-builtin', [('b', 'CBuiltin')]),
        ('ct-pointer', [('t', 'CType')]),
        ('ct-lref', [('t', 'CType')]),
        ('ct-rref', [('t', 'CType')]),
        ('ct-array', [('t', 'CType'), ('n', 'Word')]),
        ('ct-record', [('id', 'Word')]),
        ('ct-enum', [('id', 'Word')]),
        ('ct-function', [('ret', 'CType'), ('params', 'CType'), ('variadic', 'Bool'),
                         ('const', 'Bool'), ('volatile', 'Bool'), ('ref', 'CRefQual'),
                         ('noexcept', 'Bool')]),
        ('ct-member-pointer', [('class', 'Word'), ('t', 'CType')]),
        ('ct-qual', [('const', 'Bool'), ('volatile', 'Bool'), ('restrict', 'Bool'), ('t', 'CType')]),
        ('ct-nil', []),
        ('ct-cons', [('x', 'CType'), ('xs', 'CType')]),
    ]),
    ('CValCat', 'value categories', [
        ('vc-prvalue', []), ('vc-lvalue', []), ('vc-xvalue', []),
    ]),
    ('CCastKind', "Clang's cast kinds met", [
        ('ck-LValueToRValue', []), ('ck-NoOp', []), ('ck-FunctionToPointerDecay', []),
        ('ck-ArrayToPointerDecay', []), ('ck-IntegralCast', []), ('ck-IntegralToBoolean', []),
        ('ck-BitCast', []), ('ck-DerivedToBase', []), ('ck-UncheckedDerivedToBase', []),
        ('ck-NullToPointer', []), ('ck-ToVoid', []), ('ck-BuiltinFnToFnPtr', []),
        ('ck-ConstructorConversion', []), ('ck-PointerToBoolean', []),
        ('ck-NullToMemberPointer', []), ('ck-UserDefinedConversion', []),
        ('ck-IntegralToFloating', []), ('ck-FloatingToIntegral', []), ('ck-FloatingCast', []), ('ck-BaseToDerived', []),
        ('ck-Dynamic', []),
    ]),
    ('CUnOp', 'unary operators met', [
        ('uo-post-inc', []), ('uo-post-dec', []), ('uo-pre-inc', []), ('uo-pre-dec', []),
        ('uo-addr-of', []), ('uo-deref', []), ('uo-plus', []), ('uo-minus', []), ('uo-not', []),
        ('uo-lnot', []),
    ]),
    ('CBinOp', 'binary operators met (a compound assignment names the operator applied)', [
        ('cbo-mul', []), ('cbo-div', []), ('cbo-rem', []), ('cbo-add', []), ('cbo-sub', []),
        ('cbo-shl', []), ('cbo-shr', []), ('cbo-lt', []), ('cbo-gt', []), ('cbo-le', []), ('cbo-ge', []),
        ('cbo-eq', []), ('cbo-ne', []), ('cbo-and', []), ('cbo-xor', []), ('cbo-or', []),
        ('cbo-land', []), ('cbo-lor', []), ('cbo-assign', []), ('cbo-comma', []),
        ('cbo-ptr-mem-d', []), ('cbo-ptr-mem-i', []),
    ]),
    ('CRefKind', 'the kind of declaration a reference names', [
        ('crk-function', []), ('crk-method', []), ('crk-var', []), ('crk-parm', []),
        ('crk-enum-constant', []), ('crk-field', []),
    ]),
    ('CConstructKind', 'what a constructor call constructs', [
        ('cck-complete', []), ('cck-non-virtual-base', []), ('cck-delegating', []),
        ('cck-virtual-base', []),
    ]),
    ('CStorageDuration', 'the storage duration of a materialized temporary', [
        ('csd-full-expression', []), ('csd-automatic', []), ('csd-static', []),
    ]),
    ('CTraitKind', 'sizeof, alignof, and __alignof (the preferred alignment)', [
        ('ctk-sizeof', []), ('ctk-alignof', []), ('ctk-preferred-alignof', []),
    ]),
    ('CAtomicOp', "the operation of an AtomicExpr, by Clang's builtin name", [
        ('cao-atomic-load', []), ('cao-atomic-load-n', []), ('cao-atomic-store-n', []),
        ('cao-atomic-exchange-n', []), ('cao-atomic-compare-exchange-n', []),
        ('cao-atomic-fetch-add', []), ('cao-atomic-fetch-sub', []),
    ]),
    ('CNewInit', 'the initializer style of a new-expression', [
        ('cni-none', []), ('cni-parens', []), ('cni-braces', []),
    ]),
    ('CExpr', 'an expression; every node has its type and value category. One constructor '
              'per Clang Expr kind met; ce-nil/ce-cons build argument and initializer lists', [
        ('ce-integer-literal', [('t', 'CType'), ('vc', 'CValCat'), ('v', 'Word')]),
        ('ce-character-literal', [('t', 'CType'), ('vc', 'CValCat'), ('v', 'Word')]),
        ('ce-bool-literal', [('t', 'CType'), ('vc', 'CValCat'), ('v', 'Bool')]),
        ('ce-floating-literal', [('t', 'CType'), ('vc', 'CValCat'), ('bits', 'Word')]),
        ('ce-string-literal', [('t', 'CType'), ('vc', 'CValCat'), ('s', 'CStr')]),
        ('ce-string-literal-units', [('t', 'CType'), ('vc', 'CValCat'), ('units', 'List Word')]),
        ('ce-nullptr-literal', [('t', 'CType'), ('vc', 'CValCat')]),
        ('ce-declref', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CRefKind'), ('id', 'Word')]),
        ('ce-member', [('t', 'CType'), ('vc', 'CValCat'), ('base', 'CExpr'), ('arrow', 'Bool'),
                       ('qualified', 'Bool'), ('k', 'CRefKind'), ('id', 'Word')]),
        ('ce-call', [('t', 'CType'), ('vc', 'CValCat'), ('callee', 'CExpr'), ('args', 'CExpr')]),
        ('ce-member-call', [('t', 'CType'), ('vc', 'CValCat'), ('callee', 'CExpr'),
                            ('args', 'CExpr')]),
        ('ce-operator-call', [('t', 'CType'), ('vc', 'CValCat'), ('callee', 'CExpr'),
                              ('args', 'CExpr')]),
        ('ce-construct', [('t', 'CType'), ('vc', 'CValCat'), ('ctor', 'Word'),
                          ('k', 'CConstructKind'), ('elidable', 'Bool'), ('list', 'Bool'),
                          ('zeroing', 'Bool'), ('args', 'CExpr')]),
        ('ce-temporary-object', [('t', 'CType'), ('vc', 'CValCat'), ('ctor', 'Word'),
                                 ('k', 'CConstructKind'), ('elidable', 'Bool'), ('list', 'Bool'),
                                 ('zeroing', 'Bool'), ('args', 'CExpr')]),
        ('ce-implicit-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                              ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-static-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                            ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-reinterpret-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                                 ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-functional-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                                ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-cstyle-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                            ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-unary', [('t', 'CType'), ('vc', 'CValCat'), ('op', 'CUnOp'), ('e', 'CExpr')]),
        ('ce-binary', [('t', 'CType'), ('vc', 'CValCat'), ('op', 'CBinOp'), ('l', 'CExpr'),
                       ('r', 'CExpr')]),
        ('ce-compound-assign', [('t', 'CType'), ('vc', 'CValCat'), ('op', 'CBinOp'),
                                ('lhs-type', 'CType'), ('result-type', 'CType'), ('l', 'CExpr'),
                                ('r', 'CExpr')]),
        ('ce-conditional', [('t', 'CType'), ('vc', 'CValCat'), ('c', 'CExpr'), ('a', 'CExpr'),
                            ('b', 'CExpr')]),
        ('ce-paren', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-array-subscript', [('t', 'CType'), ('vc', 'CValCat'), ('base', 'CExpr'),
                                ('index', 'CExpr')]),
        ('ce-init-list', [('t', 'CType'), ('vc', 'CValCat'), ('union-field', 'COptId'),
                          ('filler', 'CExpr'), ('inits', 'CExpr')]),
        ('ce-implicit-value-init', [('t', 'CType'), ('vc', 'CValCat')]),
        ('ce-default-init', [('t', 'CType'), ('vc', 'CValCat'), ('field', 'Word')]),
        ('ce-default-arg', [('t', 'CType'), ('vc', 'CValCat'), ('parm', 'Word')]),
        ('ce-this', [('t', 'CType'), ('vc', 'CValCat')]),
        ('ce-materialize-temporary', [('t', 'CType'), ('vc', 'CValCat'),
                                      ('sd', 'CStorageDuration'), ('extending', 'COptId'),
                                      ('e', 'CExpr')]),
        ('ce-with-cleanups', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-constant', [('t', 'CType'), ('vc', 'CValCat'), ('v', 'CInt'), ('e', 'CExpr')]),
        ('ce-lambda', [('t', 'CType'), ('vc', 'CValCat'), ('closure', 'Word'),
                       ('captures', 'CExpr')]),
        ('ce-unary-or-type-trait', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CTraitKind'),
                                    ('arg-type', 'CType'), ('arg', 'CExpr')]),
        ('ce-subst-nttp', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-rewritten-binary', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-coawait', [('t', 'CType'), ('vc', 'CValCat'), ('operand', 'CExpr'), ('common', 'CExpr'),
                        ('ready', 'CExpr'), ('suspend', 'CExpr'), ('resume', 'CExpr')]),
        ('ce-coyield', [('t', 'CType'), ('vc', 'CValCat'), ('operand', 'CExpr'), ('common', 'CExpr'),
                        ('ready', 'CExpr'), ('suspend', 'CExpr'), ('resume', 'CExpr')]),
        ('ce-opaque-value', [('t', 'CType'), ('vc', 'CValCat'), ('id', 'Word'), ('source', 'CExpr')]),
        ('ce-paren-list-init', [('t', 'CType'), ('vc', 'CValCat'), ('inits', 'CExpr')]),
        ('ce-throw', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-bind-temporary', [('t', 'CType'), ('vc', 'CValCat'), ('dtor', 'Word'), ('e', 'CExpr')]),
        ('ce-dynamic-cast', [('t', 'CType'), ('vc', 'CValCat'), ('k', 'CCastKind'),
                             ('path', 'List Word'), ('e', 'CExpr')]),
        ('ce-typeid', [('t', 'CType'), ('vc', 'CValCat'), ('arg-type', 'CType'), ('arg', 'CExpr')]),
        ('ce-noexcept', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-atomic', [('t', 'CType'), ('vc', 'CValCat'), ('op', 'CAtomicOp'), ('args', 'CExpr')]),
        ('ce-std-initializer-list', [('t', 'CType'), ('vc', 'CValCat'), ('e', 'CExpr')]),
        ('ce-new', [('t', 'CType'), ('vc', 'CValCat'), ('global', 'Bool'), ('array', 'Bool'),
                    ('operator-new', 'COptId'), ('init-style', 'CNewInit'),
                    ('array-size', 'CExpr'), ('placement', 'CExpr'), ('init', 'CExpr')]),
        ('ce-delete', [('t', 'CType'), ('vc', 'CValCat'), ('global', 'Bool'), ('array', 'Bool'),
                       ('operator-delete', 'COptId'), ('e', 'CExpr')]),
        ('ce-nil', []),
        ('ce-cons', [('x', 'CExpr'), ('xs', 'CExpr')]),
    ]),
    ('CStorage', 'storage class as written', [
        ('cst-none', []), ('cst-static', []), ('cst-extern', []),
        ('cst-thread-local', []), ('cst-static-thread-local', []), ('cst-extern-thread-local', []),
    ]),
    ('CVarInit', 'the initializer of a variable, by its style', [
        ('cvi-none', []),
        ('cvi-c', [('e', 'CExpr')]),
        ('cvi-call', [('e', 'CExpr')]),
        ('cvi-list', [('e', 'CExpr')]),
    ]),
    ('CAlign', 'an alignment specifier on a variable: a number of bytes, or as of a type', [
        ('cal-none', []),
        ('cal-bytes', [('n', 'Word')]),
        ('cal-type', [('t', 'CType')]),
    ]),
    ('CVar', 'VarDecl and ParmVarDecl (with its default argument, ce-nil when it has none)', [
        ('cv-var', [('id', 'Word'), ('name', 'CStr'), ('t', 'CType'), ('storage', 'CStorage'),
                    ('align', 'CAlign'), ('constexpr', 'Bool'), ('nrvo', 'Bool'),
                    ('init', 'CVarInit')]),
        ('cv-parm', [('id', 'Word'), ('name', 'CStr'), ('t', 'CType'), ('default', 'CExpr')]),
    ]),
    ('CStmt', 'a statement. One constructor per Clang Stmt kind met; cs-expr is an expression '
              'in statement position; cs-none an absent child; cs-nil/cs-cons the children of '
              'cs-compound and the handlers of cs-try (each a cs-catch, whose var is empty for '
              'catch (...))', [
        ('cs-compound', [('ss', 'CStmt')]),
        ('cs-decl', [('vars', 'List CVar')]),
        ('cs-expr', [('e', 'CExpr')]),
        ('cs-if', [('init', 'CStmt'), ('cond-var', 'CStmt'), ('cond', 'CExpr'), ('then-s', 'CStmt'),
                   ('else-s', 'CStmt')]),
        ('cs-if-constexpr', [('init', 'CStmt'), ('cond-var', 'CStmt'), ('cond', 'CExpr'),
                             ('then-s', 'CStmt'), ('else-s', 'CStmt')]),
        ('cs-while', [('cond-var', 'CStmt'), ('cond', 'CExpr'), ('body', 'CStmt')]),
        ('cs-do', [('body', 'CStmt'), ('cond', 'CExpr')]),
        ('cs-for', [('init', 'CStmt'), ('cond-var', 'CStmt'), ('cond', 'CExpr'), ('inc', 'CExpr'),
                    ('body', 'CStmt')]),
        ('cs-for-range', [('init', 'CStmt'), ('range', 'CStmt'), ('begin', 'CStmt'),
                          ('end', 'CStmt'), ('cond', 'CExpr'), ('inc', 'CExpr'),
                          ('loop-var', 'CStmt'), ('body', 'CStmt')]),
        ('cs-return', [('e', 'CExpr')]),
        ('cs-break', []),
        ('cs-continue', []),
        ('cs-switch', [('init', 'CStmt'), ('cond-var', 'CStmt'), ('cond', 'CExpr'),
                       ('body', 'CStmt')]),
        ('cs-case', [('value', 'CExpr'), ('s', 'CStmt')]),
        ('cs-default', [('s', 'CStmt')]),
        ('cs-coroutine-body', [('body', 'CStmt'), ('promise', 'CStmt'), ('init-suspend', 'CStmt'),
                               ('final-suspend', 'CStmt'), ('on-exception', 'CStmt'),
                               ('on-fallthrough', 'CStmt'), ('allocate', 'CStmt'),
                               ('deallocate', 'CStmt'), ('result-decl', 'CStmt'),
                               ('return-value', 'CStmt'), ('return-stmt', 'CStmt'),
                               ('return-on-alloc-failure', 'CStmt'), ('param-moves', 'CStmt')]),
        ('cs-coreturn', [('operand', 'CExpr'), ('promise-call', 'CExpr')]),
        ('cs-try', [('body', 'CStmt'), ('handlers', 'CStmt')]),
        ('cs-catch', [('var', 'List CVar'), ('body', 'CStmt')]),
        ('cs-goto', [('label', 'Word')]),
        ('cs-label', [('id', 'Word'), ('name', 'CStr'), ('s', 'CStmt')]),
        ('cs-null', []),
        ('cs-none', []),
        ('cs-nil', []),
        ('cs-cons', [('x', 'CStmt'), ('xs', 'CStmt')]),
    ]),
    ('CTag', 'struct, class, union', [
        ('ctg-struct', []), ('ctg-class', []), ('ctg-union', []),
    ]),
    ('CBitWidth', 'a bit-field width', [
        ('cbw-none', []),
        ('cbw-bits', [('n', 'Word')]),
    ]),
    ('CCapture', "what a field of a lambda's closure class holds: inside the lambda's body a "
                 'reference to that variable of the enclosing function, or `this`, is this field; '
                 'ccap-init is an init-capture, whose variable (the id) exists only as this field', [
        ('ccap-none', []),
        ('ccap-var', [('id', 'Word')]),
        ('ccap-init', [('id', 'Word')]),
        ('ccap-this', []),
    ]),
    ('CField', 'FieldDecl: offset in bits from the record layout; the default member '
               'initializer (ce-nil when absent)', [
        ('cfield', [('id', 'Word'), ('name', 'CStr'), ('t', 'CType'), ('offset-bits', 'Word'),
                    ('width', 'CBitWidth'), ('mutable', 'Bool'), ('capture', 'CCapture'),
                    ('init', 'CExpr')]),
    ]),
    ('CBase', 'a direct base class with its offset in bytes from the record layout', [
        ('cbase', [('base', 'Word'), ('offset', 'Word'), ('primary', 'Bool'),
                   ('virtual', 'Bool')]),
    ]),
    ('CVPtr', "the vtable pointer's offset in bytes, when the record has its own", [
        ('cvp-none', []),
        ('cvp-at', [('offset', 'Word')]),
    ]),
    ('COptInt', 'an integer that may be absent', [
        ('coin-none', []),
        ('coin-some', [('v', 'CInt')]),
    ]),
    ('CVTEntry', 'one vtable slot, as the target ABI lays it out. cvt-thunk is a slot that '
                 'holds a thunk to the function of the entry inside: `this` adjusted by this-nv '
                 'bytes and then, when this-vcall is there, by the vcall offset stored that many '
                 'bytes from the vtable address; the result adjusted by ret-nv and, when '
                 'ret-vbase is there, by the virtual base offset stored that many bytes from the '
                 "result's vtable address", [
        ('cvt-offset-to-top', [('v', 'CInt')]),
        ('cvt-rtti', []),
        ('cvt-method', [('id', 'Word')]),
        ('cvt-pure', [('id', 'Word')]),
        ('cvt-complete-dtor', [('id', 'Word')]),
        ('cvt-deleting-dtor', [('id', 'Word')]),
        ('cvt-vbase-offset', [('v', 'CInt')]),
        ('cvt-vcall-offset', [('v', 'CInt')]),
        ('cvt-thunk', [('e', 'CVTEntry'), ('this-nv', 'CInt'), ('this-vcall', 'COptInt'),
                       ('ret-nv', 'CInt'), ('ret-vbase', 'COptInt')]),
    ]),
    ('CVAddrPoint', 'an address point of a vtable: the vtable pointer of the base subobject '
                    '`base` at `offset` bytes addresses the slot `index`', [
        ('cvap', [('base', 'Word'), ('offset', 'Word'), ('index', 'Word')]),
    ]),
    ('CVBaseOffsetOffset', 'where a vtable holds the offset of a virtual base: `at` bytes from '
                           'the address point', [
        ('cvboo', [('vbase', 'Word'), ('at', 'CInt')]),
    ]),
    ('CCtorVTable', 'a construction vtable: the vtable of the base subobject `base` at `offset` '
                    'while the complete object is under construction', [
        ('cctor-vtable', [('base', 'Word'), ('offset', 'Word'), ('address-points', 'List CVAddrPoint'),
                          ('entries', 'List CVTEntry')]),
    ]),
    ('CVTTEntry', "an entry of the VTT: a slot index in the class's own vtable, or in the "
                  'construction vtable of the base at that offset', [
        ('cvtt-own', [('index', 'Word')]),
        ('cvtt-construction', [('base', 'Word'), ('offset', 'Word'), ('index', 'Word')]),
    ]),
    ('CVTable', "a record's vtable. cvtable: the slots and the one index the object's vtable "
                'pointer addresses, for a vtable this unit emits that has one address point and '
                'no thunks. cvtable-full: every other vtable: whether this unit emits it (false: '
                'its key function is defined in another unit, or nothing here refers to it), its '
                'address points, slots, virtual base offset offsets, VTT, construction vtables; '
                'with no slots at all: a polymorphic class Clang laid out no vtable for in this '
                'unit (nothing here constructs it or calls a virtual function of it)', [
        ('cvt-none', []),
        ('cvtable', [('address-point', 'Word'), ('entries', 'List CVTEntry')]),
        ('cvtable-full', [('emitted', 'Bool'), ('address-points', 'List CVAddrPoint'),
                          ('entries', 'List CVTEntry'), ('vbase-offsets', 'List CVBaseOffsetOffset'),
                          ('vtt', 'List CVTTEntry'), ('construction', 'List CCtorVTable')]),
    ]),
    ('CVBase', 'a virtual base (direct or not) with its offset in bytes in an object whose '
               'most derived class is this one', [
        ('cvbase', [('base', 'Word'), ('offset', 'Word')]),
    ]),
    ('CLayout', "the record's layout on the target, in bytes: the implementation parameters of "
                'the plan (archive/2026-11-11/PLAN-rev9.md, revision 9, 9 Nov 2026, sha256 4d012c3a10fd…, §5 “The distinctions everything above x86 rests on”, item 1 “The abstract machine is written; implementations are lifted”). clayout-v is the layout of a class with virtual bases', [
        ('clayout', [('size', 'Word'), ('align', 'Word'), ('dsize', 'Word'), ('nvsize', 'Word'),
                     ('nvalign', 'Word'), ('vptr', 'CVPtr')]),
        ('clayout-v', [('size', 'Word'), ('align', 'Word'), ('dsize', 'Word'), ('nvsize', 'Word'),
                       ('nvalign', 'Word'), ('vptr', 'CVPtr'), ('vbases', 'List CVBase')]),
    ]),
    ('CRecord', 'the definition of a class', [
        ('crecord', [('id', 'Word'), ('name', 'CStr'), ('tag', 'CTag'), ('lambda', 'Bool'),
                     ('bases', 'List CBase'), ('fields', 'List CField'), ('layout', 'CLayout'),
                     ('vtable', 'CVTable')]),
    ]),
    ('CTArg', 'a template argument of a specialization', [
        ('cta-type', [('t', 'CType')]),
        ('cta-integral', [('v', 'CInt')]),
    ]),
    ('CEnumerator', 'EnumConstantDecl with its value', [
        ('cenumerator', [('id', 'Word'), ('name', 'CStr'), ('v', 'CInt')]),
    ]),
    ('CBody', 'the body of a function', [
        ('cfb-none', []),
        ('cfb-defaulted', []),
        ('cfb-pure', []),
        ('cfb-stmt', [('s', 'CStmt')]),
    ]),
    ('CCtorInit', 'CXXCtorInitializer, in the order of initialization', [
        ('cci-field', [('field', 'Word'), ('e', 'CExpr')]),
        ('cci-base', [('t', 'CType'), ('e', 'CExpr')]),
        ('cci-delegating', [('e', 'CExpr')]),
    ]),
    ('CFunction', 'what every function declaration has', [
        ('cfunction', [('id', 'Word'), ('name', 'CStr'), ('mangled', 'CStr'), ('t', 'CType'),
                       ('params', 'List CVar'), ('storage', 'CStorage'), ('inline', 'Bool'),
                       ('constexpr', 'Bool'), ('implicit', 'Bool'), ('body', 'CBody')]),
    ]),
    ('CDecl', 'a declaration of the unit. One constructor per Clang Decl kind met at the top '
              'level (members are listed flat, with the id of their class). cd-var-dynamic is a '
              "variable of static storage that the unit's start-up code initializes (Clang did "
              "not make its initializer a constant), in the order of the unit's `statics`", [
        ('cd-function', [('f', 'CFunction')]),
        ('cd-method', [('parent', 'Word'), ('virtual', 'Bool'), ('f', 'CFunction')]),
        ('cd-constructor', [('parent', 'Word'), ('inits', 'List CCtorInit'), ('f', 'CFunction')]),
        ('cd-destructor', [('parent', 'Word'), ('virtual', 'Bool'), ('f', 'CFunction')]),
        ('cd-conversion', [('parent', 'Word'), ('virtual', 'Bool'), ('f', 'CFunction')]),
        ('cd-record', [('r', 'CRecord')]),
        ('cd-class-template-specialization', [('args', 'List CTArg'), ('r', 'CRecord')]),
        ('cd-enum', [('id', 'Word'), ('name', 'CStr'), ('underlying', 'CType'),
                     ('scoped', 'Bool'), ('enumerators', 'List CEnumerator')]),
        ('cd-var', [('mangled', 'CStr'), ('v', 'CVar')]),
        ('cd-var-dynamic', [('mangled', 'CStr'), ('v', 'CVar')]),
        ('cd-var-template-specialization', [('args', 'List CTArg'), ('mangled', 'CStr'), ('v', 'CVar')]),
    ]),
    ('CTU', 'the translation unit: its name, the target, the ids of its roots, the ids of its '
            'variables of static storage in the order of their definitions, its declarations', [
        ('ctu', [('name', 'CStr'), ('target', 'CStr'), ('roots', 'List Word'),
                 ('statics', 'List Word'), ('decls', 'List CDecl')]),
    ]),
]

CTORS = {}       # constructor -> (type, [(field, field type)])
for _t, _doc, _cs in SCHEMA:
    for _c, _fs in _cs:
        assert _c not in CTORS, _c
        CTORS[_c] = (_t, _fs)

AST_PRELUDE = '''-- lib/cxx/ast.cub — Clang's AST after semantic analysis, as plain first-order data (archive/2026-11-11/PLAN-rev9.md, revision 9, 9 Nov 2026, sha256 4d012c3a10fd…, §6 “Phases 2–8: everything that will be built”, “Phase 4 — C++: the abstract machine, reading, and compilation”, item 2 “The AST”; archive/2026-11-11/C-clang.md, sha256 31dca2011367…, “Track C — C++ as data: Clang\'s AST into the checker (first half of D3)”). GENERATED by `python3 tools/clang2cub.py --emit-ast`: the
-- schema in that file is the one description of these constructors, and every unit it writes
-- (experiments/cxx/*.cub) is a value of CTU built against it. Loaded after lib/prim.cub.
--
-- A constructor exists for each Clang node kind, cast kind, operator and builtin type the
-- importer has met in the units imported so far; an unmet kind stops the import, and the
-- constructor is added then. Declaration ids are Words, unique within a unit. Clang's Sema is
-- the oracle for a value of this type: "the AST written is the AST Sema produced" is trusted.
--
-- The checker has no mutual data and rejects `List D` as a field of `data D`, so the three
-- recursive types carry their own list constructors (ct-nil/ct-cons, ce-nil/ce-cons,
-- cs-nil/cs-cons): a field documented as a list of expressions is a CExpr built from those two,
-- and one function recursing structurally over CExpr reads both nodes and lists. An absent
-- expression child is ce-nil, an absent statement child cs-none.

-- ---------------------------------------------------------------- strings
-- A string is its length and its bytes. There is no byte-string literal: cstr builds one from
-- the list of its bytes. cstr-put is %store8 (bounds-checked by the machine: a store outside
-- the string leaves it unchanged), trusted like the stores of lib/prim.cub.
primitive cstr-put : (n : Nat) → Bytes n → Word → Word → Bytes n = λ n b off v → %store8 b off (%u32ofw v)
CStr : Type
CStr = (n : Nat) × Bytes n
cstr-fill : (n : Nat) → List Word → Word → Bytes n → Bytes n
cstr-fill n nil off b = b
cstr-fill n (cons c cs) off b = cstr-fill n cs (wadd off #1) (cstr-put n b off c)
cstr-len : List Word → Nat
cstr-len nil = zero
cstr-len (cons c cs) = suc (cstr-len cs)
cstr : List Word → CStr
cstr cs = (cstr-len cs , cstr-fill (cstr-len cs) cs #0 (bytes (cstr-len cs)))
-- reading a string: its length, the byte at an index (0 outside the string: %load8 is
-- bounds-checked by the machine; trusted like cstr-put), and whether it is exactly these bytes
primitive cstr-get : (n : Nat) → Bytes n → Word → Word = λ n b off → %wofu32 (%load8 b off)
cstr-length : CStr → Word
cstr-length s = fromNat (fst s)
cstr-at : CStr → Word → Word
cstr-at s i = cstr-get (fst s) (snd s) i
cstr-count : List Word → Word
cstr-count nil = #0
cstr-count (cons c cs) = wadd #1 (cstr-count cs)
cstr-from : (n : Nat) → Bytes n → List Word → Word → Bool
cstr-from n b nil off = true
cstr-from n b (cons c cs) off = case weq (cstr-get n b off) c of
  true → cstr-from n b cs (wadd off #1)
  false → false
cstr-is : CStr → List Word → Bool
cstr-is s cs = case weq (cstr-length s) (cstr-count cs) of
  true → cstr-from (fst s) (snd s) cs #0
  false → false

-- ---------------------------------------------------------------- the AST
'''


def emit_ast(out):
    out.write(AST_PRELUDE)
    for t, doc, cs in SCHEMA:
        out.write('-- %s: %s\n' % (t, doc))
        parts = []
        for c, fs in cs:
            parts.append(c + ''.join(' (%s : %s)' % (f, ft) for f, ft in fs))
        if len(' | '.join(parts)) < 100:
            out.write('data %s = %s\n' % (t, ' | '.join(parts)))
        else:
            out.write('data %s =\n    %s\n' % (t, '\n  | '.join(parts)))


# ============================================================================ terms
class T:
    """A constructor applied to its fields, checked against the schema as it is built."""
    __slots__ = ('con', 'args', 'size')

    def __init__(self, con, *args):
        if con not in CTORS:
            stop('%s: not met in any unit imported before; add the constructor to SCHEMA in tools/clang2cub.py '
                 'and regenerate lib/cxx/ast.cub' % con)
        fs = CTORS[con][1]
        if len(fs) != len(args):
            stop('internal: %s takes %d fields, given %d' % (con, len(fs), len(args)))
        for (f, ft), a in zip(fs, args):
            check_field(con, f, ft, a)
        self.con, self.args = con, args
        self.size = 1 + sum(term_size(a) for a in args)


class Ref:
    """A reference to a top-level definition of the unit (a shared type, a hoisted subterm)."""
    __slots__ = ('name', 'sort')
    size = 1

    def __init__(self, name, sort):
        self.name, self.sort = name, sort


class W:
    """A Word literal."""
    __slots__ = ('n',)
    size = 1

    def __init__(self, n):
        if not isinstance(n, int) or n < 0:
            stop('internal: a Word is a natural number, given %r' % (n,))
        self.n = n


class S:
    """A string: its bytes."""
    __slots__ = ('b', 'size')

    def __init__(self, b):
        if isinstance(b, str):
            b = b.encode('utf-8')
        self.b = b
        self.size = 2 + 2 * len(b)


def term_size(a):
    if isinstance(a, list):
        return 1 + sum(term_size(x) for x in a)
    if isinstance(a, bool):
        return 1
    return a.size


def sort_of(a):
    if isinstance(a, T):
        return CTORS[a.con][0]
    if isinstance(a, Ref):
        return a.sort
    if isinstance(a, W):
        return 'Word'
    if isinstance(a, S):
        return 'CStr'
    if isinstance(a, bool):
        return 'Bool'
    return None


def check_field(con, f, ft, a):
    if ft.startswith('List '):
        if not isinstance(a, list):
            stop('internal: %s.%s is a %s, given %r' % (con, f, ft, a))
        for x in a:
            check_field(con, f, ft[5:], x)
        return
    if sort_of(a) != ft:
        stop('internal: %s.%s is a %s, given a %s' % (con, f, ft, sort_of(a)))


def inlist(sort, items):
    p = INLIST[sort]
    r = T(p + '-nil')
    for x in reversed(items):
        r = T(p + '-cons', x, r)
    return r


def cint(v):
    return T('cint-pos', W(v)) if v >= 0 else T('cint-neg', W(-v))


def optid(v):
    return T('coi-none') if v is None else T('coi-some', W(v))


# ============================================================================ type strings
BUILTIN_KW = {'void', 'bool', 'char', 'short', 'int', 'long', 'signed', 'unsigned', 'float',
              'double', '__int128', 'char8_t', 'char16_t', 'char32_t', 'wchar_t', '_Bool'}
BUILTINS = {
    ('void',): 'void', ('bool',): 'bool', ('_Bool',): 'bool', ('char',): 'char',
    ('char', 'signed'): 'schar', ('char', 'unsigned'): 'uchar',
    ('short',): 'short', ('short', 'unsigned'): 'ushort', ('int', 'short'): 'short',
    ('int',): 'int', ('unsigned',): 'uint', ('int', 'unsigned'): 'uint', ('signed',): 'int',
    ('long',): 'long', ('long', 'unsigned'): 'ulong', ('int', 'long'): 'long',
    ('long', 'long'): 'llong', ('long', 'long', 'unsigned'): 'ullong',
    ('__int128',): 'int128', ('__int128', 'unsigned'): 'uint128',
    ('float',): 'float', ('double',): 'double', ('double', 'long'): 'ldouble',
    ('char8_t',): 'char8', ('char16_t',): 'char16', ('char32_t',): 'char32',
    ('wchar_t',): 'wchar',
}
UNSIGNED_BITS = {'bool': 1, 'uchar': 8, 'char8': 8, 'ushort': 16, 'char16': 16, 'uint': 32, 'char32': 32,
                 'ulong': 64, 'ullong': 64}       # x86-64 (the import stops elsewhere)
SIGNED_KINDS = ('schar', 'short', 'int', 'long', 'llong', 'char', 'wchar', 'int128')
PLACEHOLDERS = {'<bound member function type>': 'bound-member', '<builtin fn type>': 'builtin-fn'}
TOKEN = re.compile(r"\s*(?:([A-Za-z_$][A-Za-z0-9_$]*)|(\d+)[uUlL]*|(::|->|&&|\.\.\.|'(?:[^'\\]|\\.)+'|.))")
SPECIAL = ('(lambda at ', '(anonymous ', '(unnamed ')


def tokenize(s):
    toks, i = [], 0
    while i < len(s):
        if s[i].isspace():
            i += 1
            continue
        if s.startswith(SPECIAL, i):
            depth, j = 0, i
            while True:
                if j >= len(s):
                    stop('type string: unclosed parenthesis in %r' % s)
                if s[j] == '(':
                    depth += 1
                elif s[j] == ')':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            toks.append(('special', s[i:j + 1]))
            i = j + 1
            continue
        m = TOKEN.match(s, i)
        if m.group(1):
            toks.append(('id', m.group(1)))
        elif m.group(2):
            toks.append(('num', int(m.group(2))))
        else:
            toks.append(('p', m.group(3)))
        i = m.end()
    toks.append(('end', None))
    return toks


def qual(t, c=False, v=False, r=False):
    """t with qualifiers added (qualifiers on an array are its elements')."""
    if not (c or v or r):
        return t
    if t[0] == 'array':
        return ('array', qual(t[1], c, v, r), t[2])
    if t[0] == 'qual':
        return ('qual', t[1] or c, t[2] or v, t[3] or r, t[4])
    return ('qual', c, v, r, t)


def unqual(t):
    return t[4] if t[0] == 'qual' else t


class TypeParser:
    def __init__(self, unit, s, scope, hint, env=None):
        self.u, self.s, self.scope, self.hint = unit, s, scope, hint
        self.env = env or {}        # template parameter -> ('type', T) / ('int', n), in an alias template
        self.toks = tokenize(s)
        self.i = 0

    def peek(self, k=0):
        return self.toks[min(self.i + k, len(self.toks) - 1)]

    def next(self):
        t = self.toks[self.i]
        self.i += 1
        return t

    def at(self, p, k=0):
        return self.peek(k) == ('p', p)

    def expect(self, p):
        if not self.at(p):
            self.fail('expected %r' % p)
        self.i += 1

    def fail(self, why):
        stop('type string not read: %r (%s, at token %d %r)' % (self.s, why, self.i, self.peek()))

    def whole(self):
        t = self.type()
        if self.peek()[0] != 'end':
            self.fail('trailing text')
        if has_auto(t):
            raise Dependent('auto in %r' % self.s)
        return t

    def type(self):
        base = self.declspec()
        return self.declarator()(base)

    # ---- declaration specifiers: qualifiers, a builtin or one (qualified, templated) name
    def declspec(self):
        c = v = False
        kws, base = [], None
        while True:
            k, x = self.peek()
            if k == 'id' and x == 'const':
                c = True
            elif k == 'id' and x == 'volatile':
                v = True
            elif k == 'id' and x in ('struct', 'class', 'union', 'enum', 'typename'):
                pass
            elif k == 'id' and x in BUILTIN_KW and base is None:
                kws.append(x)
            elif k == 'id' and x == 'decltype' and base is None and not kws:
                if [self.peek(j) for j in (1, 2, 3)] == [('p', '('), ('id', 'nullptr'), ('p', ')')]:
                    self.i += 3
                    base = ('builtin', 'nullptr')
                else:
                    raise NeedsProbe('type string not read: %r (decltype of an expression)' % self.s)
            elif k == 'id' and x == 'auto' and base is None and not kws:
                base = ('auto',)        # stands only before a trailing return type
            elif (k in ('id', 'special') or (k, x) == ('p', '::')) and base is None and not kws:
                base = self.u.resolve_name(self.name(), self.scope, self.hint, self.s, self.env)
                continue
            else:
                break
            self.i += 1
        if kws:
            key = tuple(sorted(kws))
            if key not in BUILTINS:
                self.fail('builtin type %r' % ' '.join(kws))
            base = ('builtin', BUILTINS[key])
        if base is None:
            self.fail('no type specifier')
        return qual(base, c, v)

    def name(self):
        """A qualified name: [(identifier or special text, template arguments or None)]."""
        comps = []
        if self.at('::'):
            self.i += 1
            comps.append(('', None))
        while True:
            k, x = self.next()
            if k not in ('id', 'special'):
                self.i -= 1
                self.fail('a name')
            args = None
            if self.at('<'):
                self.i += 1
                args = []
                while not self.at('>'):
                    args.append(self.targ())
                    if self.at(','):
                        self.i += 1
                    elif not self.at('>'):
                        self.fail('template arguments')
                self.i += 1
            comps.append((x, args))
            if self.at('::') and not self.at('*', 1):
                self.i += 1
                continue
            return comps

    def targ(self):
        k, x = self.peek()
        if k == 'num':
            self.i += 1
            return ('int', x)
        if (k, x) == ('p', '-') and self.peek(1)[0] == 'num':
            self.i += 2
            return ('int', -self.toks[self.i - 1][1])
        if k == 'id' and x in ('true', 'false'):
            self.i += 1
            return ('int', 1 if x == 'true' else 0)
        if k == 'p' and x.startswith("'"):
            self.i += 1
            return ('int', ord(eval(x)))
        if k == 'id' and self.env.get(x, ('type',))[0] == 'int' and self.peek(1) in (('p', ','), ('p', '>')):
            self.i += 1
            return self.env[x]
        start = self.i
        try:
            return ('type', self.type())
        except NotAType:
            # not a type: the name of a constant (std::span's `dynamic_extent`)
            self.i = start
            comps = self.name()
            if not (self.at(',') or self.at('>')) or any(a is not None for _, a in comps):
                raise
            return ('int', self.u.constant_value(comps, self.s))

    # ---- abstract declarators, read inside out
    def memptr_ahead(self):
        """Is a `C::*` next?  (a name followed by ::*)"""
        j = self.i
        if self.toks[j][0] != 'id' or self.toks[j][1] in ('const', 'volatile', 'noexcept'):
            return False
        depth = 0
        while j < len(self.toks):
            k, x = self.toks[j]
            if (k, x) == ('p', '<'):
                depth += 1
            elif (k, x) == ('p', '>'):
                depth -= 1
            elif depth == 0:
                if (k, x) == ('p', '::') and self.toks[j + 1] == ('p', '*'):
                    return True
                if k != 'id' and (k, x) != ('p', '::'):
                    return False
            if k == 'end':
                return False
            j += 1
        return False

    def cv_after(self):
        c = v = r = False
        while self.peek()[0] == 'id' and self.peek()[1] in ('const', 'volatile', 'restrict', '__restrict'):
            x = self.next()[1]
            c, v, r = c or x == 'const', v or x == 'volatile', r or x in ('restrict', '__restrict')
        return c, v, r

    def declarator(self):
        ops = []
        while True:
            if self.at('*'):
                self.i += 1
                ops.append(('ptr',) + self.cv_after())
            elif self.at('&'):
                self.i += 1
                ops.append(('lref',))
            elif self.at('&&'):
                self.i += 1
                ops.append(('rref',))
            elif self.memptr_ahead():
                cls = self.u.resolve_name(self.name(), self.scope, None, self.s, self.env)
                self.expect('::')
                self.expect('*')
                ops.append(('memptr', cls) + self.cv_after())
            else:
                break
        direct = self.direct()

        def build(t):
            for op in ops:
                if op[0] == 'ptr':
                    t = qual(('ptr', t), *op[1:])
                elif op[0] == 'memptr':
                    if op[1][0] != 'record':
                        self.fail('member pointer into a non-class')
                    t = qual(('memptr', op[1][1], t), *op[2:])
                else:
                    t = (op[0], t)
            return direct(t)
        return build

    def direct(self):
        inner = None
        if self.at('(') and (self.peek(1) in (('p', '*'), ('p', '&'), ('p', '&&'), ('p', '('))
                             or (self.peek(1)[0] == 'id' and self._group_memptr())):
            self.i += 1
            inner = self.declarator()
            self.expect(')')
        sufs = []
        while True:
            if self.at('['):
                self.i += 1
                if self.at(']'):
                    self.fail('array of unknown bound')
                k, n = self.next()
                if k == 'id' and self.env.get(n, ('type',))[0] == 'int':
                    k, n = 'num', self.env[n][1]
                if k != 'num':
                    self.fail('array bound')
                self.expect(']')
                sufs.append(('array', n))
            elif self.at('('):
                self.i += 1
                params, variadic = [], False
                while not self.at(')'):
                    if self.at('...'):
                        self.i += 1
                        variadic = True
                    else:
                        params.append(self.type())
                    if self.at(','):
                        self.i += 1
                    elif not self.at(')'):
                        self.fail('parameter list')
                self.i += 1
                c = v = False
                ref, noex, ret = 'none', False, None
                while True:
                    k, x = self.peek()
                    if k == 'id' and x == 'const':
                        c = True
                    elif k == 'id' and x == 'volatile':
                        v = True
                    elif (k, x) == ('p', '&'):
                        ref = 'lvalue'
                    elif (k, x) == ('p', '&&'):
                        ref = 'rvalue'
                    elif k == 'id' and x == 'noexcept':
                        noex = True
                        if self.at('(', 1):
                            if self.peek(2) == ('id', 'true') and self.at(')', 3):
                                self.i += 3
                            elif self.peek(2) == ('id', 'false') and self.at(')', 3):
                                self.i += 3
                                noex = False
                            else:
                                raise NeedsProbe('type string not read: %r (noexcept of an expression)' % self.s)
                    elif k == 'id' and x == 'throw' and self.at('(', 1) and self.at(')', 2):
                        self.i += 2
                        noex = True
                    elif k == 'id' and x == '__attribute__' and [self.peek(j) for j in (1, 2, 3, 4, 5)] == \
                            [('p', '('), ('p', '('), ('id', 'noreturn'), ('p', ')'), ('p', ')')]:
                        self.i += 5
                        self.u.type_attrs.add('__attribute__((noreturn)) in a function type')
                    elif (k, x) == ('p', '->'):
                        self.i += 1
                        ret = self.type()
                        break
                    else:
                        break
                    self.i += 1
                sufs.append(('func', tuple(params), variadic, c, v, ref, noex, ret))
            else:
                break

        def build(t):
            for suf in reversed(sufs):
                if suf[0] == 'array':
                    t = ('array', t, suf[1])
                else:
                    t = ('func', suf[7] if suf[7] is not None else t) + suf[1:7]
            return inner(t) if inner else t
        return build

    def _group_memptr(self):
        self.i += 1
        r = self.memptr_ahead()
        self.i -= 1
        return r


def has_auto(t):
    return t == ('auto',) or any(has_auto(x) for x in t if isinstance(x, tuple))


def type_str(t):
    k = t[0]
    if k == 'builtin':
        return t[1]
    if k in ('ptr', 'lref', 'rref'):
        return '%s(%s)' % (k, type_str(t[1]))
    if k == 'array':
        return '%s[%d]' % (type_str(t[1]), t[2])
    if k == 'func':
        return 'fn(%s)->%s' % (','.join(type_str(p) for p in t[2]), type_str(t[1]))
    if k == 'qual':
        return 'q%d%d%d %s' % (t[1], t[2], t[3], type_str(t[4]))
    return '%s#%s' % (k, t[1])


# ============================================================================ the unit
RECORD_KINDS = ('CXXRecordDecl', 'ClassTemplateSpecializationDecl')
FUNCTION_KINDS = ('FunctionDecl', 'CXXMethodDecl', 'CXXConstructorDecl', 'CXXDestructorDecl',
                  'CXXConversionDecl')
TEMPLATE_KINDS = ('ClassTemplateDecl', 'FunctionTemplateDecl', 'TypeAliasTemplateDecl',
                  'ClassTemplatePartialSpecializationDecl', 'VarTemplateDecl',
                  'VarTemplatePartialSpecializationDecl', 'TemplateTemplateParmDecl',
                  'ConceptDecl', 'CXXDeductionGuideDecl')
TYPE_ENTITY_KINDS = RECORD_KINDS + ('EnumDecl', 'TypedefDecl', 'TypeAliasDecl', 'ClassTemplateDecl',
                                    'TypeAliasTemplateDecl', 'NamespaceDecl', 'NamespaceAliasDecl',
                                    'ClassTemplatePartialSpecializationDecl')
SCOPE_KINDS = RECORD_KINDS + FUNCTION_KINDS + ('NamespaceDecl', 'TranslationUnitDecl',
                                               'ClassTemplatePartialSpecializationDecl')
# Attributes with no node of their own in the AST written: what each one does is already in
# what is written (a layout, a chosen overload) or holds only at compile time.
ATTRS_CARRIED_ELSEWHERE = {
    'PackedAttr': 'in the record layout', 'AlignedAttr': 'in the record layout',
    'NoUniqueAddressAttr': 'in the record layout',
    'FinalAttr': 'compile time', 'OverrideAttr': 'compile time',
    'OwnerAttr': 'a lifetime-analysis hint', 'PointerAttr': 'a lifetime-analysis hint',
    'NoThrowAttr': 'in the function type', 'NonNullAttr': 'a diagnostic and optimizer hint',
    'BuiltinAttr': 'the name is the builtin', 'ConstAttr': 'an optimizer hint',
    'PureAttr': 'an optimizer hint', 'AlwaysInlineAttr': 'an optimizer hint',
    'VisibilityAttr': 'linkage, outside the unit', 'AsmLabelAttr': 'in the mangled name',
    'NoDiscardAttr': 'a diagnostic', 'WarnUnusedResultAttr': 'a diagnostic',
    'DeprecatedAttr': 'a diagnostic', 'ArtificialAttr': 'a debugger hint',
    'AbiTagAttr': 'in the mangled name', 'GNUInlineAttr': 'an inlining hint',
    'ReturnsNonNullAttr': 'an optimizer hint', 'AllocSizeAttr': 'an optimizer hint',
    'AllocAlignAttr': 'an optimizer hint', 'RestrictAttr': 'an optimizer hint',
    'UnusedAttr': 'a diagnostic', 'NoInlineAttr': 'an optimizer hint',
    'FormatAttr': 'a diagnostic', 'ReturnsTwiceAttr': 'an optimizer hint',
    'ColdAttr': 'an optimizer hint', 'MaybeUnusedAttr': 'a diagnostic',
    'CXX11NoReturnAttr': 'an optimizer hint', 'NoReturnAttr': 'an optimizer hint',
    'LeafAttr': 'an optimizer hint',
}
# Declarations inside a function body that leave nothing to run: the names they introduce were
# resolved by Sema (a local class or enum that is used is written with the unit's declarations).
LOCAL_DECLS_COMPILE_TIME = ('TypedefDecl', 'TypeAliasDecl', 'UsingDecl', 'UsingDirectiveDecl',
                            'StaticAssertDecl', 'CXXRecordDecl', 'EnumDecl', 'UsingShadowDecl')

UNOPS = {('++', True): 'post-inc', ('--', True): 'post-dec', ('++', False): 'pre-inc',
         ('--', False): 'pre-dec', ('&', False): 'addr-of', ('*', False): 'deref',
         ('+', False): 'plus', ('-', False): 'minus', ('~', False): 'not', ('!', False): 'lnot'}
BINOPS = {'*': 'mul', '/': 'div', '%': 'rem', '+': 'add', '-': 'sub', '<<': 'shl', '>>': 'shr',
          '<': 'lt', '>': 'gt', '<=': 'le', '>=': 'ge', '==': 'eq', '!=': 'ne', '&': 'and',
          '^': 'xor', '|': 'or', '&&': 'land', '||': 'lor', '=': 'assign', ',': 'comma',
          '.*': 'ptr-mem-d', '->*': 'ptr-mem-i'}
REFKINDS = {'FunctionDecl': 'function', 'CXXMethodDecl': 'method', 'VarDecl': 'var',
            'VarTemplateSpecializationDecl': 'var',
            'ParmVarDecl': 'parm', 'EnumConstantDecl': 'enum-constant', 'FieldDecl': 'field',
            'CXXConversionDecl': 'method', 'CXXDestructorDecl': 'method',
            'CXXConstructorDecl': 'method'}
CAST_NODES = {'ImplicitCastExpr': 'ce-implicit-cast', 'CXXStaticCastExpr': 'ce-static-cast',
              'CXXDynamicCastExpr': 'ce-dynamic-cast',
              'CXXReinterpretCastExpr': 'ce-reinterpret-cast',
              'CXXFunctionalCastExpr': 'ce-functional-cast', 'CStyleCastExpr': 'ce-cstyle-cast'}


class Unit:
    def __init__(self, path, name, flags=None):
        self.path, self.name = path, name
        self.flags = list(flags if flags is not None else CLANG_FLAGS)
        self.met = {}            # node kind -> count, for the kinds written
        self.met_casts = {}
        self.elided = {}         # kind -> (count, reason)
        self.scope_dependent = set()

    # ------------------------------------------------------------------ running Clang
    def run_clang(self):
        base = ['clang++'] + self.flags
        p = subprocess.run(base + ['-fsyntax-only', '-Xclang', '-ast-dump=json', '-Xclang',
                                   '-fdump-record-layouts', self.path],
                           capture_output=True, text=True)
        if p.returncode != 0:
            stop('clang failed on %s:\n%s' % (self.path, p.stderr[-2000:]))
        cut = p.stdout.find('\n{\n')
        if p.stdout.startswith('{\n'):
            cut = -1
        self.layout_text = p.stdout[:cut + 1]
        self.tu = json.loads(p.stdout[cut + 1:])
        import tempfile
        with tempfile.NamedTemporaryFile(suffix='.ll') as ir:
            p = subprocess.run(base + ['-emit-llvm', '-S', '-o', ir.name, '-Xclang',
                                       '-fdump-vtable-layouts', self.path],
                               capture_output=True, text=True)
            if p.returncode != 0:
                stop('clang failed on %s:\n%s' % (self.path, p.stderr[-2000:]))
            self.vtable_text = p.stdout
            with open(ir.name) as fh:
                self.ir_text = fh.read()
            self.ir_globals = [l for l in self.ir_text.splitlines() if l.startswith('@')]
        p = subprocess.run(base + ['-fsyntax-only', '-Xclang', '-ast-dump', self.path],
                           capture_output=True, text=True)
        if p.returncode != 0:
            stop('clang failed on %s:\n%s' % (self.path, p.stderr[-2000:]))
        self.member_lines, self.aligned_lines = [], []
        lines = p.stdout.splitlines()
        for i, line in enumerate(lines):
            m = re.match(r"^([|` -]*)MemberExpr 0x[0-9a-f]+ <.*?> '.*' (?:lvalue |xvalue |bitfield )*(->|\.)(.*?) ?0x[0-9a-f]+", line)
            if m:
                nxt = lines[i + 1] if i + 1 < len(lines) else ''
                qualified = len(nxt) > len(m.group(1)) and nxt[len(m.group(1)):].lstrip('|` -').startswith('NestedNameSpecifier')
                self.member_lines.append((m.group(2) == '->', m.group(3), qualified))
            elif re.match(r'^[|` -]*MemberExpr 0x', line):
                stop('the text dump has a MemberExpr line this file does not read: %r' % line)
            m = re.match(r"^[|` -]*AlignedAttr 0x[0-9a-f]+ <.*?>(?: Implicit| Inherited)* (\w+)(?: '(.*?)'(?::'.*')?)?$", line)
            if m:
                self.aligned_lines.append(m.group(2))
            elif re.match(r'^[|` -]*AlignedAttr 0x', line):
                stop('the text dump has an AlignedAttr line this file does not read: %r' % line)
        p = subprocess.run(base + ['-print-target-triple'], capture_output=True, text=True)
        self.target = p.stdout.strip()
        p = subprocess.run(['clang++', '--version'], capture_output=True, text=True)
        self.clang_version = p.stdout.splitlines()[0].strip()

    # ------------------------------------------------------------------ the index
    def index(self):
        self.nodes, self.parent = {}, {}
        self.by_name = {}                   # identifier -> type entities anywhere in the unit
        self.lambdas = {}                   # "(lambda at ...)" -> closure records
        self.unnamed = {}                   # "file:line:col" -> the unnamed classes and enums there
        self.var_by_name = {}
        self.ns_nodes = {}                  # qualified namespace name -> its NamespaceDecls
        self.redecl = {}                    # id -> id of the previous declaration
        last = {'file': None, 'line': None}

        def locs(o):
            for k, v in o.items():
                if k == 'file':
                    last['file'] = v
                elif k == 'line':
                    last['line'] = v
                elif isinstance(v, dict) and k != 'includedFrom':
                    locs(v)

        members, aligned = [], []
        stack = [(self.tu, None)]
        while stack:
            n, par = stack.pop()
            if 'loc' in n:
                locs(n['loc'])
                n['_file'], n['_line'] = last['file'], last['line']
                if 'col' in n['loc']:
                    n['_col'] = n['loc']['col']
                elif 'expansionLoc' in n['loc']:
                    n['_col'] = n['loc']['expansionLoc'].get('col')
            if 'range' in n:
                if 'begin' in n['range']:
                    locs(n['range']['begin'])
                    b = n['range']['begin']
                    b = b.get('expansionLoc', b)
                    if n.get('kind') in RECORD_KINDS + ('EnumDecl',) and 'name' not in n and 'col' in b:
                        self.unnamed.setdefault('%s:%s:%s' % (last['file'], last['line'], b['col']), []).append(n)
                locs(n['range'])
            # a declaration listed again elsewhere (an instantiation under another declaration
            # of its template) is printed there as a stub: id, kind, name. The node is the full one.
            stub = 'id' in n and not (set(n) - {'id', 'kind', 'name'})
            if 'id' in n and not (stub and n['id'] in self.nodes):
                self.nodes[n['id']] = n
                n['_order'] = len(self.nodes)
            self.parent[id(n)] = par
            k = n.get('kind')
            if k in TYPE_ENTITY_KINDS and n.get('name') and not stub:
                self.by_name.setdefault(n['name'], []).append(n)
            if k == 'LambdaExpr':
                self.lambdas.setdefault(n['type']['qualType'], []).append(n['inner'][0])
            if k == 'VarDecl' and n.get('name') and not stub:
                self.var_by_name.setdefault(n['name'], []).append(n)
            if k == 'MemberExpr':
                members.append(n)
            if k == 'AlignedAttr':
                aligned.append(n)
            if 'previousDecl' in n:
                self.redecl[n['id']] = n['previousDecl']
            # children: `inner`, or for an InitListExpr with a filler `array_filler` (the filler,
            # then the initializers: the JSON puts them all under that label)
            for c in reversed(n.get('inner', []) + n.get('array_filler', [])):
                if c:
                    stack.append((c, n))
        if len(members) != len(self.member_lines):
            stop('the JSON dump has %d MemberExprs and the text dump %d: the two do not list the same nodes'
                 % (len(members), len(self.member_lines)))
        for n, (arrow, name, qualified) in zip(members, self.member_lines):
            # a member without a name is printed by the text dump as its unnamed type
            same = n['name'] == name if n.get('name') else (name == '' or '(anonymous' in name or '(unnamed' in name)
            if bool(n.get('isArrow')) != arrow or not same:
                stop('the MemberExprs of the JSON and text dumps do not correspond: %r %r against %r %r'
                     % (n.get('isArrow'), n.get('name'), arrow, name))
            n['_qualified'] = qualified
        if len(aligned) != len(self.aligned_lines):
            stop('the JSON dump has %d AlignedAttrs and the text dump %d: the two do not list the same nodes'
                 % (len(aligned), len(self.aligned_lines)))
        for n, t in zip(aligned, self.aligned_lines):
            if (t is None) != bool(n.get('inner')):
                stop('the AlignedAttrs of the JSON and text dumps do not correspond (one has an expression, the other a type)')
            n['_align_type'] = t
        for n in self.nodes.values():
            if n.get('kind') == 'NamespaceDecl':
                self.ns_nodes.setdefault(self.ns_key(n), []).append(n)
        # redeclaration chains: every declaration of an entity -> its defining declaration
        groups = {}
        for i in self.nodes:
            r = i
            while r in self.redecl and self.redecl[r] in self.nodes:
                r = self.redecl[r]
            groups.setdefault(r, []).append(i)
        self.canon, self.redecls = {}, {}
        for r, ids in groups.items():
            if len(ids) == 1:
                continue
            d = [i for i in ids if self.is_definition(self.nodes[i])]
            c = d[0] if d else r
            for i in ids:
                self.canon[i] = c
            self.redecls[c] = ids
        self.type_memo, self.typedef_memo, self.table_memo, self.spec_memo = {}, {}, {}, {}
        self.global_memo = {}
        self.probes, self.fn_type_memo = {}, {}
        self.sources, self.stubs = {}, []
        self._dynamic = None
        self.mangled_memo, self.from_mangled = {}, set()
        self.opaque = {}
        self.type_attrs = set()
        self.lazy_inits = set()

    def is_definition(self, n):
        k = n['kind']
        if k in RECORD_KINDS or k == 'EnumDecl':
            return bool(n.get('completeDefinition')) or k == 'EnumDecl' and 'inner' in n
        if k in FUNCTION_KINDS:
            return any(c.get('kind') in ('CompoundStmt', 'CXXTryStmt', 'CoroutineBodyStmt') for c in n.get('inner', [])) or \
                'explicitlyDefaulted' in n
        if k in ('VarDecl', 'VarTemplateSpecializationDecl'):
            return 'init' in n or n.get('storageClass') != 'extern'
        return False

    def node(self, i):
        if i not in self.nodes:
            stop('a declaration id the dump does not contain: %s' % i)
        return self.nodes[self.canon.get(i, i)]

    def ns_key(self, n):
        parts = []
        while n is not None and n.get('kind') != 'TranslationUnitDecl':
            if n.get('kind') == 'NamespaceDecl':
                parts.append(n.get('name', '(anonymous namespace)'))
            n = self.parent[id(n)]
        return '::'.join(reversed(parts))

    def qualified_name(self, n):
        parts = []
        while n is not None and n.get('kind') != 'TranslationUnitDecl':
            if n.get('kind') in SCOPE_KINDS and n.get('kind') not in FUNCTION_KINDS:
                parts.append(n.get('name', '(anonymous)'))
            sem = n.get('parentDeclContextId')
            n = self.nodes[sem] if sem in self.nodes and parts == [] else self.parent[id(n)]
        return '::'.join(reversed(parts))

    def display_name(self, n):
        """A name for the comments of the output (nothing is resolved by it)."""
        parts = []
        while n is not None and n.get('kind') != 'TranslationUnitDecl':
            k = n.get('kind')
            if k in SCOPE_KINDS or not parts:
                nm = n.get('name') or ('(lambda)' if self.is_lambda(n) else '(unnamed)')
                if k == 'ClassTemplateSpecializationDecl':
                    try:
                        nm += '<%s>' % ', '.join(self.show_type(x) if kd == 'type' else str(x) for kd, x in self.template_args(n))
                    except (Dependent, Stop):
                        nm += '<...>'
                parts.append(nm)
            sem = n.get('parentDeclContextId')
            n = self.nodes[sem] if sem in self.nodes and len(parts) == 1 else self.parent[id(n)]
        return '::'.join(reversed(parts))

    def show_type(self, t):
        k = t[0]
        if k == 'builtin':
            return t[1]
        if k in ('record', 'enum'):
            return self.display_name(self.nodes[t[1]])
        if k == 'ptr':
            return self.show_type(t[1]) + ' *'
        if k == 'lref':
            return self.show_type(t[1]) + ' &'
        if k == 'rref':
            return self.show_type(t[1]) + ' &&'
        if k == 'array':
            return '%s[%d]' % (self.show_type(t[1]), t[2])
        if k == 'qual':
            return ' '.join(q for q, on in zip(('const', 'volatile', 'restrict'), t[1:4]) if on) + ' ' + self.show_type(t[4])
        if k == 'func':
            return '%s (%s)' % (self.show_type(t[1]), ', '.join(self.show_type(p) for p in t[2]))
        return type_str(t)

    def where(self, n):
        while n is not None and '_file' not in n:
            n = self.parent[id(n)]
        return '%s:%s' % (n.get('_file'), n.get('_line')) if n else '?'

    # ------------------------------------------------------------------ name lookup
    def scope_chain(self, n):
        """The contexts a name used at n is looked up in, innermost first."""
        while n is not None:
            k = n.get('kind')
            if k in TEMPLATE_KINDS:
                yield ('template', n)
            if k in SCOPE_KINDS:
                yield ('ns', self.ns_key(n)) if k == 'NamespaceDecl' else ('decl', n)
            sem = n.get('parentDeclContextId')
            if sem in self.nodes and self.nodes[sem].get('kind') in RECORD_KINDS and k in FUNCTION_KINDS + ('VarDecl',):
                m = self.nodes[sem]
                while m is not None and m.get('kind') in RECORD_KINDS:
                    yield ('decl', self.node(m['id']))
                    m = self.parent[id(m)]
            n = self.parent[id(n)]

    def members(self, n):
        """name -> type entities declared directly in the declaration context n."""
        t = self.table_memo.get(id(n))
        if t is not None:
            return t
        t = {}
        self.table_memo[id(n)] = t

        def add(c):
            k = c.get('kind')
            if k in ('LinkageSpecDecl', 'ExportDecl') or (k == 'NamespaceDecl' and (c.get('isInline') or 'name' not in c)):
                for d in c.get('inner', []):
                    add(d)
            if k == 'UsingShadowDecl':
                tg = self.nodes.get(c['target']['id'])
                if tg is not None and tg.get('kind') in TYPE_ENTITY_KINDS and tg.get('name'):
                    t.setdefault(tg['name'], []).append(tg)
            elif k == 'UsingDirectiveDecl':
                t.setdefault('', []).append(c)
            elif k in TYPE_ENTITY_KINDS and c.get('name') and k not in ('ClassTemplateSpecializationDecl',
                                                                       'ClassTemplatePartialSpecializationDecl'):
                # (an explicit specialization is found through its template, not by its name)
                t.setdefault(c['name'], []).append(self.nodes[c['id']])
        if n.get('kind') in FUNCTION_KINDS:
            # local types: anywhere in the body
            stack = list(n.get('inner', []))
            while stack:
                c = stack.pop()
                if c.get('kind') in ('TypedefDecl', 'TypeAliasDecl', 'EnumDecl') or \
                        (c.get('kind') == 'CXXRecordDecl' and c.get('name')):
                    t.setdefault(c['name'], []).append(c)
                elif c.get('kind') not in ('LambdaExpr',) + RECORD_KINDS:
                    stack.extend(x for x in c.get('inner', []) if x)
        else:
            for c in n.get('inner', []):
                add(c)
        return t

    def lookup_ctx(self, ctx, ident, seen=None):
        kind, x = ctx
        if kind == 'template':
            for c in x.get('inner', []):
                if c.get('kind', '').endswith('ParmDecl') and c.get('name') == ident:
                    raise Dependent('template parameter %s' % ident)
            return []
        if kind == 'ns':
            seen = seen if seen is not None else set()
            if x in seen:
                return []
            seen.add(x)
            out = []
            for n in self.ns_nodes.get(x, []) if x else [self.tu]:
                m = self.members(n)
                out += m.get(ident, [])
            if not out:
                for n in self.ns_nodes.get(x, []) if x else [self.tu]:
                    for d in self.members(n).get('', []):
                        nn = self.nodes[d['nominatedNamespace']['id']]
                        out += self.lookup_ctx(('ns', self.ns_key(nn)), ident, seen)
            return out
        n = x
        if n.get('kind') == 'TranslationUnitDecl':
            return self.lookup_ctx(('ns', ''), ident)
        out = list(self.members(n).get(ident, []))
        if not out and n.get('kind') in RECORD_KINDS + ('ClassTemplatePartialSpecializationDecl',):
            for b in n.get('bases', []):
                bt = self.base_type(n, b)
                out += self.lookup_ctx(('decl', self.nodes[bt[1]]), ident)
        return out

    def base_type(self, rec, b):
        """The class a base specifier of rec names (read in the scope the class is declared in:
        the class's own members are not in scope in its base clause)."""
        bt = unqual(self.resolve_type(b['type'], self.parent[id(rec)]))
        if bt[0] != 'record':
            stop('a base class that is not a class: %r of %s' % (b['type'], rec.get('name')))
        return bt

    def entity_key(self, e):
        if e.get('kind') == 'NamespaceDecl':
            return ('ns', self.ns_key(e))
        if e.get('kind') == 'CXXRecordDecl' and e.get('isImplicit'):
            p = self.parent[id(e)]
            if p.get('kind') in RECORD_KINDS + ('ClassTemplatePartialSpecializationDecl',) and p.get('name') == e.get('name'):
                e = p                                  # the injected class name
        return ('decl', self.canon.get(e['id'], e['id']))

    def resolve_name(self, comps, scope, hint, whole, env=None):
        """A qualified name used at scope -> the type it names."""
        first, args = comps[0]
        if env and first in env and args is None:
            if env[first][0] != 'type':
                stop('type string not read: %r (the constant %s where a type is expected)' % (whole, first))
            return self.resolve_from(('type', env[first][1]), comps[1:], scope, whole, False, env)
        if first.startswith('(lambda at '):
            if len(comps) != 1:
                stop('a member of a lambda named in a type: %r' % whole)
            return ('record', self.lambda_record(first, scope)['id'])
        if comps[-1][0].startswith(('(unnamed ', '(anonymous ')) and comps[-1][0] != '(anonymous namespace)':
            return self.unnamed_type(comps, scope, whole)
        if first == '(anonymous namespace)':
            return self.resolve_from(('ns', first), comps[1:], scope, whole, False, env)
        if first.startswith('('):
            stop('type string not read: %r (an unnamed entity %r: no rule yet)' % (whole, first))
        if hint is not None and len(comps) >= 1 and comps[-1] == (hint.get('name'), None):
            return self.typedef_type(hint)
        if first == '':
            ents, rest = [('ns', '')], comps[1:]
            via_scope = False
        else:
            found = []
            for ctx in self.scope_chain(scope):
                found = self.lookup_ctx(ctx, first)
                if found:
                    break
            via_scope = bool(found)
            if not found:
                # not in scope where the type is used: the string was written elsewhere (a
                # member typedef named in a signature). Every declaration of the name in the
                # unit is a candidate, and they must all mean the same type.
                found = [e for e in self.by_name.get(first, []) if e.get('kind') != 'ClassTemplateSpecializationDecl']
            # a declaration inside a template that is not instantiated is never what a name in
            # instantiated code means
            found = [e for e in found if not self.is_pattern(e)]
            if not found:
                raise NotAType('a name in a type is not declared as a type anywhere in the unit outside templates: %r in %r (used at %s)'
                               % (first, whole, self.where(scope)))
            ents, rest = sorted(set(self.entity_key(e) for e in found)), comps
        results = set()
        for ent in ents:
            results.add(self.resolve_from(ent, rest, scope, whole, first != '', env))
        if len(results) != 1:
            stop('a name in a type has several meanings and the scope does not choose: %r in %r (used at %s): %s'
                 % (first, whole, self.where(scope), sorted(type_str(r) for r in results)))
        if via_scope:
            self.note_scope_dependence(first)
        return results.pop()

    def unnamed_type(self, comps, scope, whole):
        """`(unnamed struct at file:line:col)`, possibly qualified: the class or enum declared
        there. A template and its instantiations declare one each at the same place; the
        qualifier says which."""
        m = re.match(r'\((?:unnamed|anonymous)(?: (struct|class|union|enum))? at (.*)\)$', comps[-1][0])
        if not m:
            stop('type string not read: %r (%r)' % (whole, comps[-1][0]))
        cands = [c for c in self.unnamed.get(m.group(2), []) if not self.is_pattern(c)]
        if len(cands) > 1 and len(comps) > 1:
            outer = self.resolve_name(comps[:-1], scope, None, whole)
            if outer[0] == 'record':
                cands = [c for c in cands if self.parent[id(c)] is self.nodes[outer[1]]]
        if len(cands) != 1:
            stop('type string not read: %r (%d unnamed declarations at that place)' % (whole, len(cands)))
        c = self.node(cands[0]['id'])
        return ('enum' if c['kind'] == 'EnumDecl' else 'record', c['id'])

    def constant_value(self, comps, whole):
        """A template argument printed as the name of a constant: its value. Every variable of
        that name in the unit (outside templates) is a candidate and all must have the value;
        a value Sema did not leave in the dump as a ConstantExpr is asked of Clang by a probe."""
        name = comps[-1][0]
        vals = set()
        for v in self.var_by_name.get(name, []):
            if self.is_pattern(v) or self.is_local(v):
                continue
            ks = [c for c in v.get('inner', []) if not c.get('kind', '').endswith('Attr')]
            if ks and ks[0].get('kind') == 'ConstantExpr' and re.match(r'-?\d+$', str(ks[0].get('value'))):
                vals.add(int(ks[0]['value']))
            else:
                vals.add(self.probe_value('(%s)' % self.spell_decl(v)))
        if len(vals) != 1:
            stop('type string not read: %r (the template argument %s names %d constants of different values)'
                 % (whole, name, len(vals)))
        return vals.pop()

    # ------------------------------------------------------------------ probes
    # What the dumps do not say and Clang knows is asked of Clang: a second translation unit
    # that includes this one and declares one alias, whose canonical type the dump then prints.
    PROBE = ('template <class T> struct __c2c_canon;\ntemplate <auto V> struct __c2c_val;\n'
             'using __c2c_probe = %s;\n')

    def probe(self, rhs):
        if rhs in self.probes:
            return self.probes[rhs]
        p = subprocess.run(['clang++'] + self.flags + ['-fsyntax-only', '-Xclang', '-fno-access-control', '-Xclang',
                            '-ast-dump=json', '-Xclang', '-ast-dump-filter=__c2c_probe', '-include', self.path,
                            '-x', 'c++', '-'], input=self.PROBE % rhs, capture_output=True, text=True)
        if p.returncode != 0 or '{' not in p.stdout:
            stop('a probe of Clang failed: `using __c2c_probe = %s;` after %s:\n%s' % (rhs, self.path, p.stderr[-1500:]))
        obj, _ = json.JSONDecoder().raw_decode(p.stdout[p.stdout.index('{'):])
        if obj.get('name') != '__c2c_probe':
            stop('a probe of Clang printed something else: %r' % obj.get('name'))
        t = obj['type'].get('desugaredQualType', obj['type']['qualType'])
        m = re.match(r'__c2c_(?:canon|val)<(.*)>$', t)
        if not m:
            stop('a probe of Clang answered %r' % t)
        self.probes[rhs] = m.group(1)
        return m.group(1)

    def probe_type(self, cxx_type):
        return TypeParser(self, self.probe('__c2c_canon<%s>' % cxx_type), self.tu, None).whole()

    def probe_value(self, cxx_expr):
        a = self.probe('__c2c_val<%s>' % cxx_expr)
        m = re.match(r'(-?\d+)[uUlL]*$', a)
        if m:
            return int(m.group(1))
        if a in ('true', 'false'):
            return int(a == 'true')
        stop('a probe of Clang for a value answered %r' % a)

    def spell_scope(self, n):
        """The C++ spelling, from the global namespace, of the context the declaration n is a
        member of ('' for the global namespace), or None when it cannot be named from outside."""
        sem = n.get('parentDeclContextId')
        p = self.nodes[sem] if sem in self.nodes else self.parent[id(n)]
        while p is not None and p.get('kind') in ('LinkageSpecDecl', 'FriendDecl', 'ClassTemplateDecl', 'FunctionTemplateDecl',
                                                  'EnumDecl') and not (p.get('kind') == 'EnumDecl' and 'scopedEnumTag' in p):
            p = self.parent[id(p)]
        if p is None or p.get('kind') == 'TranslationUnitDecl':
            return ''
        k = p.get('kind')
        outer = self.spell_scope(p)
        if outer is None:
            return None
        if k == 'NamespaceDecl':
            return outer if 'name' not in p else outer + '::' + p['name']
        if k in RECORD_KINDS + ('EnumDecl',) and p.get('name'):
            return outer + '::' + p['name'] + (self.spell_args(p) if k == 'ClassTemplateSpecializationDecl' else '')
        return None

    def spell_args(self, n):
        if not any(c.get('kind') == 'TemplateArgument' for c in n.get('inner', [])):
            return ''
        out = []
        types = iter([a for a in self.flat_targs(n) if 'type' in a])
        for kind, x in self.template_args_or_stop(n):
            if kind == 'type':
                out.append(next(types)['type']['qualType'])     # Clang's canonical spelling
            elif kind == 'int':
                out.append(str(x))
            else:
                stop('a template argument of %s printed as %d whose type is not read: it cannot be spelled for a probe' % (n.get('name'), x))
        return '<%s>' % ', '.join(out)

    def flat_targs(self, n):
        out = []

        def one(a):
            if a.get('isPack'):
                for b in a.get('inner', []):
                    one(b)
            else:
                out.append(a)
        for c in n.get('inner', []):
            if c.get('kind') == 'TemplateArgument':
                one(c)
        return out

    def spell_decl(self, n):
        sc = self.spell_scope(n)
        if sc is None or not n.get('name'):
            stop('%s (at %s) cannot be named from outside its scope, so Clang cannot be asked about it by a probe: no rule yet'
                 % (n.get('name') or 'an unnamed declaration', self.where(n)))
        return sc + '::' + n['name'] + (self.spell_args(n) if n['kind'] in FUNCTION_KINDS + ('ClassTemplateSpecializationDecl',) else '')

    def function_type(self, d):
        """The type of the function d. Its string is read in d's scope; when that fails (a part
        only Clang can evaluate, a name printed as written elsewhere) Clang is asked for the type
        of `&d`."""
        if d['id'] in self.fn_type_memo:
            return self.fn_type_memo[d['id']]
        try:
            t = self.resolve_type(d['type'], d)
        except Stop as why:
            # the string has a part only Clang can evaluate, or a name that the place of the
            # declaration does not determine (sugar printed as written somewhere else)
            if d['kind'] in ('CXXConstructorDecl', 'CXXDestructorDecl'):
                # its address cannot be taken to ask Clang; its parameters are in its mangled
                # name, its result is void, and what is left is the exception specification
                q = d['type']['qualType']
                tail = q[q.rindex(')') + 1:].strip() if ')' in q else '?'
                if tail not in ('', 'noexcept', 'noexcept(false)', 'noexcept(true)'):
                    stop('the type of %s (at %s) is not read from its string (%s), and its exception specification is %r'
                         % (d.get('name'), self.where(d), why, tail))
                t = ('func', ('builtin', 'void'), tuple(self.mangled_params(d)), False, False, False, 'none',
                     tail in ('noexcept', 'noexcept(true)'))
                self.fn_type_memo[d['id']] = t
                return t
            t = self.probe_type('decltype(&%s)' % self.spell_decl(d))
            if t[0] == 'ptr' and t[1][0] == 'func':
                t = t[1]
            elif t[0] == 'memptr' and t[2][0] == 'func':
                t = t[2]
            else:
                stop('a probe for the type of %s answered %s' % (d.get('name'), type_str(t)))
        self.fn_type_memo[d['id']] = t
        return t

    def mangled_params(self, d):
        """The parameter types of the function d from its mangled name, demangled by c++filt:
        canonical and fully qualified, whatever sugar the dump printed. (A parameter's own
        top-level const is not part of a function's signature and is not there.)"""
        if d['id'] in self.mangled_memo:
            return self.mangled_memo[d['id']]
        if not d.get('mangledName'):
            stop('%s (at %s) has no mangled name to read its parameter types from' % (d.get('name'), self.where(d)))
        p = subprocess.run(['c++filt', d['mangledName']], capture_output=True, text=True)
        dem = p.stdout.strip()
        if p.returncode != 0 or dem == d['mangledName']:
            stop('c++filt did not demangle %s' % d['mangledName'])
        body = re.sub(r'(\s*(const|volatile|&&|&))*$', '', dem)
        body = re.sub(r'\s*\[clone [^\]]*\]$', '', body)
        if not body.endswith(')'):
            stop('demangled name not read: %r' % dem)
        depth, i = 0, len(body) - 1
        while i >= 0:
            if body[i] == ')':
                depth += 1
            elif body[i] == '(':
                depth -= 1
                if depth == 0:
                    break
            i -= 1
        inner, parts, depth, cur = body[i + 1:-1], [], 0, ''
        for ch in inner:
            if ch in '<([':
                depth += 1
            elif ch in '>)]':
                depth -= 1
            if ch == ',' and depth == 0:
                parts.append(cur)
                cur = ''
            else:
                cur += ch
        if cur.strip():
            parts.append(cur)
        variadic = bool(parts) and parts[-1].strip() == '...'
        out = [TypeParser(self, x.strip(), self.tu, None).whole() for x in (parts[:-1] if variadic else parts)]
        self.mangled_memo[d['id']] = out
        self.from_mangled.add(dem)
        return out

    def is_pattern(self, e):
        """Is the declaration inside a template that is not instantiated (or a partial
        specialization)? An instantiation is listed under its template; that is not inside."""
        if e.get('kind') == 'ClassTemplatePartialSpecializationDecl':
            return True
        prev, n = e, self.parent[id(e)]
        while n is not None:
            if n.get('kind') in TEMPLATE_KINDS:
                inst = prev.get('kind') == 'ClassTemplateSpecializationDecl' or (
                    prev.get('kind') in FUNCTION_KINDS and any(c.get('kind') == 'TemplateArgument' for c in prev.get('inner', [])))
                if not inst and not (prev is e and e.get('kind') in ('TypeAliasDecl',) and False):
                    return True
            prev, n = n, self.parent[id(n)]
        return False

    def note_scope_dependence(self, ident):
        """Record the identifiers that name different types in different scopes of the unit:
        for these the answer rests on this file's lookup following C++'s from the place of use."""
        if ident in self.global_memo:
            return
        self.global_memo[ident] = None
        types = set()
        for e in self.by_name.get(ident, []):
            if self.is_pattern(e):
                continue
            key = self.entity_key(e)
            try:
                types.add(key if key[0] == 'ns' or self.nodes[key[1]]['kind'] in ('ClassTemplateDecl', 'TypeAliasTemplateDecl')
                          else self.entity_type(key, ident))
            except (Dependent, Stop) as x:
                types.add(('unread', str(x)))
        if len(types) > 1:
            self.scope_dependent.add(ident)

    def resolve_from(self, ent, comps, scope, whole, entity_given, env=None):
        """ent is what the first component names (or a namespace or type to look the components
        up in, entity_given False)."""
        i = 0
        cur = ent
        if entity_given:
            cur = self.apply_args(cur, comps[0][1], scope, whole)
            i = 1
        while i < len(comps):
            ident, args = comps[i]
            if cur[0] == 'ns' and ident == '(anonymous namespace)':
                cur = ('ns', (cur[1] + '::' if cur[1] else '') + ident)
                i += 1
                continue
            if cur[0] == 'ns':
                found = self.lookup_ctx(cur, ident)
            else:
                t = self.entity_type(cur, whole)
                if t[0] != 'record':
                    stop('type string not read: %r (%r is not a class or namespace)' % (whole, comps[i - 1][0]))
                found = self.lookup_ctx(('decl', self.nodes[t[1]]), ident)
            keys = sorted(set(self.entity_key(e) for e in found))
            if len(keys) != 1:
                if not keys:
                    stop('type string not read: %r (no type member %r)' % (whole, ident))
                stop('type string not read: %r (%r names %d things)' % (whole, ident, len(keys)))
            cur = self.apply_args(keys[0], args, scope, whole)
            i += 1
        return self.entity_type(cur, whole)

    def apply_args(self, ent, args, scope, whole):
        if args is None:
            return ent
        if ent[0] != 'decl':
            stop('type string not read: %r (template arguments on a namespace or a type)' % whole)
        n = self.nodes[ent[1]]
        if n['kind'] == 'TypeAliasTemplateDecl':
            return ('type', self.alias_template(n, args, whole))
        if n['kind'] == 'ClassTemplateSpecializationDecl':
            # the injected class name with arguments names the template
            t = self.parent[id(n)]
            if t.get('kind') != 'ClassTemplateDecl':
                cands = [x for x in self.by_name.get(n.get('name'), []) if x.get('kind') == 'ClassTemplateDecl' and self.same_scope(x, n)]
                if len(cands) < 1:
                    stop('type string not read: %r (the template of the specialization %s is not found)' % (whole, n.get('name')))
                t = cands[0]
            n = t
        if n['kind'] != 'ClassTemplateDecl':
            stop('type string not read: %r (%s is a %s, not a class template)' % (whole, n.get('name'), n['kind']))
        return ('decl', self.specialization(n, args, whole)['id'])

    def alias_template(self, n, args, whole):
        """An alias template applied: its type, with the arguments for its parameters."""
        key = (n['id'], tuple(args))
        if key in self.spec_memo:
            return self.spec_memo[key]
        params = [c for c in n.get('inner', []) if c.get('kind', '').endswith('ParmDecl')]
        alias = [c for c in n.get('inner', []) if c.get('kind') == 'TypeAliasDecl']
        if len(alias) != 1 or any(c.get('isParameterPack') or c['kind'] == 'TemplateTemplateParmDecl' for c in params):
            stop('type string not read: %r (the alias template %s has a parameter pack or a template parameter: no rule yet)'
                 % (whole, n.get('name')))
        if len(params) != len(args):
            stop('type string not read: %r (the alias template %s takes %d arguments, %d are printed: defaults, no rule yet)'
                 % (whole, n.get('name'), len(params), len(args)))
        env = {}
        for c, (kind, x) in zip(params, args):
            if (c['kind'] == 'TemplateTypeParmDecl') != (kind == 'type'):
                stop('type string not read: %r (argument kinds of the alias template %s)' % (whole, n.get('name')))
            env[c.get('name')] = (kind, x)
        t = TypeParser(self, alias[0]['type']['qualType'], alias[0], None, env).whole()
        self.spec_memo[key] = t
        return t

    def entity_type(self, ent, whole):
        if ent[0] == 'type':
            return ent[1]
        if ent[0] == 'ns':
            stop('type string not read: %r (a namespace where a type is expected)' % whole)
        n = self.nodes[ent[1]]
        k = n['kind']
        if k in RECORD_KINDS:
            return ('record', n['id'])
        if k == 'EnumDecl':
            return ('enum', n['id'])
        if k in ('TypedefDecl', 'TypeAliasDecl'):
            return self.typedef_type(n)
        stop('type string not read: %r (the %s %s where a type is expected)' % (whole, k, n.get('name')))

    def typedef_type(self, n):
        i = n['id']
        if i not in self.typedef_memo:
            self.typedef_memo[i] = None
            self.typedef_memo[i] = self.resolve_type(n['type'], n)
        if self.typedef_memo[i] is None:
            stop('a typedef defined through itself: %s' % n.get('name'))
        return self.typedef_memo[i]

    def lambda_record(self, text, scope):
        cands = self.lambdas.get(text, [])
        if not cands:
            stop('a lambda type with no LambdaExpr in the unit: %s' % text)
        if len(cands) == 1:
            return cands[0]
        # the same lambda in a template and in its instantiations: the one in the function
        # (or the instantiation) the type is used in
        anc = {}
        for c in cands:
            n, d = c, 0
            while n is not None:
                anc.setdefault(id(n), []).append((d, c))
                n, d = self.parent[id(n)], d + 1
        n = scope
        while n is not None:
            if id(n) in anc:
                best = sorted(anc[id(n)], key=lambda p: p[0])
                if len(best) > 1 and best[0][0] == best[1][0] or n.get('kind') in ('TranslationUnitDecl', 'NamespaceDecl') and len(best) > 1:
                    stop('a lambda type that several LambdaExprs share, used outside them: %s at %s' % (text, self.where(scope)))
                return best[0][1]
            n = self.parent[id(n)]
        stop('lambda not found: %s' % text)

    def template_args(self, spec):
        """The arguments of a specialization: ('type', T) / ('int', n), packs flattened.

        The JSON prints an integral argument sign-extended from its own width: `true` is -1,
        and std::size_t's maximum is -1. The value is read at the type of the template's
        parameter (found with the earlier arguments bound). Where that type cannot be read and
        the number printed is negative the argument is ('int?', n): it matches the values it
        could stand for, and stops the import if it has to be written."""
        raw = []

        def one(a):
            if a.get('isPack'):
                for b in a.get('inner', []):
                    one(b)
            elif 'type' in a:
                raw.append(('type', self.resolve_type(a['type'], self.tu)))
            elif 'value' in a:
                raw.append(('int', int(a['value'])))
            else:
                raise Dependent('a template argument that is neither a type nor an integer')
        for c in spec.get('inner', []):
            if c.get('kind') == 'TemplateArgument':
                one(c)
        params = self.template_params(spec)
        env, out = {}, []
        for i, (kind, x) in enumerate(raw):
            p = params[min(i, len(params) - 1)] if params else None
            if kind == 'int' and x < 0:
                pt = None
                if p is not None and p.get('kind') == 'NonTypeTemplateParmDecl':
                    try:
                        pt = unqual(TypeParser(self, p['type'].get('desugaredQualType', p['type']['qualType']), p, None, env).whole())
                    except (Stop, Dependent):
                        pt = None
                if pt is not None and pt[0] == 'enum' and 'fixedUnderlyingType' in self.nodes[pt[1]]:
                    pt = unqual(self.resolve_type(self.nodes[pt[1]]['fixedUnderlyingType'], self.nodes[pt[1]]))
                if pt is not None and pt[0] == 'builtin' and pt[1] in UNSIGNED_BITS and self.target.startswith('x86_64'):
                    x = x % (1 << UNSIGNED_BITS[pt[1]])
                elif not (pt is not None and pt[0] == 'builtin' and pt[1] in SIGNED_KINDS):
                    kind = 'int?'
            out.append((kind, x))
            if p is not None and p.get('name') and not p.get('isParameterPack') and kind != 'int?':
                env[p['name']] = (kind, x)
        return tuple(out)

    def template_params(self, spec):
        """The parameters of the template a specialization is of, or None when not found."""
        t = self.parent[id(spec)]
        if t.get('kind') not in ('ClassTemplateDecl', 'FunctionTemplateDecl', 'VarTemplateDecl'):
            cands = [x for x in self.by_name.get(spec.get('name'), []) if x.get('kind') == 'ClassTemplateDecl' and self.same_scope(x, spec)]
            if not cands:
                return None
            t = cands[0]
        return [c for c in t.get('inner', []) if c.get('kind', '').endswith('ParmDecl')]

    @staticmethod
    def args_match(have, want):
        """Do a specialization's arguments equal the wanted ones (the first len(want) of them)?"""
        if len(have) < len(want):
            return False
        for (hk, hx), (wk, wx) in zip(have, want):
            if hk == 'int?':
                if wk != 'int' or wx not in (hx, hx % (1 << 64), hx % (1 << 32), hx % (1 << 16), hx % (1 << 8), 1 if hx == -1 else hx):
                    return False
            elif (hk, hx) != (wk, wx):
                return False
        return True

    def specialization(self, tmpl, args, whole):
        key = (tmpl['id'], tuple(args))
        if key in self.spec_memo:
            return self.spec_memo[key]
        root = tmpl['id']
        chain = [n for n in self.nodes.values() if n.get('kind') == 'ClassTemplateDecl' and
                 self.redecl_root(n['id']) == self.redecl_root(root)]
        # arguments left out are the parameters' defaults (written on any declaration of the
        # template), read with the arguments before them bound; a default that is not a type
        # leaves the list short, and then the specialization must be the only one it begins
        args = list(args)
        params = [c for c in tmpl.get('inner', []) if c.get('kind', '').endswith('ParmDecl')]
        if len(args) < len(params) and not any(c.get('isParameterPack') for c in params):
            env = {c.get('name'): a for c, a in zip(params, args) if c.get('name') and a[0] != 'int?'}
            for i in range(len(args), len(params)):
                d = None
                for t in chain:
                    ps = [c for c in t.get('inner', []) if c.get('kind', '').endswith('ParmDecl')]
                    if i < len(ps) and 'defaultArg' in ps[i]:
                        d, scope = ps[i]['defaultArg'], ps[i]
                if d is None or 'type' not in d:
                    break
                try:
                    v = ('type', TypeParser(self, d['type']['qualType'], scope, None, env).whole())
                except (Stop, Dependent):
                    break
                args.append(v)
                if params[i].get('name'):
                    env[params[i]['name']] = v
        exact, prefix = [], []
        for t in chain:
            for s in t.get('inner', []):
                if s.get('kind') != 'ClassTemplateSpecializationDecl':
                    continue
                s = self.nodes[s['id']]
                try:
                    sa = self.template_args(s)
                except Dependent:
                    continue
                s = self.node(s['id'])
                if len(sa) == len(args) and self.args_match(sa, args):
                    exact.append(s)
                elif self.args_match(sa, args):
                    prefix.append(s)
        # explicit specializations are declared beside the template, not under it
        for s in self.by_name.get(tmpl['name'], []):
            if s.get('kind') == 'ClassTemplateSpecializationDecl' and self.parent[id(s)].get('kind') != 'ClassTemplateDecl':
                try:
                    sa = self.template_args(s)
                except Dependent:
                    continue
                if len(sa) == len(args) and self.args_match(sa, args) and self.same_scope(s, tmpl):
                    exact.append(self.node(s['id']))
        cands = {s['id']: s for s in (exact or prefix)}
        if len(cands) != 1:
            if not cands:
                stop('type string not read: %r (no specialization of %s with these arguments is in the unit)' % (whole, tmpl.get('name')))
            stop('type string not read: %r (%d specializations of %s share the arguments printed; the defaults are not printed)'
                 % (whole, len(cands), tmpl.get('name')))
        r = list(cands.values())[0]
        self.spec_memo[key] = r
        return r

    def redecl_root(self, i):
        while i in self.redecl and self.redecl[i] in self.nodes:
            i = self.redecl[i]
        return i

    def same_scope(self, a, b):
        return self.qualified_name(self.parent[id(a)]) == self.qualified_name(self.parent[id(b)])

    # ------------------------------------------------------------------ types
    def resolve_type(self, tobj, scope):
        """A JSON type object, used at the node `scope` -> the structured type."""
        hint_id = tobj.get('typeAliasDeclId')
        if hint_id is not None:
            s = tobj['qualType']
        else:
            s = tobj.get('desugaredQualType', tobj['qualType'])
        if tobj.get('desugaredQualType') == 'std::nullptr_t':
            return ('builtin', 'nullptr')       # how Clang prints the builtin type of nullptr
        if s in PLACEHOLDERS:
            return ('builtin', PLACEHOLDERS[s])
        if s.startswith('<') or s == 'NULL TYPE':
            raise Dependent('the type %r' % s)
        ctx = scope
        while ctx is not None and ctx.get('kind') not in SCOPE_KINDS:
            ctx = self.parent[id(ctx)]
        key = (s, hint_id, id(ctx))
        if key not in self.type_memo:
            hint = self.nodes.get(hint_id) if hint_id else None
            if hint is not None and hint.get('kind') not in ('TypedefDecl', 'TypeAliasDecl'):
                hint = None
            self.type_memo[key] = TypeParser(self, s, scope, hint).whole()
        return self.type_memo[key]

    # ------------------------------------------------------------------ layouts
    def read_layouts(self):
        """Blocks of the record-layout dump, by the record's printed name."""
        self.layout_blocks = {}
        cur = None
        for line in self.layout_text.splitlines():
            if line.startswith('*** Dumping AST Record Layout'):
                cur = []
                continue
            if cur is None or '|' not in line:
                continue
            left, right = line.split('|', 1)
            if right.lstrip().startswith('[sizeof=') or (cur and cur[-1][0] == 'tail'):
                if cur and cur[-1][0] == 'tail':
                    cur[-1] = ('tail', cur[-1][1] + right)
                else:
                    cur.append(('tail', right))
                if ']' in right:
                    head = cur[0][2]
                    name = re.sub(r'^(struct|class|union) ', '', head)
                    name = re.sub(r' \(empty\)$', '', name)
                    self.layout_blocks.setdefault(name, []).append(cur)
                    cur = None
                continue
            depth = (len(right) - len(right.lstrip(' ')) - 1) // 2
            cur.append((left.strip(), depth, right.strip()))
        self.layout_memo = {}

    def layout_of(self, rec):
        """(sizes, vptr offset, [base (record id, offset, primary)], [field offsets in bits],
        [virtual base (record id, offset, primary)])."""
        if rec['id'] in self.layout_memo:
            return self.layout_memo[rec['id']]
        if self.is_lambda(rec):
            name = [k for k, v in self.lambdas.items() if any(x is rec for x in v)][0]
            blocks = self.layout_blocks.get(name, [])
        else:
            blocks = self.blocks_of(self.layout_blocks, 'layout').get(rec['id'], [])
        if not blocks:
            stop('no layout in the record-layout dump for %s (declared at %s)'
                 % (self.qualified_name(rec) or rec.get('name'), self.where(rec)))
        if any(b != blocks[0] for b in blocks[1:]):
            stop('several different layouts are printed under the name of %s' % rec.get('name'))
        b = blocks[0]
        tail = b[-1][1]
        sizes = {}
        for k in ('sizeof', 'dsize', 'align', 'nvsize', 'nvalign'):
            m = re.search(r'\b%s=(\d+)' % k, tail)
            if not m:
                stop('record layout of %s: no %s in %r' % (rec.get('name'), k, tail))
            sizes[k] = int(m.group(1))
        vptr, bases, fields, vbases = None, [], [], []
        for off, depth, text in b[1:-1]:
            if depth != 1:
                continue
            if text.endswith('vtable pointer)'):
                vptr = int(off)
            elif re.search(r' \((primary )?(virtual )?base\)( \(empty\))?$', text):
                nm = re.sub(r' \((primary )?(virtual )?base\)( \(empty\))?$', '', text)
                nm = re.sub(r'^(struct|class|union) ', '', nm)
                bt = TypeParser(self, nm, self.tu, None).whole()
                if 'virtual base)' in text:
                    vbases.append((bt[1], int(off), 'primary' in text))
                else:
                    bases.append((bt[1], int(off), '(primary base)' in text))
            elif text.endswith('vbtable pointer)'):
                stop('record layout of %s: %r: no rule yet' % (rec.get('name'), text))
            else:
                if off.endswith(':-'):
                    continue            # a zero-width bit-field: it only aligns what follows
                if ':' in off:
                    byte, bits = off.split(':')
                    fields.append((int(byte) * 8 + int(bits.split('-')[0]), text))
                else:
                    fields.append((int(off) * 8, text))
        r = (sizes, vptr, bases, fields, vbases)
        self.layout_memo[rec['id']] = r
        return r

    def blocks_of(self, by_name, what):
        """The blocks of a dump by the class each is headed with. A heading that does not read
        as a class of the unit (a template printed with its parameters) heads no class; a class
        that is written and finds no block stops the import where it is needed."""
        memo = self.__dict__.setdefault('_blocks_' + what, None)
        if memo is None:
            memo = {}
            for name, bs in by_name.items():
                try:
                    t = TypeParser(self, name, self.tu, None).whole()
                except (Dependent, Stop):
                    continue
                if t[0] == 'record':
                    memo.setdefault(t[1], []).extend(bs)
            self.__dict__['_blocks_' + what] = memo
        return memo

    def is_lambda(self, rec):
        return bool(rec.get('definitionData', {}).get('isLambda'))

    def read_vtables(self):
        """The vtable dump, by printed class name: {'main': entry lines, 'vbo': virtual base
        offset offset lines, 'ctor': [(base name, offset, entry lines)]}; and from the IR of the
        same run, which vtables the unit emits and the VTTs (the dump has no VTT)."""
        self.vtable_blocks = {}
        cur = None
        for line in self.vtable_text.splitlines():
            m = re.match(r"Vtable for '(.*)' \((\d+) entr", line)
            m2 = re.match(r"Construction vtable for \('(.*)', (\d+)\) in '(.*)' \((\d+) entr", line)
            m3 = re.match(r"Virtual base offset offsets for '(.*)' \((\d+) entr", line)
            if m or m2 or m3:
                name = (m or m3).group(1) if not m2 else m2.group(3)
                blk = self.vtable_blocks.setdefault(name, {'main': None, 'vbo': [], 'ctor': []})
                cur = []
                if m:
                    if blk['main'] is not None:
                        cur = blk['dup'] = []
                    else:
                        blk['main'] = cur
                elif m3:
                    blk['vbo'] = cur
                else:
                    blk['ctor'].append((m2.group(1), int(m2.group(2)), cur))
                continue
            if cur is None:
                continue
            if not line.strip() or re.match(r'(VTable indices|Thunks for|\*\*\*)', line):
                cur = None
                continue
            cur.append(line)
        for name, blk in self.vtable_blocks.items():
            if 'dup' in blk and blk['dup'] != blk['main']:
                stop('several different vtables are printed under the name %s' % name)
        self.vtable_memo = {}
        # the IR: @_ZTV… (vtables), @_ZTT… (VTTs), @_ZTC… (construction vtables)
        self.ir_vtables, self.ir_vtts = {}, {}
        syms = {}
        for line in self.ir_globals:
            m = re.match(r'@(_ZT[VTC]\S+) = (.*)$', line)
            if m:
                syms[m.group(1)] = m.group(2)
        if syms:
            names = list(syms)
            p = subprocess.run(['c++filt'] + names, capture_output=True, text=True)
            dem = p.stdout.splitlines()
            if p.returncode != 0 or len(dem) != len(names):
                stop('c++filt did not demangle the vtable symbols of the unit')
            self.ir_demangled = dict(zip(names, dem))
            for sym, d in self.ir_demangled.items():
                if d.startswith('vtable for '):
                    self.ir_vtables[d[len('vtable for '):]] = (sym, not syms[sym].startswith('external '))
                elif d.startswith('VTT for '):
                    self.ir_vtts[d[len('VTT for '):]] = syms[sym]
        else:
            self.ir_demangled = {}

    def class_named(self, name):
        try:
            t = TypeParser(self, name, self.tu, None).whole()
        except (Dependent, Stop):
            return None
        return t[1] if t[0] == 'record' else None

    def ir_of(self, table, rec):
        """The entry of an IR table (by demangled class name) for the class rec."""
        memo = self.__dict__.setdefault('_ir_memo', {})
        if id(table) not in memo:
            memo[id(table)] = {}
            for name, v in table.items():
                rid = self.class_named(name)
                if rid is not None:
                    memo[id(table)][rid] = v
        return memo[id(table)].get(rec['id'])

    def vtable_entries(self, lines, rec):
        """Entry lines -> ([(base record id, offset, index)], [entry])."""
        methods = self.virtual_candidates(rec)
        entries, points = [], []
        for line in lines:
            s = line.strip()
            m = re.match(r'-- \((.*), (\d+)\) vtable address --$', s)
            if m:
                rid = self.class_named(m.group(1))
                if rid is None:
                    stop('vtable of %s: the address point %r names no class of the unit' % (rec.get('name'), s))
                points.append((rid, int(m.group(2)), len(entries)))
                continue
            m = re.match(r'\[(this|return) adjustment: (-?\d+) non-virtual(?:, (-?\d+) (vcall offset offset|vbase offset offset))?\]$', s)
            if m:
                if not entries or entries[-1][0] in ('offset-to-top', 'rtti', 'vbase-offset', 'vcall-offset'):
                    stop('vtable of %s: an adjustment with no function before it: %r' % (rec.get('name'), s))
                e = entries[-1]
                adj = dict(e[2]) if e[0] == 'thunk' else {}
                inner = e[1] if e[0] == 'thunk' else e
                adj[m.group(1)] = (int(m.group(2)), int(m.group(3)) if m.group(3) is not None else None)
                if m.group(3) is not None and (m.group(1) == 'this') != (m.group(4) == 'vcall offset offset'):
                    stop('vtable of %s: adjustment not read: %r' % (rec.get('name'), s))
                entries[-1] = ('thunk', inner, adj)
                continue
            m = re.match(r'(\d+) \| (.*)$', s)
            if not m:
                stop('vtable of %s: line not read: %r' % (rec.get('name'), line))
            if int(m.group(1)) != len(entries):
                stop('vtable of %s: entries out of order at %r' % (rec.get('name'), line))
            e = m.group(2)
            m2 = re.match(r'(offset_to_top|vbase_offset|vcall_offset) \((-?\d+)\)$', e)
            if m2:
                entries.append((m2.group(1).replace('_', '-'), int(m2.group(2))))
            elif e.endswith(' RTTI'):
                entries.append(('rtti', None))
            else:
                flags = re.findall(r' \[([a-z ]+)\]', e)
                sig = re.sub(r' \[[a-z ]+\]', '', e)
                meth = self.match_method(sig, methods, rec)
                kind = 'method'
                for f in flags:
                    if f == 'pure':
                        kind = 'pure'
                    elif f == 'complete':
                        kind = 'complete-dtor'
                    elif f == 'deleting':
                        kind = 'deleting-dtor'
                    else:
                        stop('vtable of %s: entry flag %r: no rule yet' % (rec.get('name'), f))
                entries.append((kind, meth))
        return points, entries

    def vtable_of(self, rec):
        """None, or {'points', 'entries', 'vbo': [(vbase id, at)], 'ctor': [(base id, offset,
        points, entries)], 'emitted': bool, 'vtt': [(None | (base id, offset), index)]}."""
        if rec['id'] in self.vtable_memo:
            return self.vtable_memo[rec['id']]
        self.vtable_memo[rec['id']] = None
        blk = None
        for name, b in self.vtable_blocks.items():
            if b['main'] is not None and self.class_named(name) == rec['id']:
                if blk is not None and blk is not b:
                    stop('several vtables are printed for %s' % rec.get('name'))
                blk = b
        if blk is None:
            return None
        points, entries = self.vtable_entries(blk['main'], rec)
        vbo = []
        for line in blk['vbo']:
            m = re.match(r'\s*(.*) \| (-?\d+)$', line)
            rid = self.class_named(m.group(1)) if m else None
            if rid is None:
                stop('vtable of %s: virtual base offset offset line not read: %r' % (rec.get('name'), line))
            vbo.append((rid, int(m.group(2))))
        ctor = []
        for bname, off, lines in blk['ctor']:
            rid = self.class_named(bname)
            if rid is None:
                stop('vtable of %s: the construction vtable for %r names no class of the unit' % (rec.get('name'), bname))
            ps, es = self.vtable_entries(lines, self.nodes[rid])
            ctor.append((rid, off, ps, es))
        irv = self.ir_of(self.ir_vtables, rec)
        vtt = []
        irt = self.ir_of(self.ir_vtts, rec)
        if irt is not None:
            own = irv[0] if irv else None
            body = irt[irt.index('['):]
            elems = re.findall(r'ptr getelementptr inbounds \(\{ ([^}]*) \}, ptr @(\S+), i32 0, inrange i32 (\d+), i32 (\d+)\)', body)
            m = re.match(r'.*constant \[(\d+) x ptr\]', irt)
            if not m or int(m.group(1)) != len(elems):
                stop('the VTT of %s in the IR is not read: %r' % (rec.get('name'), irt[:200]))
            for arrays, sym, ai, ei in elems:
                sizes = [int(x) for x in re.findall(r'\[(\d+) x ptr\]', arrays)]
                flat = sum(sizes[:int(ai)]) + int(ei)
                if sym == own:
                    vtt.append((None, flat))
                else:
                    m = re.match(re.escape('_ZTC' + own[len('_ZTV'):]) + r'(\d+)_', sym) if own else None
                    d = self.ir_demangled.get(sym, '')
                    m2 = re.match(r'construction vtable for (.*)-in-', d)
                    rid = self.class_named(m2.group(1)) if m2 else None
                    if not m or rid is None:
                        stop('the VTT of %s refers to %s (%s), which is not read' % (rec.get('name'), sym, d))
                    vtt.append(((rid, int(m.group(1))), flat))
        r = {'points': points, 'entries': entries, 'vbo': vbo, 'ctor': ctor,
             'emitted': bool(irv and irv[1]), 'vtt': vtt}
        self.vtable_memo[rec['id']] = r
        return r

    def all_bases(self, rec):
        out = [rec]
        for b in rec.get('bases', []):
            bt = self.base_type(rec, b)
            out += self.all_bases(self.nodes[bt[1]])
        return out

    def virtual_candidates(self, rec):
        out = []
        for r in self.all_bases(rec):
            for c in r.get('inner', []):
                if c.get('kind') in ('CXXMethodDecl', 'CXXDestructorDecl', 'CXXConversionDecl'):
                    out.append((r, self.node(c['id'])))
        return out

    def match_method(self, sig, methods, rec):
        """A method as the vtable dump prints it -> its declaration."""
        ns = sig.replace(' ', '')
        hits = []
        for r, m in methods:
            qt = m['type']['qualType']
            cut = self.param_paren(qt)
            ret, rest = qt[:cut].strip(), qt[cut:]
            rest = re.sub(r'\s*(noexcept(\(.*\))?|throw\(\))$', '', rest)     # the dump prints no exception specification
            if m['kind'] == 'CXXDestructorDecl':
                ret = ''
            head, tail = ret.replace(' ', ''), ('::' + m['name'] + rest).replace(' ', '')
            if not (ns.startswith(head) and ns.endswith(tail) and len(ns) > len(head) + len(tail)):
                continue
            cls = ns[len(head):len(ns) - len(tail)]
            try:
                ct = TypeParser(self, cls, self.tu, None).whole()
            except (Dependent, Stop):
                continue
            if ct == ('record', r['id']):
                hits.append(m)
        ids = set(h['id'] for h in hits)
        if len(ids) != 1:
            stop('vtable of %s: the entry %r matches %d methods' % (rec.get('name'), sig, len(ids)))
        return hits[0]

    @staticmethod
    def param_paren(qt):
        """Index of the parenthesis opening the parameter list of a function type string."""
        depth = 0
        for i, ch in enumerate(qt):
            if ch == '<':
                depth += 1
            elif ch == '>':
                depth -= 1
            elif ch == '(' and depth == 0 and not qt.startswith(SPECIAL, i):
                if qt[i + 1] in '*&':
                    stop('function type string not read: %r' % qt)
                return i
        stop('function type string not read: %r' % qt)

    # ------------------------------------------------------------------ ids and the worklist
    def start(self):
        self.ids = {}            # canonical Clang id -> Word
        self.defined = set()     # Words given a declaration in the output
        self.queue = []
        self.decls = []          # (Word, term) in the order written
        self.labels = []         # a comment for each: kind, qualified name, type as Clang prints it
        self.types = {}          # structured type -> (Ref, term)
        self.type_defs = []
        self.nrecords = self.nlayouts = 0

    def idof(self, clang_id):
        c = self.canon.get(clang_id, clang_id)
        if c not in self.nodes:
            stop('a declaration id the dump does not contain: %s' % clang_id)
        if c not in self.ids:
            self.ids[c] = len(self.ids) + 1
        return self.ids[c]

    def need(self, clang_id):
        """The declaration is used: number it, and queue it if it is written at the top level."""
        n = self.node(clang_id)
        k = n['kind']
        fresh = n['id'] not in self.ids
        w = self.idof(n['id'])
        if not fresh:
            return w
        if k in RECORD_KINDS or k in FUNCTION_KINDS or k == 'EnumDecl':
            self.queue.append(n)
            if k in FUNCTION_KINDS and k != 'FunctionDecl':
                self.need(self.record_of_member(n)['id'])
        elif k in ('VarDecl', 'VarTemplateSpecializationDecl'):
            if self.is_local(n) or n.get('_init_capture'):
                pass
            else:
                self.queue.append(n)
        elif k == 'FieldDecl':
            self.need(self.record_of_member(n)['id'])
        elif k == 'EnumConstantDecl':
            self.need(self.parent[id(n)]['id'])
        elif k == 'ParmVarDecl':
            pass
        else:
            stop('a reference to a %s (%s, at %s): no rule yet' % (k, n.get('name'), self.where(n)))
        return w

    def is_local(self, n):
        p = self.parent[id(n)]
        return p.get('kind') in ('DeclStmt', 'CXXForRangeStmt', 'IfStmt', 'WhileStmt', 'SwitchStmt', 'ForStmt') or p.get('kind') in FUNCTION_KINDS

    def record_of_member(self, n):
        sem = n.get('parentDeclContextId')
        p = self.nodes[sem] if sem in self.nodes else self.parent[id(n)]
        while p is not None and p.get('kind') not in RECORD_KINDS:
            if p.get('kind') in ('FriendDecl',):
                stop('internal: a friend treated as a member: %s' % n.get('name'))
            p = self.parent[id(p)]
        if p is None:
            stop('a member outside any class: %s at %s' % (n.get('name'), self.where(n)))
        return self.node(p['id'])

    def count(self, kind):
        self.met[kind] = self.met.get(kind, 0) + 1

    def elide(self, kind, why):
        c, _ = self.elided.get(kind, (0, why))
        self.elided[kind] = (c + 1, why)

    # ------------------------------------------------------------------ types as terms
    def ty(self, t):
        """The structured type as a term: a reference to a definition shared by the unit."""
        if t in self.types:
            return self.types[t][0]
        k = t[0]
        if k == 'builtin':
            con = 'ctb-' + t[1]
            if con not in CTORS:
                stop('a builtin type no unit has met before: %s (add %s to CBuiltin)' % (t[1], con))
            term = T('ct-builtin', T(con))
        elif k == 'ptr':
            term = T('ct-pointer', self.ty(t[1]))
        elif k == 'lref':
            term = T('ct-lref', self.ty(t[1]))
        elif k == 'rref':
            term = T('ct-rref', self.ty(t[1]))
        elif k == 'array':
            term = T('ct-array', self.ty(t[1]), W(t[2]))
        elif k == 'record':
            term = T('ct-record', W(self.need(t[1])))
        elif k == 'enum':
            term = T('ct-enum', W(self.need(t[1])))
        elif k == 'func':
            term = T('ct-function', self.ty(t[1]), inlist('CType', [self.ty(p) for p in t[2]]),
                     t[3], t[4], t[5], T('crq-' + t[6]), t[7])
        elif k == 'memptr':
            term = T('ct-member-pointer', W(self.need(t[1])), self.ty(t[2]))
        elif k == 'qual':
            term = T('ct-qual', t[1], t[2], t[3], self.ty(t[4]))
        else:
            stop('internal: type %r' % (t,))
        ref = Ref('%s-t%d' % (self.name, len(self.types) + 1), 'CType')
        self.types[t] = (ref, term)
        self.type_defs.append((ref, term))
        return ref

    def type_at(self, tobj, scope):
        try:
            return self.resolve_type(tobj, scope)
        except Dependent as e:
            stop('a type that is not instantiated reached the output: %r at %s (%s)'
                 % (tobj.get('qualType'), self.where(scope), e))

    def expr_type(self, n):
        """The type of an expression or declaration node. Two forms are read from the
        declaration rather than from the string printed at the use: a reference to a function
        has the function's type, whose string was written in the function's scope (it may name
        member typedefs of its class that are not in scope at the use), and the decay of a
        function is the pointer to that."""
        k = n.get('kind')
        if k == 'DeclRefExpr':
            d = self.node(n['referencedDecl']['id'])
            if d['kind'] in FUNCTION_KINDS and n['type']['qualType'] not in PLACEHOLDERS:
                return self.function_type(d)
        if k == 'ImplicitCastExpr' and n.get('castKind') == 'FunctionToPointerDecay':
            return ('ptr', self.expr_type(n['inner'][0]))
        return self.type_at(n['type'], n)

    def tyof(self, n):
        return self.ty(self.expr_type(n))

    # ------------------------------------------------------------------ expressions
    def kids(self, n):
        return [c for c in n.get('inner', [])]

    def head(self, n):
        """The two fields every expression starts with."""
        vc = n.get('valueCategory')
        if vc not in ('prvalue', 'lvalue', 'xvalue'):
            stop('%s at %s: value category %r' % (n['kind'], self.where(n), vc))
        return self.tyof(n), T('vc-' + vc)

    def exprs(self, ns):
        return inlist('CExpr', [self.expr(c) for c in ns])

    def opt_expr(self, n):
        return self.expr(n) if n else T('ce-nil')

    def expr(self, n):
        k = n.get('kind')
        if not k:
            stop('an absent expression where one is required, under %s' % self.where(n))
        f = getattr(self, 'e_' + k, None)
        if f is None:
            stop('expression kind with no rule yet: %s (at %s)' % (k, self.where(n)))
        self.count(k)
        return f(n)

    def e_IntegerLiteral(self, n):
        return T('ce-integer-literal', *self.head(n), W(int(n['value'])))

    def e_CharacterLiteral(self, n):
        return T('ce-character-literal', *self.head(n), W(int(n['value'])))

    def e_CXXBoolLiteralExpr(self, n):
        return T('ce-bool-literal', *self.head(n), bool(n['value']))

    def e_CXXNullPtrLiteralExpr(self, n):
        return T('ce-nullptr-literal', *self.head(n))

    def e_FloatingLiteral(self, n):
        t = unqual(self.type_at(n['type'], n))
        formats = {'float': (24, 8, False), 'double': (53, 11, False)}
        if self.target.startswith('x86_64'):
            formats['ldouble'] = (64, 15, True)     # the x87 extended format, the integer bit explicit
        if t[0] != 'builtin' or t[1] not in formats:
            stop('floating literal of type %s for the target %s at %s: no rule yet' % (type_str(t), self.target, self.where(n)))
        return T('ce-floating-literal', *self.head(n), W(ieee_bits(n['value'], *formats[t[1]])))

    def e_StringLiteral(self, n):
        """A narrow string is its array's bytes. A wide or UTF string is its array's code units
        (the element type gives their width; the bytes of a unit are the target's)."""
        t = unqual(self.type_at(n['type'], n))
        v = n['value']
        m = re.match(r'(L|u8|u|U|)"(.*)"$', v, re.S)
        if not m or t[0] != 'array' or unqual(t[1])[0] != 'builtin':
            stop('string literal %r of type %s at %s: no rule yet' % (v, type_str(t), self.where(n)))
        elem = unqual(t[1])[1]
        want = {'': 'char', 'L': 'wchar', 'u8': 'char8', 'u': 'char16', 'U': 'char32'}[m.group(1)]
        if elem != want:
            stop('string literal %r with elements of type %s at %s' % (v, elem, self.where(n)))
        items = c_unescape(m.group(2))
        units = []
        for kind, x in items:
            if kind == 'raw' or elem in ('char32', 'wchar'):
                units.append(x)
            elif elem in ('char', 'char8'):
                units += list(chr(x).encode('utf-8'))
            else:
                units += list(struct.unpack('<%dH' % (len(chr(x).encode('utf-16-le')) // 2), chr(x).encode('utf-16-le')))
        if elem == 'wchar' and not self.target.startswith('x86_64') or len(units) > t[2]:
            stop('string literal %r at %s: longer than its array, or wchar_t on a target this rule does not know' % (v, self.where(n)))
        units += [0] * (t[2] - len(units))
        if elem == 'char':
            return T('ce-string-literal', *self.head(n), S(bytes(units)))
        return T('ce-string-literal-units', *self.head(n), [W(u) for u in units])

    def ref(self, clang_id):
        n = self.node(clang_id)
        if n['kind'] not in REFKINDS:
            stop('a reference to a %s (%s): no rule yet' % (n['kind'], n.get('name')))
        return T('crk-' + REFKINDS[n['kind']]), W(self.need(n['id']))

    def e_DeclRefExpr(self, n):
        return T('ce-declref', *self.head(n), *self.ref(n['referencedDecl']['id']))

    def e_MemberExpr(self, n):
        base, = self.kids(n)
        return T('ce-member', *self.head(n), self.expr(base), bool(n['isArrow']), n['_qualified'],
                 *self.ref(n['referencedMemberDecl']))

    def call(self, con, n):
        ks = self.kids(n)
        return T(con, *self.head(n), self.expr(ks[0]), self.exprs(ks[1:]))

    def e_CallExpr(self, n):
        return self.call('ce-call', n)

    def e_CXXMemberCallExpr(self, n):
        return self.call('ce-member-call', n)

    def e_CXXOperatorCallExpr(self, n):
        return self.call('ce-operator-call', n)

    def constructor_of(self, n):
        """The constructor a CXXConstructExpr calls: the JSON gives its type, not its id."""
        t = unqual(self.type_at(n['type'], n))
        while t[0] == 'array':
            t = unqual(t[1])
        if t[0] != 'record':
            stop('a constructor call of a non-class type at %s' % self.where(n))
        rec = self.nodes[t[1]]
        want = n['ctorType']['qualType']
        ctors = []
        for c in rec.get('inner', []):
            if c.get('kind') == 'CXXConstructorDecl':
                ctors.append(c)
            elif c.get('kind') == 'FunctionTemplateDecl':
                # a constructor template: its instantiations (the ones with arguments)
                ctors += [d for d in c.get('inner', []) if d.get('kind') == 'CXXConstructorDecl' and
                          any(x.get('kind') == 'TemplateArgument' for x in d.get('inner', []))]
        hits = [c for c in ctors if c['type']['qualType'] == want]
        if len(hits) != 1:
            # the same signature spelled through different sugar: compare the types themselves
            wt = self.type_at(n['ctorType'], rec)
            hits = [c for c in ctors if self.type_at(c['type'], c) == wt]
        if len(hits) != 1:
            stop('the constructor %r of %s called at %s is not found once among its constructors (%d)'
                 % (want, rec.get('name'), self.where(n), len(hits)))
        return hits[0]

    def try_type(self, tobj, scope):
        try:
            return self.resolve_type(tobj, scope)
        except Dependent:
            return None

    def construct(self, con, n):
        ctor = self.constructor_of(n)
        ck = n.get('constructionKind')
        kinds = {'complete': 'complete', 'non-virtual base': 'non-virtual-base', 'delegating': 'delegating',
                 'virtual base': 'virtual-base'}
        if ck not in kinds:
            stop('construction kind %r at %s: no rule yet' % (ck, self.where(n)))
        if n.get('initializer_list'):
            stop('std::initializer_list construction at %s: no rule yet' % self.where(n))
        return T(con, *self.head(n), W(self.need(ctor['id'])), T('cck-' + kinds[ck]),
                 bool(n.get('elidable')), bool(n.get('list')), bool(n.get('zeroing')),
                 self.exprs(self.kids(n)))

    def e_CXXConstructExpr(self, n):
        return self.construct('ce-construct', n)

    def e_CXXTemporaryObjectExpr(self, n):
        return self.construct('ce-temporary-object', n)

    def cast(self, n):
        con = CAST_NODES[n['kind']]
        ck = 'ck-' + n['castKind']
        if ck not in CTORS:
            stop('a cast kind no unit has met before: %s at %s (add %s to CCastKind)'
                 % (n['castKind'], self.where(n), ck))
        self.met_casts[n['castKind']] = self.met_casts.get(n['castKind'], 0) + 1
        sub, = self.kids(n)
        path = []
        if 'path' in n:
            # the path runs from the derived class to the base: the operand's class for a cast
            # to a base, the result's for a cast to a derived class
            down = n['castKind'].startswith('BaseToDerived')
            t = unqual(self.expr_type(n) if down else self.expr_type(sub))
            if t[0] in ('ptr', 'lref', 'rref'):
                t = unqual(t[1])
            for step in n['path']:
                if t[0] != 'record':
                    stop('a base path from a non-class at %s' % self.where(n))
                hits = []
                for b in self.nodes[t[1]].get('bases', []):
                    bt = self.base_type(self.nodes[t[1]], b)
                    if self.nodes[bt[1]].get('name') == step['name']:
                        hits.append(bt)
                if len(hits) != 1:
                    stop('base path step %r at %s matches %d bases' % (step['name'], self.where(n), len(hits)))
                t = hits[0]
                path.append(W(self.need(t[1])))
        if 'conversionFunc' in n:
            self.need(n['conversionFunc']['id'])
        return T(con, *self.head(n), T(ck), path, self.expr(sub))

    e_ImplicitCastExpr = e_CXXStaticCastExpr = e_CXXReinterpretCastExpr = cast
    e_CXXFunctionalCastExpr = e_CStyleCastExpr = e_CXXDynamicCastExpr = cast

    def e_UnaryOperator(self, n):
        key = (n['opcode'], bool(n.get('isPostfix')))
        if key not in UNOPS or 'uo-' + UNOPS[key] not in CTORS:
            stop('unary operator %r at %s: no rule yet' % (key, self.where(n)))
        sub, = self.kids(n)
        return T('ce-unary', *self.head(n), T('uo-' + UNOPS[key]), self.expr(sub))

    def binop(self, op, n):
        if op not in BINOPS or 'cbo-' + BINOPS[op] not in CTORS:
            stop('binary operator %r at %s: no rule yet' % (op, self.where(n)))
        return T('cbo-' + BINOPS[op])

    def e_BinaryOperator(self, n):
        l, r = self.kids(n)
        return T('ce-binary', *self.head(n), self.binop(n['opcode'], n), self.expr(l), self.expr(r))

    def e_CompoundAssignOperator(self, n):
        l, r = self.kids(n)
        op = n['opcode']
        if not op.endswith('='):
            stop('compound assignment %r' % op)
        return T('ce-compound-assign', *self.head(n), self.binop(op[:-1], n),
                 self.ty(self.type_at(n['computeLHSType'], n)),
                 self.ty(self.type_at(n['computeResultType'], n)), self.expr(l), self.expr(r))

    def e_ConditionalOperator(self, n):
        c, a, b = self.kids(n)
        return T('ce-conditional', *self.head(n), self.expr(c), self.expr(a), self.expr(b))

    def e_ParenExpr(self, n):
        sub, = self.kids(n)
        return T('ce-paren', *self.head(n), self.expr(sub))

    def e_ArraySubscriptExpr(self, n):
        b, i = self.kids(n)
        return T('ce-array-subscript', *self.head(n), self.expr(b), self.expr(i))

    def e_InitListExpr(self, n):
        field = None
        if 'field' in n:
            field = self.need(n['field']['id'])
        filler = T('ce-nil')
        if 'array_filler' in n:
            fl = [c for c in n['array_filler']]
            # the filler is printed first, then the explicit initializers
            filler = self.expr(fl[0])
            inits = fl[1:]
        else:
            inits = self.kids(n)
        return T('ce-init-list', *self.head(n), optid(field), filler, self.exprs(inits))

    def e_ImplicitValueInitExpr(self, n):
        return T('ce-implicit-value-init', *self.head(n))

    def e_CXXDefaultInitExpr(self, n):
        """The field whose default member initializer this is: the JSON does not say, the
        position does (the initializer of a constructor, or the i-th element of an aggregate's
        initializer list: bases first, then the fields in order, unnamed bit-fields skipped)."""
        p = self.parent[id(n)]
        if p.get('kind') == 'CXXCtorInitializer' and 'anyInit' in p:
            fid = p['anyInit']['id']
        elif p.get('kind') == 'InitListExpr':
            t = unqual(self.type_at(p['type'], p))
            if t[0] != 'record':
                stop('a default member initializer in the initializer list of a non-class at %s' % self.where(n))
            rec = self.nodes[t[1]]
            if rec.get('tagUsed') == 'union':
                fid = p['field']['id']
            else:
                slots = [None] * len(rec.get('bases', [])) + \
                    [c['id'] for c in rec.get('inner', []) if c.get('kind') == 'FieldDecl' and
                     not (c.get('isBitfield') and 'name' not in c)]
                i = [id(c) for c in p['inner']].index(id(n))
                if i >= len(slots) or slots[i] is None:
                    stop('a default member initializer at position %d of %s at %s' % (i, rec.get('name'), self.where(n)))
                fid = slots[i]
        else:
            stop('a default member initializer under %s at %s: no rule yet' % (p.get('kind'), self.where(n)))
        f = self.node(fid)
        if not [c for c in f.get('inner', []) if not c.get('kind', '').endswith('Attr') and
                not (f.get('isBitfield') and c is f['inner'][0])]:
            stop('the default member initializer of %s is used at %s and the dump has no expression for it' % (f.get('name'), self.where(n)))
        return T('ce-default-init', *self.head(n), W(self.need(fid)))

    def e_CXXThisExpr(self, n):
        return T('ce-this', *self.head(n))

    def e_MaterializeTemporaryExpr(self, n):
        sd = n.get('storageDuration')
        kinds = {'full expression': 'full-expression', 'automatic': 'automatic', 'static': 'static'}
        if sd not in kinds:
            stop('storage duration %r at %s: no rule yet' % (sd, self.where(n)))
        ext = None
        if 'extendingDecl' in n:
            ext = self.idof(n['extendingDecl']['id'])
        sub, = self.kids(n)
        return T('ce-materialize-temporary', *self.head(n), T('csd-' + kinds[sd]), optid(ext), self.expr(sub))

    def e_ExprWithCleanups(self, n):
        sub, = self.kids(n)
        return T('ce-with-cleanups', *self.head(n), self.expr(sub))

    def e_ConstantExpr(self, n):
        v = n.get('value')
        if v in ('true', 'false'):
            v = 1 if v == 'true' else 0
        else:
            try:
                v = int(v)
            except (TypeError, ValueError):
                stop('a constant expression whose value is not an integer: %r at %s: no rule yet' % (v, self.where(n)))
        sub, = self.kids(n)
        return T('ce-constant', *self.head(n), cint(v), self.expr(sub))

    def e_LambdaExpr(self, n):
        ks = self.kids(n)
        rec, body = ks[0], ks[-1]
        if rec.get('kind') != 'CXXRecordDecl' or body.get('kind') != 'CompoundStmt':
            stop('a LambdaExpr of an unexpected shape at %s' % self.where(n))
        return T('ce-lambda', *self.head(n), W(self.need(rec['id'])), self.exprs(ks[1:-1]))

    def e_UnaryExprOrTypeTraitExpr(self, n):
        kinds = {'sizeof': 'sizeof', 'alignof': 'alignof', '__alignof': 'preferred-alignof'}
        if n.get('name') not in kinds:
            stop('type trait %r at %s: no rule yet' % (n.get('name'), self.where(n)))
        ks = self.kids(n)
        if 'argType' in n:
            at, arg = self.ty(self.type_at(n['argType'], n)), T('ce-nil')
        else:
            at, arg = self.tyof(ks[0]), self.expr(ks[0])
        return T('ce-unary-or-type-trait', *self.head(n), T('ctk-' + kinds[n['name']]), at, arg)

    def e_SubstNonTypeTemplateParmExpr(self, n):
        parm, sub = self.kids(n)
        if parm.get('kind') != 'NonTypeTemplateParmDecl':
            stop('a SubstNonTypeTemplateParmExpr of an unexpected shape at %s' % self.where(n))
        return T('ce-subst-nttp', *self.head(n), self.expr(sub))

    def suspend(self, con, n):
        # operand; the awaiter (the value the three calls below share, as OpaqueValueExprs);
        # await_ready, await_suspend, await_resume
        ks = self.kids(n)
        if len(ks) != 5:
            stop('a %s with %d children at %s' % (n['kind'], len(ks), self.where(n)))
        return T(con, *self.head(n), *[self.expr(c) for c in ks])

    def e_CoawaitExpr(self, n):
        return self.suspend('ce-coawait', n)

    def e_CoyieldExpr(self, n):
        return self.suspend('ce-coyield', n)

    def e_OpaqueValueExpr(self, n):
        # one value named in several places: the id says which; its source is repeated at each
        ks = self.kids(n)
        if n['id'] not in self.opaque:
            self.opaque[n['id']] = len(self.opaque) + 1
        return T('ce-opaque-value', *self.head(n), W(self.opaque[n['id']]), self.expr(ks[0]) if ks else T('ce-nil'))

    def e_CXXParenListInitExpr(self, n):
        return T('ce-paren-list-init', *self.head(n), self.exprs(self.kids(n)))

    def e_CXXThrowExpr(self, n):
        ks = self.kids(n)
        return T('ce-throw', *self.head(n), self.expr(ks[0]) if ks else T('ce-nil'))

    def e_CXXBindTemporaryExpr(self, n):
        sub, = self.kids(n)
        return T('ce-bind-temporary', *self.head(n), W(self.need(n['dtor']['id'])), self.expr(sub))

    def e_CXXTypeidExpr(self, n):
        ks = self.kids(n)
        if 'typeArg' in n:
            at, arg = self.ty(self.type_at(n['typeArg'], n)), T('ce-nil')
        else:
            at, arg = self.tyof(ks[0]), self.expr(ks[0])
        return T('ce-typeid', *self.head(n), at, arg)

    def e_CXXNoexceptExpr(self, n):
        # the value Sema computed is in neither dump: the operand (unevaluated) is what there is
        sub, = self.kids(n)
        return T('ce-noexcept', *self.head(n), self.expr(sub))

    def e_AtomicExpr(self, n):
        con = 'cao-' + n.get('name', '').lstrip('_').replace('_', '-')
        if con not in CTORS:
            stop('an atomic operation no unit has met before: %s at %s (add %s to CAtomicOp)' % (n.get('name'), self.where(n), con))
        # the arguments are in Clang's order for the node: pointer, order, then the operation's
        # values (for compare-exchange: pointer, success order, expected, failure order, desired, weak)
        return T('ce-atomic', *self.head(n), T(con), self.exprs(self.kids(n)))

    def e_CXXRewrittenBinaryOperator(self, n):
        sub, = self.kids(n)
        return T('ce-rewritten-binary', *self.head(n), self.expr(sub))

    def e_CXXStdInitializerListExpr(self, n):
        sub, = self.kids(n)
        return T('ce-std-initializer-list', *self.head(n), self.expr(sub))

    def e_CXXNewExpr(self, n):
        ks = self.kids(n)
        size = placement = init = None
        style = {None: 'none', 'call': 'parens', 'list': 'braces'}.get(n.get('initStyle'), '?')
        if style == '?':
            stop('new-expression initializer style %r at %s' % (n.get('initStyle'), self.where(n)))
        # the children are: the array size (an array new), the initializer, the placement arguments
        rest = list(ks)
        if n.get('isArray'):
            size, rest = rest[0], rest[1:]
        nplace = 0
        if n.get('isPlacement'):
            if 'operatorNewDecl' not in n:
                stop('a placement new without its operator new, at %s' % self.where(n))
            od = self.node(n['operatorNewDecl']['id'])
            nplace = len([c for c in od.get('inner', []) if c.get('kind') == 'ParmVarDecl']) - 1
        if len(rest) == nplace + 1:
            init, rest = rest[0], rest[1:]
        if len(rest) != nplace or (style != 'none' and init is None):
            stop('a new-expression with children this rule does not place, at %s' % self.where(n))
        opnew = None
        if 'operatorNewDecl' in n:
            opnew = self.need(n['operatorNewDecl']['id'])
        if 'operatorDeleteDecl' in n:
            self.need(n['operatorDeleteDecl']['id'])
        return T('ce-new', *self.head(n), bool(n.get('isGlobal')), bool(n.get('isArray')),
                 optid(opnew), T('cni-' + style), self.opt_expr(size), self.exprs(rest), self.opt_expr(init))

    def e_CXXDeleteExpr(self, n):
        sub, = self.kids(n)
        opdel = None
        if 'operatorDeleteDecl' in n:
            opdel = self.need(n['operatorDeleteDecl']['id'])
        return T('ce-delete', *self.head(n), bool(n.get('isGlobal')), bool(n.get('isArray')),
                 optid(opdel), self.expr(sub))

    # ------------------------------------------------------------------ statements
    def stmt(self, n):
        k = n.get('kind')
        if not k:
            return T('cs-none')
        f = getattr(self, 's_' + k, None)
        if f is not None:
            self.count(k)
            return f(n)
        if hasattr(self, 'e_' + k):
            return T('cs-expr', self.expr(n))
        stop('statement kind with no rule yet: %s (at %s)' % (k, self.where(n)))

    def s_CompoundStmt(self, n):
        return T('cs-compound', inlist('CStmt', [self.stmt(c) for c in self.kids(n)]))

    def s_DeclStmt(self, n):
        vs = []
        for c in self.kids(n):
            k = c.get('kind')
            if k == 'VarDecl':
                vs.append(self.var(c))
            elif k in LOCAL_DECLS_COMPILE_TIME:
                self.elide(k, 'a local declaration with nothing to run; its name was resolved by Sema')
            else:
                stop('a %s in a declaration statement at %s: no rule yet' % (k, self.where(c)))
        return T('cs-decl', vs)

    def flagged(self, n, flags):
        """Children of a statement whose optional leading children are announced by flags."""
        ks = self.kids(n)
        out = []
        for f in flags:
            out.append(ks.pop(0) if n.get(f) else {})
        return out, ks

    def s_IfStmt(self, n):
        if n.get('isConsteval'):
            stop('if consteval at %s: no rule yet' % self.where(n))
        (init, var), ks = self.flagged(n, ('hasInit', 'hasVar'))
        if len(ks) != (3 if n.get('hasElse') else 2):
            stop('an IfStmt of an unexpected shape at %s' % self.where(n))
        # if constexpr: the condition is a ConstantExpr with its value and the branch not taken
        # is an empty compound statement (Sema discarded it)
        return T('cs-if-constexpr' if n.get('isConstexpr') else 'cs-if', self.stmt(init), self.stmt(var), self.expr(ks[0]), self.stmt(ks[1]),
                 self.stmt(ks[2]) if n.get('hasElse') else T('cs-none'))

    def s_WhileStmt(self, n):
        (var,), ks = self.flagged(n, ('hasVar',))
        cond, body = ks
        return T('cs-while', self.stmt(var), self.expr(cond), self.stmt(body))

    def s_DoStmt(self, n):
        body, cond = self.kids(n)
        return T('cs-do', self.stmt(body), self.expr(cond))

    def s_ForStmt(self, n):
        init, var, cond, inc, body = self.kids(n)
        return T('cs-for', self.stmt(init), self.stmt(var), self.opt_expr(cond), self.opt_expr(inc), self.stmt(body))

    def s_CXXForRangeStmt(self, n):
        init, rng, begin, end, cond, inc, var, body = self.kids(n)
        return T('cs-for-range', self.stmt(init), self.stmt(rng), self.stmt(begin), self.stmt(end),
                 self.expr(cond), self.expr(inc), self.stmt(var), self.stmt(body))

    def s_ReturnStmt(self, n):
        ks = self.kids(n)
        return T('cs-return', self.expr(ks[0]) if ks else T('ce-nil'))

    def s_BreakStmt(self, n):
        return T('cs-break')

    def s_ContinueStmt(self, n):
        return T('cs-continue')

    def s_NullStmt(self, n):
        return T('cs-null')

    def s_SwitchStmt(self, n):
        (init, var), ks = self.flagged(n, ('hasInit', 'hasVar'))
        cond, body = ks
        return T('cs-switch', self.stmt(init), self.stmt(var), self.expr(cond), self.stmt(body))

    def s_CaseStmt(self, n):
        if n.get('isGNURange'):
            stop('a GNU case range at %s: no rule yet' % self.where(n))
        value, sub = self.kids(n)
        return T('cs-case', self.expr(value), self.stmt(sub))

    def s_CoroutineBodyStmt(self, n):
        """The coroutine as Sema desugared it. Clang's twelve sub-statements in its order (an
        absent one is cs-none; an expression is a cs-expr), then the copies of the parameters
        into the frame."""
        ks = self.kids(n)
        if len(ks) < 12:
            stop('a CoroutineBodyStmt with %d children at %s' % (len(ks), self.where(n)))
        return T('cs-coroutine-body', *[self.stmt(c) for c in ks[:12]], inlist('CStmt', [self.stmt(c) for c in ks[12:]]))

    def s_CoreturnStmt(self, n):
        operand, call = self.kids(n)
        return T('cs-coreturn', self.opt_expr(operand), self.opt_expr(call))

    def s_CXXTryStmt(self, n):
        ks = self.kids(n)
        if any(c.get('kind') != 'CXXCatchStmt' for c in ks[1:]):
            stop('a CXXTryStmt of an unexpected shape at %s' % self.where(n))
        return T('cs-try', self.stmt(ks[0]), inlist('CStmt', [self.stmt(c) for c in ks[1:]]))

    def s_CXXCatchStmt(self, n):
        var, body = self.kids(n)
        # catch (...) has no variable: the JSON prints a null declaration there
        return T('cs-catch', [self.var(var)] if var.get('kind') else [], self.stmt(body))

    def label(self, clang_id, name=None):
        """The id of a label (the dump has no node for a LabelDecl, only its id)."""
        if clang_id not in self.nodes:
            stub = {'id': clang_id, 'kind': 'LabelDecl', 'name': name}
            self.nodes[clang_id] = stub
            self.parent[id(stub)] = self.tu
        return self.idof(clang_id)

    def s_GotoStmt(self, n):
        return T('cs-goto', W(self.label(n['targetLabelDeclId'])))

    def s_LabelStmt(self, n):
        sub, = self.kids(n)
        w = self.label(n['declId'])
        self.defined.add(w)
        return T('cs-label', W(w), S(n.get('name', '')), self.stmt(sub))

    def s_DefaultStmt(self, n):
        sub, = self.kids(n)
        return T('cs-default', self.stmt(sub))

    # ------------------------------------------------------------------ variables
    def storage(self, n):
        sc = n.get('storageClass')
        if sc not in (None, 'static', 'extern'):
            stop('storage class %r at %s: no rule yet' % (sc, self.where(n)))
        if n.get('tls') not in (None, 'dynamic'):
            stop('thread-local storage of kind %r (not thread_local) at %s: no rule yet' % (n.get('tls'), self.where(n)))
        if n.get('tls'):
            return T('cst-' + (sc + '-' if sc else '') + 'thread-local')
        return T('cst-' + (sc or 'none'))

    def attrs(self, n):
        """The children of a declaration that are attributes; each is accounted for."""
        rest = []
        for c in self.kids(n):
            k = c.get('kind', '')
            if k == 'AlignedAttr' and n.get('kind') == 'VarDecl':
                continue                    # written with the variable (cv-var's align)
            if k.endswith('Attr'):
                if k not in ATTRS_CARRIED_ELSEWHERE:
                    stop('attribute with no rule yet: %s on %s at %s' % (k, n.get('name'), self.where(n)))
                self.elide(k, ATTRS_CARRIED_ELSEWHERE[k])
            else:
                rest.append(c)
        return rest

    def var(self, n):
        self.count('VarDecl')
        w = self.idof(n['id'])
        self.defined.add(w)
        align = T('cal-none')
        for c in self.kids(n):
            if c.get('kind') == 'AlignedAttr':
                if align.con != 'cal-none':
                    stop('two alignment specifiers on %s at %s: no rule yet' % (n.get('name'), self.where(n)))
                self.count('AlignedAttr (on a variable)')
                if c['_align_type'] is not None:
                    align = T('cal-type', self.ty(self.type_at({'qualType': c['_align_type']}, n)))
                else:
                    e, = c['inner']
                    if e.get('kind') != 'ConstantExpr':
                        stop('an alignment that is not a constant expression at %s' % self.where(n))
                    align = T('cal-bytes', W(int(e['value'])))
        ks = [c for c in self.attrs(n) if c.get('kind') != 'TemplateArgument']
        style = n.get('init')
        if style is None:
            if ks:
                stop('a VarDecl with children and no initializer at %s' % self.where(n))
            init = T('cvi-none')
        else:
            if style not in ('c', 'call', 'list') or len(ks) != 1:
                stop('a VarDecl with initializer style %r and %d children at %s' % (style, len(ks), self.where(n)))
            init = T('cvi-' + style, self.expr(ks[0]))
        return T('cv-var', W(w), S(n.get('name', '')), self.tyof(n), self.storage(n), align,
                 bool(n.get('constexpr')), bool(n.get('nrvo')), init)

    def parms_of(self, fn):
        return [c for c in fn.get('inner', []) if c.get('kind') == 'ParmVarDecl']

    def parm(self, n, fn, i):
        """The i-th parameter of the function fn (its defining declaration). Its default
        argument may be written on another declaration of the function: all are searched."""
        self.count('ParmVarDecl')
        w = self.idof(n['id'])
        self.defined.add(w)
        defaults = []
        for rid in self.redecls.get(fn['id'], [fn['id']]):
            ps = self.parms_of(self.nodes[rid])
            if i < len(ps):
                ks = self.attrs(ps[i])
                if ks:
                    if 'init' not in ps[i] or len(ks) != 1:
                        stop('a parameter with children this rule does not place: %s at %s' % (n.get('name'), self.where(ps[i])))
                    defaults.append(ks[0])
        if len(defaults) > 1:
            stop('the parameter %s of %s has a default argument on %d declarations' % (n.get('name'), fn.get('name'), len(defaults)))
        try:
            t = self.expr_type(n)
        except Stop as why:
            # the string is not read (sugar printed as written elsewhere): the type is in the
            # function's mangled name, without the parameter's own top-level qualifiers
            q = n['type']['qualType']
            if re.match(r'(const|volatile)\b', q) and not q.rstrip().endswith(('&', '*', ']', ')')):
                stop('the type of the parameter %s of %s is not read from its string (%s) and is qualified at the top level'
                     % (n.get('name'), fn.get('name'), why))
            ps = self.mangled_params(fn)
            if i >= len(ps):
                stop('the mangled name of %s has %d parameters; the parameter %d is not there' % (fn.get('name'), len(ps), i))
            t = ps[i]
        return T('cv-parm', W(w), S(n.get('name', '')), self.ty(t), self.expr(defaults[0]) if defaults else T('ce-nil'))

    def e_CXXDefaultArgExpr(self, n):
        """The parameter whose default argument this is: the JSON does not say, the position in
        the call does (the callee's declaration and the argument's index)."""
        p = self.parent[id(n)]
        i = [id(c) for c in p['inner']].index(id(n))
        k = p.get('kind')
        if k in ('CXXConstructExpr', 'CXXTemporaryObjectExpr'):
            fn = self.constructor_of(p)
        elif k in ('CallExpr', 'CXXMemberCallExpr', 'CXXOperatorCallExpr'):
            c = p['inner'][0]
            while c.get('kind') in ('ImplicitCastExpr', 'ParenExpr'):
                c = c['inner'][0]
            if c.get('kind') == 'DeclRefExpr':
                fn = self.node(c['referencedDecl']['id'])
            elif c.get('kind') == 'MemberExpr':
                fn = self.node(c['referencedMemberDecl'])
            else:
                stop('a default argument in a call whose callee is a %s at %s: no rule yet' % (c.get('kind'), self.where(p)))
            i -= 1
            if k == 'CXXOperatorCallExpr' and fn['kind'] != 'FunctionDecl' and fn.get('storageClass') != 'static':
                i -= 1                      # the first argument of a member operator call is the object
        else:
            stop('a default argument under a %s at %s: no rule yet' % (k, self.where(n)))
        if fn['kind'] not in FUNCTION_KINDS:
            stop('a default argument in a call of a %s at %s' % (fn['kind'], self.where(p)))
        ps = self.parms_of(fn)
        if not 0 <= i < len(ps):
            stop('a default argument at position %d of %s, which has %d parameters (at %s)' % (i, fn.get('name'), len(ps), self.where(p)))
        self.need(fn['id'])
        return T('ce-default-arg', *self.head(n), W(self.idof(ps[i]['id'])))

    # ------------------------------------------------------------------ declarations
    def run(self):
        roots, statics = self.roots()
        root_ids = [W(self.need(r['id'])) for r in roots]
        qi = 0
        while qi < len(self.queue):
            n = self.queue[qi]
            qi += 1
            w = self.ids[n['id']]
            self.defined.add(w)
            self.decls.append((w, self.decl(n)))
            self.labels.append('%s %s%s' % (n['kind'], self.display_name(n),
                                            ' : ' + n['type']['qualType'] if 'type' in n else ''))
        written = [n for n in self.queue if n['kind'] in ('VarDecl', 'VarTemplateSpecializationDecl')]
        static_ids = [W(self.ids[n['id']]) for n in sorted(written, key=lambda n: n['_order'])]
        missing = sorted(set(self.ids.values()) - self.defined)
        if missing:
            names = [self.nodes[c].get('name') for c, w in self.ids.items() if w in missing]
            stop('declarations referred to and never written: %s' % names[:20])
        self.tu_term = T('ctu', S(os.path.basename(self.path)), S(self.target), root_ids, static_ids,
                         [Ref('%s-d%d' % (self.name, w), 'CDecl') for w, _ in self.decls])

    def roots(self):
        """What the unit must provide: the functions defined in the main file that another unit
        can call (external linkage, not inline, not a template), and every variable of static
        storage defined in it (its initialization runs whether or not it is referred to).
        The unit's `statics` are all variables of static storage written, these and the ones
        reached in headers, in the order of their definitions."""
        roots, statics = [], []

        def scan(n, internal):
            for c in n.get('inner', []):
                k = c.get('kind')
                if k in ('LinkageSpecDecl',):
                    scan(c, internal)
                elif k == 'NamespaceDecl':
                    scan(c, internal or 'name' not in c)
                elif c.get('_file') != self.path or c.get('isImplicit'):
                    continue
                elif k == 'FunctionDecl':
                    if self.is_definition(c) and not internal and not c.get('inline') and \
                            c.get('storageClass') != 'static' and not c.get('constexpr') and \
                            'explicitlyDeleted' not in c:
                        roots.append(self.node(c['id']))
                elif k == 'VarDecl':
                    if self.is_definition(c):
                        statics.append(self.node(c['id']))
                        roots.append(self.node(c['id']))
        scan(self.tu, False)
        if not roots:
            stop('%s defines nothing another unit can use: no roots' % self.path)
        return roots, statics

    def decl(self, n):
        k = n['kind']
        f = getattr(self, 'd_' + k, None)
        if f is None:
            stop('declaration kind with no rule yet: %s (%s at %s)' % (k, n.get('name'), self.where(n)))
        self.count(k)
        return f(n)

    def function(self, n):
        ks = self.attrs(n)
        params, body, inits = [], None, []
        for c in ks:
            ck = c.get('kind')
            if ck == 'ParmVarDecl':
                params.append(self.parm(c, n, len(params)))
            elif ck in ('CompoundStmt', 'CXXTryStmt', 'CoroutineBodyStmt'):
                body = c                    # (a function-try-block, or a coroutine's body, is the body)
            elif ck == 'CXXCtorInitializer':
                inits.append(c)
            elif ck == 'TemplateArgument':
                self.elide('TemplateArgument (of a function)', 'the specialization is written already substituted')
            elif ck in ('RequiresExpr', 'ConceptSpecializationExpr', 'BinaryOperator', 'UnaryOperator', 'ParenExpr',
                        'CXXBoolLiteralExpr', 'CXXFoldExpr', 'DeclRefExpr', 'UnresolvedLookupExpr', 'TypeTraitExpr'):
                # an expression that is a direct child of a function is its requires-clause
                self.elide('requires-clause (%s)' % ck, 'a constraint: Sema checked it when it chose the function')
            else:
                stop('a %s under the function %s at %s: no rule yet' % (ck, n.get('name'), self.where(n)))
        if 'explicitlyDeleted' in n:
            stop('a deleted function is referred to: %s at %s' % (n.get('name'), self.where(n)))
        if body is not None:
            b = T('cfb-stmt', self.stmt(body))
            self.count('CompoundStmt (function bodies)')
        elif n.get('pure'):
            b = T('cfb-pure')
        elif 'explicitlyDefaulted' in n or (n.get('isImplicit') and k_is_special_member(n)):
            b = T('cfb-defaulted')
        else:
            b = T('cfb-none')
        if n.get('variadic') is not None and not isinstance(n.get('variadic'), bool):
            stop('variadic flag %r' % n.get('variadic'))
        t = self.function_type(n)
        fn = T('cfunction', W(self.ids[n['id']]), S(n.get('name', '')), S(n.get('mangledName', '')),
               self.ty(t), params, self.storage(n), bool(n.get('inline')), bool(n.get('constexpr')),
               bool(n.get('isImplicit')), b)
        return fn, inits

    def d_FunctionDecl(self, n):
        fn, inits = self.function(n)
        return T('cd-function', fn)

    def is_virtual(self, n, rec):
        vt = self.vtable_of(rec)
        if n.get('virtual') or vt is None:
            return bool(n.get('virtual'))
        fns = [e[1] if e[0] == 'thunk' else e for e in vt['entries']]
        return any(e[1] is n for e in fns if e[0] in ('method', 'pure', 'complete-dtor', 'deleting-dtor'))

    def d_CXXMethodDecl(self, n):
        rec = self.record_of_member(n)
        fn, inits = self.function(n)
        return T('cd-method', W(self.need(rec['id'])), self.is_virtual(n, rec), fn)

    def d_CXXConversionDecl(self, n):
        rec = self.record_of_member(n)
        fn, inits = self.function(n)
        return T('cd-conversion', W(self.need(rec['id'])), self.is_virtual(n, rec), fn)

    def d_CXXDestructorDecl(self, n):
        rec = self.record_of_member(n)
        fn, inits = self.function(n)
        return T('cd-destructor', W(self.need(rec['id'])), self.is_virtual(n, rec), fn)

    def d_CXXConstructorDecl(self, n):
        rec = self.record_of_member(n)
        fn, inits = self.function(n)
        out = []
        for c in inits:
            self.count('CXXCtorInitializer')
            e, = self.kids(c)
            if 'anyInit' in c:
                out.append(T('cci-field', W(self.need(c['anyInit']['id'])), self.expr(e)))
            elif 'baseInit' in c:
                out.append(T('cci-base', self.ty(self.type_at(c['baseInit'], n)), self.expr(e)))
            elif 'delegatingInit' in c:
                out.append(T('cci-delegating', self.expr(e)))
            else:
                stop('a constructor initializer that is neither a member nor a base at %s: no rule yet' % self.where(n))
        return T('cd-constructor', W(self.need(rec['id'])), out, fn)

    def record(self, n):
        if not n.get('completeDefinition'):
            stop('a class used and never defined in the unit: %s (at %s): no rule yet' % (n.get('name'), self.where(n)))
        self.nrecords += 1
        tag = n.get('tagUsed')
        if tag not in ('struct', 'class', 'union'):
            stop('record tag %r' % tag)
        sizes, vptr, lbases, lfields, lvbases = self.layout_of(n)
        self.nlayouts += 1
        # bases, in declaration order, with their offsets from the layout
        bases = []
        offs = {}
        for rid, off, primary in lbases:
            offs[rid] = (off, primary)
        voffs = {rid: (off, primary) for rid, off, primary in lvbases}
        nv = 0
        for b in n.get('bases', []):
            bt = self.base_type(n, b)
            where = voffs if b.get('isVirtual') else offs
            if bt[1] not in where:
                stop('the layout of %s does not place its base %s' % (n.get('name'), self.nodes[bt[1]].get('name')))
            nv += not b.get('isVirtual')
            bases.append(T('cbase', W(self.need(bt[1])), W(where[bt[1]][0]), where[bt[1]][1], bool(b.get('isVirtual'))))
        if nv != len(lbases):
            stop('the layout of %s lists %d non-virtual bases, the class %d' % (n.get('name'), len(lbases), nv))
        fields = []
        fdecls = [c for c in self.attrs(n) if c.get('kind') == 'FieldDecl']
        captures = self.captures(n, fdecls) if self.is_lambda(n) else {}
        placed = [c for c in fdecls if not (c.get('isBitfield') and 'name' not in c and self.bit_width(c) == 0)]
        if len(placed) != len(lfields):
            stop('the layout of %s lists %d fields, the class %d' % (n.get('name'), len(lfields), len(placed)))
        offsets = {}
        for c, (off, text) in zip(placed, lfields):
            if c.get('name') and not text.endswith(' ' + c['name']) and text != c['name']:
                stop('the layout of %s names %r where the class has the field %s' % (n.get('name'), text, c['name']))
            offsets[c['id']] = off
        for c in fdecls:
            if c['id'] not in offsets:
                continue        # a zero-width unnamed bit-field: alignment only, in the layout
            self.count('FieldDecl')
            w = self.idof(c['id'])
            self.defined.add(w)
            ks = self.attrs(c)
            width = T('cbw-none')
            if c.get('isBitfield'):
                width = T('cbw-bits', W(self.bit_width(c)))
                ks = ks[1:]
            init = T('ce-nil')
            if c.get('hasInClassInitializer') and not ks:
                # a default member initializer of a class template's member that nothing
                # instantiated: there is no expression (a CXXDefaultInitExpr for it would stop)
                self.lazy_inits.add(c['id'])
            elif c.get('hasInClassInitializer'):
                if len(ks) != 1:
                    stop('the default member initializer of %s::%s is not one expression' % (n.get('name'), c.get('name')))
                init = self.expr(ks[0])
            elif ks:
                stop('a field with children this rule does not place: %s::%s' % (n.get('name'), c.get('name')))
            fields.append(T('cfield', W(w), S(c.get('name', '')), self.tyof(c), W(offsets[c['id']]),
                            width, bool(c.get('mutable')), captures.get(c['id'], T('ccap-none')), init))
        vt = self.vtable_of(n)
        if vt is None:
            if vptr is not None or any(self.layout_of(self.nodes[b[0]])[1] is not None for b in lbases + lvbases):
                # polymorphic, and Clang laid out no vtable for it in this unit: nothing here
                # constructs it or calls a virtual function of it (std::type_info under typeid)
                vtable = T('cvtable-full', False, [], [], [], [], [])
            else:
                vtable = T('cvt-none')
        else:
            simple = vt['emitted'] and len(set(p[2] for p in vt['points'])) == 1 and not vt['vbo'] and not vt['ctor'] and \
                not vt['vtt'] and all(e[0] not in ('thunk', 'vbase-offset', 'vcall-offset') for e in vt['entries'])
            if simple:
                vtable = T('cvtable', W(vt['points'][0][2]), [self.vt_entry(e) for e in vt['entries']])
            else:
                vtable = T('cvtable-full', vt['emitted'], self.vt_points(vt['points']),
                           [self.vt_entry(e) for e in vt['entries']],
                           [T('cvboo', W(self.need(rid)), cint(at)) for rid, at in vt['vbo']],
                           [T('cvtt-own', W(i)) if c is None else T('cvtt-construction', W(self.need(c[0])), W(c[1]), W(i))
                            for c, i in vt['vtt']],
                           [T('cctor-vtable', W(self.need(rid)), W(off), self.vt_points(ps), [self.vt_entry(e) for e in es])
                            for rid, off, ps, es in vt['ctor']])
        # a destructor that is declared runs whenever an object of the class dies
        for c in n.get('inner', []):
            if c.get('kind') == 'CXXDestructorDecl' and 'explicitlyDeleted' not in c:
                self.need(c['id'])
        for c in n.get('inner', []):
            k = c.get('kind')
            if k in ('FieldDecl', 'CXXDestructorDecl') or k.endswith('Attr'):
                continue
            if k in FUNCTION_KINDS or k in ('AccessSpecDecl', 'FriendDecl', 'TemplateArgument', 'TypedefDecl',
                                            'TypeAliasDecl', 'UsingDecl', 'UsingShadowDecl', 'StaticAssertDecl',
                                            'FunctionTemplateDecl', 'CXXRecordDecl', 'ClassTemplateDecl', 'EnumDecl',
                                            'VarDecl', 'TypeAliasTemplateDecl', 'ClassTemplateSpecializationDecl',
                                            'ClassTemplatePartialSpecializationDecl', 'VarTemplateDecl',
                                            'CXXDeductionGuideDecl', 'ConstructorUsingShadowDecl'):
                continue        # members: written when referred to; the rest hold at compile time
            if k == 'IndirectFieldDecl':
                # a member of an anonymous struct or union, named in the enclosing class: every
                # access is written out by Sema through the unnamed field that holds it
                self.elide(k, 'the access path through the unnamed member is explicit in every MemberExpr')
                continue
            stop('a %s in the class %s: no rule yet' % (k, n.get('name')))
        name = n.get('name') or ''
        lay = [W(sizes['sizeof']), W(sizes['align']), W(sizes['dsize']), W(sizes['nvsize']),
               W(sizes['nvalign']), T('cvp-none') if vptr is None else T('cvp-at', W(vptr))]
        if lvbases:
            lay = T('clayout-v', *lay, [T('cvbase', W(self.need(rid)), W(off)) for rid, off, _ in lvbases])
        else:
            lay = T('clayout', *lay)
        return T('crecord', W(self.ids[n['id']]), S(name), T('ctg-' + tag), self.is_lambda(n), bases, fields, lay, vtable)

    def captures(self, rec, fdecls):
        """For a closure class: field id -> what it captures. The JSON lists the captures only
        as their initializers in the LambdaExpr, one per field in order; a capture of a
        variable is initialized from a reference to it (through the copy of a by-value
        capture), a capture of `this` from `this`."""
        lam = self.parent[id(rec)]
        if lam.get('kind') != 'LambdaExpr':
            stop('a closure class outside a LambdaExpr: at %s' % self.where(rec))
        inits = lam['inner'][1:-1]
        if len(inits) != len(fdecls):
            stop('the lambda at %s has %d captures and %d fields' % (self.where(rec), len(inits), len(fdecls)))
        out = {}
        for f, e in zip(fdecls, inits):
            while True:
                k = e.get('kind')
                if k in ('ImplicitCastExpr', 'ParenExpr') or (k == 'CXXConstructExpr' and len(e.get('inner', [])) == 1) or \
                        (k == 'UnaryOperator' and e.get('opcode') == '*' and e['inner'][0].get('kind') == 'CXXThisExpr'):
                    e = e['inner'][0]
                else:
                    break
            if k == 'CXXThisExpr':
                out[f['id']] = T('ccap-this')
                continue
            # the field's location is the capture's name in the source: a plain capture names
            # the variable its initializer refers to; any other name is an init-capture's
            name = self.source_identifier(f)
            if k == 'DeclRefExpr' and e['referencedDecl']['kind'] in ('VarDecl', 'ParmVarDecl') and \
                    e['referencedDecl'].get('name') == name and not self.init_capture_refs(rec, name):
                out[f['id']] = T('ccap-var', W(self.idof(e['referencedDecl']['id'])))
                continue
            refs = self.init_capture_refs(rec, name)
            if len(refs) > 1:
                stop('the lambda at %s refers to %d different undeclared variables named %s' % (self.where(rec), len(refs), name))
            vid = refs.pop() if refs else 'init-capture:%s:%s' % (rec['id'], name)
            stub = {'id': vid, 'kind': 'VarDecl', 'name': name, '_init_capture': True}
            self.nodes[vid] = stub
            self.parent[id(stub)] = rec
            self.stubs.append(stub)
            w = self.idof(vid)
            self.defined.add(w)
            out[f['id']] = T('ccap-init', W(w))
        return out

    def init_capture_refs(self, rec, name):
        """The ids of the variables named `name` that the lambda's body refers to and the dump
        declares nowhere: an init-capture's variable is not dumped, only referred to."""
        ids = set()
        stack = [c for c in rec.get('inner', [])]
        while stack:
            c = stack.pop()
            if c.get('kind') == 'DeclRefExpr':
                r = c['referencedDecl']
                if r.get('kind') == 'VarDecl' and r.get('name') == name and \
                        (r['id'] not in self.nodes or self.nodes[r['id']].get('_init_capture')):
                    ids.add(r['id'])
            stack.extend(x for x in c.get('inner', []) + c.get('array_filler', []) if x)
        return ids

    def source_identifier(self, n):
        """The identifier in the source at the location of n."""
        f, line, col = n.get('_file'), n.get('_line'), n.get('_col')
        if f not in self.sources:
            with open(f, encoding='utf-8', errors='replace') as fh:
                self.sources[f] = fh.read().split('\n')
        m = re.match(r'[A-Za-z_][A-Za-z0-9_]*', self.sources[f][line - 1][col - 1:]) if col else None
        if not m:
            stop('no identifier in the source at %s:%s:%s (the name of a lambda capture is read there)' % (f, line, col))
        return m.group(0)

    def vt_points(self, points):
        return [T('cvap', W(self.need(rid)), W(off), W(i)) for rid, off, i in points]

    def vt_entry(self, e):
        kind = e[0]
        if kind in ('offset-to-top', 'vbase-offset', 'vcall-offset'):
            return T('cvt-' + kind, cint(e[1]))
        if kind == 'rtti':
            return T('cvt-rtti')
        if kind == 'thunk':
            adj = e[2]
            t, r = adj.get('this', (0, None)), adj.get('return', (0, None))
            opt = lambda v: T('coin-none') if v is None else T('coin-some', cint(v))
            return T('cvt-thunk', self.vt_entry(e[1]), cint(t[0]), opt(t[1]), cint(r[0]), opt(r[1]))
        return T('cvt-' + kind, W(self.need(e[1]['id'])))

    def bit_width(self, c):
        ks = [x for x in c.get('inner', []) if not x.get('kind', '').endswith('Attr')]
        if not ks or ks[0].get('kind') != 'ConstantExpr':
            stop('a bit-field whose width is not a constant expression: %s' % c.get('name'))
        return int(ks[0]['value'])

    def d_CXXRecordDecl(self, n):
        return T('cd-record', self.record(n))

    def d_ClassTemplateSpecializationDecl(self, n):
        return T('cd-class-template-specialization', self.targs(n), self.record(n))

    def template_args_or_stop(self, n):
        try:
            return self.template_args(n)
        except Dependent as e:
            stop('template arguments of %s at %s: %s: no rule yet' % (n.get('name'), self.where(n), e))

    def d_EnumDecl(self, n):
        if 'fixedUnderlyingType' in n:
            underlying = self.type_at(n['fixedUnderlyingType'], n)
        else:
            # the type Sema chose is in no dump: Clang is asked for __underlying_type of the
            # enumeration, named directly or as the type of one of its enumerators
            first = [c for c in n.get('inner', []) if c.get('kind') == 'EnumConstantDecl']
            if n.get('name'):
                arg = self.spell_decl(n)
            elif first:
                arg = 'decltype(%s)' % self.spell_decl(first[0])
            else:
                stop('an enumeration with no name and no enumerators at %s' % self.where(n))
            underlying = self.probe_type('__underlying_type(%s)' % arg)
        es, nxt = [], 0
        for c in self.attrs(n):
            if c.get('kind') != 'EnumConstantDecl':
                stop('a %s in the enumeration %s' % (c.get('kind'), n.get('name')))
            self.count('EnumConstantDecl')
            ks = self.attrs(c)
            if ks:
                # the initializer, evaluated by Sema
                e = ks[0]
                while e.get('kind') != 'ConstantExpr':
                    if e.get('kind') != 'ImplicitCastExpr':
                        stop('the enumerator %s::%s has an initializer without a constant value' % (n.get('name'), c.get('name')))
                    e = e['inner'][0]
                nxt = int(e['value'])
            # an enumerator without an initializer is the previous one plus one ([dcl.enum]/2)
            w = self.idof(c['id'])
            self.defined.add(w)
            es.append(T('cenumerator', W(w), S(c['name']), cint(nxt)))
            nxt += 1
        return T('cd-enum', W(self.ids[n['id']]), S(n.get('name', '')),
                 self.ty(underlying), 'scopedEnumTag' in n, es)

    def d_VarDecl(self, n):
        v = self.var(n)
        self.met['VarDecl'] -= 1
        return T('cd-var-dynamic' if n['id'] in self.dynamic_vars() else 'cd-var', S(n.get('mangledName', '')), v)

    def dynamic_vars(self):
        """The ids of the variables the unit initializes at start-up. The dumps do not say which
        initializers Clang made constants; the IR does: each `__cxx_global_var_init` function
        initializes one variable, in the order of the definitions. The variable of each is one of
        those it refers to, and the choice must be the only one that keeps the order."""
        if self._dynamic is not None:
            return self._dynamic
        by_mangled = {}
        for vs in self.var_by_name.values():
            for v in vs:
                if v.get('mangledName'):
                    by_mangled.setdefault(v['mangledName'], self.node(v['id']))
        fns = re.findall(r'^define [^\n]*@(__cxx_global_var_init(?:\.\d+)?)\(\)[^\n]*\{\n(.*?)^\}', self.ir_text, re.M | re.S)
        refs = []
        for name, body in fns:
            vs = {}
            for sym in re.findall(r'@("[^"]+"|[A-Za-z0-9_.$]+)', body):
                v = by_mangled.get(sym.strip('"'))
                if v is not None:
                    vs[v['id']] = v
            refs.append(sorted(vs.values(), key=lambda v: v['_order']))
        sols = []

        def go(k, last, acc):
            if len(sols) > 1:
                return
            if k == len(refs):
                sols.append(list(acc))
                return
            for v in refs[k]:
                if v['_order'] > last:
                    acc.append(v['id'])
                    go(k + 1, v['_order'], acc)
                    acc.pop()
        go(0, -1, [])
        if len(sols) != 1:
            stop('the %d start-up initialization functions of the unit do not determine their variables (%d ways): no rule yet'
                 % (len(refs), len(sols)))
        self._dynamic = set(sols[0])
        return self._dynamic

    def d_VarTemplateSpecializationDecl(self, n):
        v = self.var(n)
        self.met['VarDecl'] -= 1
        return T('cd-var-template-specialization', self.targs(n), S(n.get('mangledName', '')), v)

    def targs(self, n):
        args = []
        for kind, x in self.template_args_or_stop(n):
            if kind == 'int?':
                stop('a template argument of %s is printed as %d and the type of its parameter is not read: no rule yet' % (n.get('name'), x))
            args.append(T('cta-type', self.ty(x)) if kind == 'type' else T('cta-integral', cint(x)))
            self.count('TemplateArgument')
        return args

    # ------------------------------------------------------------------ writing
    def write(self, out, limit):
        """One definition per shared type and per declaration; a subterm larger than `limit`
        nodes becomes a definition of its own, so that no definition is an enormous literal."""
        self.hoisted = []
        self.nnodes = 0
        lines = []

        def render(a, top=False):
            if isinstance(a, bool):
                return 'true' if a else 'false'
            if isinstance(a, W):
                return '#%d' % a.n
            if isinstance(a, Ref):
                return a.name
            if isinstance(a, S):
                r = 'nil'
                for b in reversed(a.b):
                    r = 'cons #%d (%s)' % (b, r) if r != 'nil' else 'cons #%d nil' % b
                return '(cstr %s)' % (r if r == 'nil' else '(%s)' % r)
            if isinstance(a, list):
                r = 'nil'
                for x in reversed(a):
                    r = 'cons %s %s' % (render(x), r if r == 'nil' else '(%s)' % r)
                return r if r == 'nil' else '(%s)' % r
            self.nnodes += 1
            if not a.args:
                return a.con
            parts = [a.con]
            for x in a.args:
                if isinstance(x, T) and x.args and x.size > limit:
                    parts.append(hoist(x))
                else:
                    parts.append(render(x))
            s = ' '.join(parts)
            return s if top else '(%s)' % s

        def hoist(x):
            name = '%s-n%d' % (self.name, len(self.hoisted) + 1)
            self.hoisted.append(name)
            body = render(x, top=True)
            lines.append('%s : %s\n%s = %s\n' % (name, CTORS[x.con][0], name, body))
            return name

        for ref, term in self.type_defs:
            lines.append('%s : CType\n%s = %s\n' % (ref.name, ref.name, render(term, top=True)))
        for (w, term), label in zip(self.decls, self.labels):
            name = '%s-d%d' % (self.name, w)
            body = render(term, top=True)
            lines.append('-- %s: %s\n%s : CDecl\n%s = %s\n' % (name, label, name, name, body))
        body = render(self.tu_term, top=True)
        lines.append('%s-tu : CTU\n%s-tu = %s\n' % (self.name, self.name, body))
        out.write(self.header())
        out.write(''.join(lines))

    def header(self):
        h = ['-- %s.cub — %s as a value of CTU (lib/cxx/ast.cub), after Clang\'s semantic analysis.' % (self.name, self.path),
             '-- GENERATED by tools/clang2cub.py from %s, %s, flags %s.' % (self.clang_version, self.target, ' '.join(self.flags)),
             '-- Load after the library and lib/cxx/ast.cub. %s-tu is the unit; %s-dN the declaration with id N;' % (self.name, self.name),
             '-- %s-tN a type shared by the nodes that have it; %s-nN a subterm written apart to keep definitions small.' % (self.name, self.name),
             '--']
        h += ['-- ' + l for l in self.report().splitlines()]
        return '\n'.join(h) + '\n'

    def report(self):
        r = []
        r.append('nodes written: %d; declarations: %d; shared types: %d; records with layouts: %d of %d'
                 % (self.nnodes, len(self.decls), len(self.type_defs), self.nlayouts, self.nrecords))
        r.append('node kinds met (%d): %s' % (len(self.met), ', '.join('%s %d' % kv for kv in sorted(self.met.items()))))
        r.append('cast kinds met (%d): %s' % (len(self.met_casts), ', '.join('%s %d' % kv for kv in sorted(self.met_casts.items()))))
        r.append('met and not written as nodes (%d): %s' % (len(self.elided), '; '.join(
            '%s %d (%s)' % (k, c, why) for k, (c, why) in sorted(self.elided.items()))))
        r.append('read in type strings and not written (%d): %s' % (len(self.type_attrs), '; '.join(sorted(self.type_attrs))))
        r.append('signatures read from mangled names, where the printed types were not read (%d): %s'
                 % (len(self.from_mangled), '; '.join(sorted(self.from_mangled))))
        r.append('asked of Clang by a probe (%d): %s' % (len(self.probes), '; '.join(
            '%s = %s' % kv for kv in sorted(self.probes.items()))))
        r.append('names resolved from the scope of use that name other types elsewhere in the unit (%d): %s'
                 % (len(self.scope_dependent), ', '.join(sorted(self.scope_dependent))))
        return '\n'.join(textwrap_lines(r, 110))


def textwrap_lines(paras, width):
    out = []
    for p in paras:
        line = ''
        for word in p.split(' '):
            if line and len(line) + 1 + len(word) > width:
                out.append(line)
                line = '  ' + word
            else:
                line = word if not line else line + ' ' + word
        out.append(line)
    return out


def k_is_special_member(n):
    return n['kind'] in ('CXXConstructorDecl', 'CXXDestructorDecl') or n.get('name') == 'operator='


def c_unescape(s):
    """The text of a string literal as Clang prints it -> [('cp', code point) | ('raw', unit)]:
    characters and \\u escapes are code points to be encoded, \\x and octal escapes are code
    units as they stand."""
    out, i = [], 0
    simple = {'n': 10, 't': 9, 'r': 13, '\\': 92, '"': 34, "'": 39, 'a': 7, 'b': 8, 'f': 12, 'v': 11, '?': 63}
    while i < len(s):
        ch = s[i]
        if ch != '\\':
            out.append(('cp', ord(ch)))
            i += 1
            continue
        nx = s[i + 1]
        if nx == 'x':
            j = i + 2
            while j < len(s) and s[j] in '0123456789abcdefABCDEF':
                j += 1
            out.append(('raw', int(s[i + 2:j], 16)))
            i = j
        elif nx in 'uU':
            j = i + 2 + (4 if nx == 'u' else 8)
            out.append(('cp', int(s[i + 2:j], 16)))
            i = j
        elif nx in '01234567':
            j = i + 1
            while j < len(s) and j < i + 4 and s[j] in '01234567':
                j += 1
            out.append(('raw', int(s[i + 1:j], 8)))
            i = j
        elif nx in simple:
            out.append(('cp', simple[nx]))
            i += 2
        else:
            stop('string literal escape not read: \\%s in %r' % (nx, s))
    return out


def ieee_bits(text, mant, expbits, explicit):
    """The bit pattern of the decimal `text` rounded to nearest-even in a binary format of
    `mant` significand bits (the leading one stored when `explicit`) and `expbits` exponent bits."""
    from fractions import Fraction
    m = re.match(r'(-?)(\d+(?:\.\d*)?|\.\d+)(?:[eE]([-+]?\d+))?$', text.strip())
    if not m:
        stop('floating literal not read: %r' % text)
    neg = m.group(1) == '-'
    v = Fraction(m.group(2)) * Fraction(10) ** int(m.group(3) or 0)
    bias = (1 << (expbits - 1)) - 1
    stored = mant if explicit else mant - 1
    if v == 0:
        return (1 << (stored + expbits)) if neg else 0
    e = v.numerator.bit_length() - v.denominator.bit_length()
    if Fraction(2) ** e > v:
        e -= 1                              # 2^e <= v < 2^(e+1)
    e = max(e, 1 - bias)                    # subnormals share the least exponent
    scaled = v / Fraction(2) ** (e - (mant - 1))
    q, r = divmod(scaled.numerator, scaled.denominator)
    if 2 * r > scaled.denominator or (2 * r == scaled.denominator and q & 1):
        q += 1
    if q >> mant:
        q >>= 1
        e += 1
    if e > bias:
        stop('floating literal out of range: %r' % text)
    if q >> (mant - 1):
        field = e + bias
    else:
        field = 0                           # subnormal
    frac = q if explicit else q & ((1 << (mant - 1)) - 1)
    return (int(neg) << (stored + expbits)) | (field << stored) | frac


def main(argv):
    if argv[1:] == ['--emit-ast']:
        emit_ast(sys.stdout)
        return 0
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('source')
    ap.add_argument('-o', '--output', required=True)
    ap.add_argument('--flags', default=None,
                    help='the Clang flags of the unit, space separated (default: %s)' % ' '.join(CLANG_FLAGS))
    ap.add_argument('--limit', type=int, default=400,
                    help='nodes in one definition before a subterm is written apart (default 400)')
    a = ap.parse_args(argv[1:])
    name = os.path.splitext(os.path.basename(a.output))[0]
    u = Unit(a.source, name, a.flags.split() if a.flags is not None else None)
    try:
        u.run_clang()
        u.index()
        u.read_layouts()
        u.read_vtables()
        u.start()
        u.run()
        with open(a.output, 'w') as out:
            u.write(out, a.limit)
    except Stop as e:
        sys.stderr.write('clang2cub: %s\n' % e)
        return 1
    try:
        print(u.report())
    except BrokenPipeError:
        pass
    return 0


if __name__ == '__main__':
    sys.setrecursionlimit(100000)
    sys.exit(main(sys.argv))

#!/usr/bin/env python3
"""gen_messages.py COUNT SEED — COUNT random 22-byte wire messages on stdout (the input of the
book.rs / book.cpp comparison, archive/2026-11-11/R-rust.md). 48% add, 24% cancel, 24% execute, 0.1% reset,
the rest an unknown type; sides 0/1 and, rarely, another byte (read as Ask by both); prices
994..1006 and, rarely, any 64-bit value; quantities 1..500 and, rarely, any 32-bit value."""
import random, struct, sys

def main():
    count, seed = int(sys.argv[1]), int(sys.argv[2])
    r = random.Random(seed)
    out = bytearray()
    for _ in range(count):
        x = r.random()
        t = b'A' if x < 0.48 else b'X' if x < 0.72 else b'E' if x < 0.96 else b'R' if x < 0.961 else bytes([r.randrange(256)])
        side = r.randrange(2) if r.random() < 0.99 else r.randrange(256)
        price = r.randrange(994, 1007) if r.random() < 0.995 else r.randrange(-2**63, 2**63)
        qty = r.randrange(1, 501) if r.random() < 0.995 else r.randrange(2**32)
        out += t + struct.pack('<BIqQ', side, qty, price, r.randrange(2**64))
    sys.stdout.buffer.write(out)

main()

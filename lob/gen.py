#!/usr/bin/env python3
# Generate a stream of orders for the matching engine.
# Format per line:  action,id,side,price,qty
#   action: N = new limit, M = market, C = cancel
#   side:   B = buy, S = sell
#   price:  integer cents ($100.55 -> 10055); ignored for M and C
import random
import sys

N = int(sys.argv[1]) if len(sys.argv) > 1 else 1_000_000
random.seed(42)                 # fixed seed -> reproducible benchmark

mid = 10_000                    # start the market at $100.00
next_id = 1
live = []                       # ids we've placed and could cancel later

out = []
for _ in range(N):
    r = random.random()

    # random-walk the mid so bids and asks keep overlapping (-> trades happen)
    mid += random.randint(-2, 2)
    if mid < 100:
        mid = 100

    if r < 0.80:                          # 80% new limit orders
        side  = random.choice('BS')
        price = max(1, mid + random.randint(-10, 10))   # near the mid
        qty   = random.randint(1, 100)
        oid = next_id; next_id += 1
        out.append(f"N,{oid},{side},{price},{qty}")
        live.append(oid)

    elif r < 0.90:                        # 10% market orders
        side = random.choice('BS')
        qty  = random.randint(1, 100)
        oid = next_id; next_id += 1
        out.append(f"M,{oid},{side},0,{qty}")

    else:                                 # 10% cancels of an earlier order
        if live:
            oid = live.pop(random.randrange(len(live)))
            out.append(f"C,{oid},B,0,0")  # side/price ignored by cancel
        # (many of these are stale = already filled; the book treats those
        #  as no-ops, which is realistic.)

sys.stdout.write("\n".join(out))
sys.stdout.write("\n")

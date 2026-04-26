// Sequential await chain. Each outer iteration runs 4 distinct awaits in a row,
// so per-await wall cost is amortized over more continuation shapes than
// async_await_tight_loop.mt.
// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.

import { Int } from "../lib/primitives/Int.mt";

function async stepA(int x): Promise<Int> { return new Int(x + 1); }
function async stepB(int x): Promise<Int> { return new Int(x * 2); }
function async stepC(int x): Promise<Int> { return new Int(x - 1); }
function async stepD(int x): Promise<Int> { return new Int(x ^ 7); }

function async chain(int n): Promise<Int> {
    int total = 0;
    for (int i = 0; i < n; i = i + 1) {
        Int a = await stepA(i);
        Int b = await stepB(a.getValue());
        Int c = await stepC(b.getValue());
        Int d = await stepD(c.getValue());
        total = total + d.getValue();
    }
    return new Int(total);
}

int N = 500000;
Int result = await chain(N);
print("async_await_chain total=" + result.getValue());

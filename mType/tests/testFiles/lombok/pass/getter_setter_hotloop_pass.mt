// JIT/OSR canary: synthesized getters/setters are ordinary methods, so a hot
// loop calling them must compile and produce the same result under --jit and
// --no-jit. Many iterations of the same call sites warm the inline caches and
// trip the OSR threshold; a JIT/interpreter divergence would surface as a wrong
// total.
@Getter
@Setter
@NoArgsConstructor
class Accumulator {
    private int count;
}

Accumulator acc = new Accumulator();
acc.setCount(0);
for (int i = 0; i < 100000; i = i + 1) {
    acc.setCount(acc.getCount() + 1);
}
print(acc.getCount());

// Expected output:
// 100000

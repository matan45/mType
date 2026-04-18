// MYT-164 F-b: callee with a small while loop. Exercises JUMP_BACK
// (loop edge) + JUMP_IF_FALSE (exit guard) inside the inlined body.
// The inliner's per-frame localJumpLabels map both forward (exit) and
// backward (loop head) targets. Correctness check: the accumulated
// sum equals the closed-form 0+1+...+9 = 45 whether the inline path
// fires or the interpreter/generic-dispatch paths run.

class Counter {
    public function countUp(int n): int {
        int s = 0;
        while (s < n) {
            s = s + 1;
        }
        return s;
    }
}

Counter c = new Counter();
int acc = 0;
for (int i = 0; i < 10; i = i + 1) {
    acc = acc + c.countUp(i);
}
print(acc);

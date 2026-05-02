// Regression: OSR must not compile loops containing RETURN_VALUE.
// A loop-local return needs function-return semantics, but the OSR exit path
// resumes the interpreter at a bytecode offset. Keep this loop interpreted
// once it becomes hot so the early return is preserved.

function findFirstAtOrAfter(int limit, int target): int {
    for (int i = 0; i < limit; i = i + 1) {
        if (i == target) {
            return i;
        }
    }
    return -1;
}

int hit = findFirstAtOrAfter(1000, 750);
int miss = findFirstAtOrAfter(1000, 1500);

print("osr_return_value_loop_bailout hit=" + hit + " miss=" + miss);

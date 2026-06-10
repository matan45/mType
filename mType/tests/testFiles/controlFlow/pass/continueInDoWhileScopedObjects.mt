// MYT-383: continue inside a do-while body that allocates an object in a block
// scope each iteration must leave the stack balanced when it jumps to the
// condition (the synthesized STACK_SCOPE_LEAVE delta must match the body's
// open scopes). The i==2 iteration is skipped via continue.
class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function get(): int {
        return this.value;
    }
}

int i = 0;
int sum = 0;
do {
    i = i + 1;
    Box b = new Box(i);
    if (i == 2) {
        continue;
    }
    sum = sum + b.get();
} while (i < 4);
print(sum);
print("done");

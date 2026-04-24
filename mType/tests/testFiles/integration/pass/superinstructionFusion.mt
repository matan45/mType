// MYT-202: Superinstruction fusion correctness test.
// Exercises every compile-time fused opcode through the bytecode pipeline to
// confirm the fused handlers are semantically equivalent to the unfused ones.
//   LOAD_LOAD_ADD_INT  — int c = a + b
//   LOAD_LOAD_SUB_INT  — int c = a - b
//   LOAD_LOAD_MUL_INT  — int c = a * b
//   LOAD_GET_FIELD     — obj.field
//   LOAD_STORE_LOCAL   — int b = a (pure local copy)
//   ADD_INT_STORE_LOCAL— result = a + b + c (second add-then-store)

class Point {
    public int x;
    public int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

function main(): void {
    int a = 7;
    int b = 3;
    int c = 5;

    // Pure local-to-local copy — matches LOAD_STORE_LOCAL.
    int copy = a;
    print("copy = " + copy);

    // LOAD_LOAD_ADD_INT / SUB / MUL
    int sum = a + b;
    int diff = a - b;
    int prod = a * b;
    print("sum = " + sum);
    print("diff = " + diff);
    print("prod = " + prod);

    // Three-term sum — the second ADD_INT + STORE_LOCAL should collapse
    // to ADD_INT_STORE_LOCAL after the first LOAD_LOAD_ADD_INT fires.
    int total = a + b + c;
    print("total = " + total);

    // Field access — LOAD_GET_FIELD
    Point p = new Point(11, 13);
    print("p.x = " + p.x);
    print("p.y = " + p.y);
    int combined = p.x + p.y;
    print("p.x + p.y = " + combined);

    // Hot loop — the fused ops should be hit many times per iteration.
    int acc = 0;
    int i = 0;
    while (i < 5) {
        acc = acc + i;
        i = i + 1;
    }
    print("acc = " + acc);
}
main();

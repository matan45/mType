// Test: an array typed as a functional interface, populated with three
// lambdas, then iterated and applied to a fixed input. Verifies array
// element-of-lambda dispatch through a uniform interface type.

interface IntOp {
    function apply(int x): int;
}

IntOp[] ops = new IntOp[3];
ops[0] = x -> x + 1;
ops[1] = x -> x * 2;
ops[2] = x -> x - 3;

int input = 10;
for (int i = 0; i < ops.length; i = i + 1) {
    print("op " + i + ": " + ops[i].apply(input));
}

print("done");

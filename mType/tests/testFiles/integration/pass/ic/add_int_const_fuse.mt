// MYT-198: PUSH_INT + ADD_INT → ADD_INT_CONST.
// trySpecializeArithmetic promotes ADD → ADD_INT after a few iterations; the
// subsequent tryFuseAddIntConst collapses with the immediately-preceding
// PUSH_INT literal. Output must match pre-fusion arithmetic.

int acc = 0;
for (int i = 0; i < 1000; i = i + 1) {
    acc = acc + 7;
}
print(acc);

int total = 0;
for (int j = 0; j < 500; j = j + 1) {
    total = total + j + 3;
}
print(total);

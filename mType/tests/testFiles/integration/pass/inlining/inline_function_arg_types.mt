// MYT-210: int / float / bool param + return types. Exercises the
// primitive-vs-BOXED dispatch in emitInlineLocalCopy / emitInlineLocalDestroy
// and emitInlineReturnMaterialize for each primitive flavor.

function addInts(int a, int b): int {
    return a + b;
}

function scaleFloat(float x, float s): float {
    return x * s;
}

function negate(bool b): bool {
    return !b;
}

print(addInts(7, 35));         // 42
print(scaleFloat(2.5, 3.0));   // 7.5
print(negate(true));            // false
print(negate(false));           // true

// MYT-204 AC #6: a global mutated after initialization must still reflect
// the new value through the CACHED path. The cached pointer is a
// VariableDefinition*, not a snapshot of the Value at promote time, so
// every read via LOAD_VAR_CACHED re-fetches varDef->getValue() and every
// write via STORE_VAR_CACHED mutates the same slot in place. This test
// drives both: a hot STORE_VAR site (counter = counter + 1) and a hot
// LOAD_VAR read (sum = sum + counter) sharing the same slot.

int counter = 0;
int sum = 0;
for (int i = 0; i < 100; i = i + 1) {
    counter = counter + 1;
    sum = sum + counter;
}
print(counter);
print(sum);

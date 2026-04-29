// Test: a zero-length int array reports length 0 and a for-each over it
// runs the body zero times. Final "done" must always print.

int[] a = new int[0];
print(a.length);

for (int x : a) {
    print(x);
}

print("done");

// MYT-281: regression guard — `arr isClassOf int[]` still works after the
// Object widening (no narrowing of existing isClassOf semantics).
print("Testing array isClassOf self type");

int[] a = new int[1];
a[0] = 42;

if (a isClassOf int[]) {
    print("int[] is int[]: yes");
} else {
    print("int[] is int[]: no");
}

string[] s = new string[1];
if (s isClassOf string[]) {
    print("string[] is string[]: yes");
} else {
    print("string[] is string[]: no");
}

if (s isClassOf int[]) {
    print("string[] is int[]: yes");
} else {
    print("string[] is int[]: no");
}

print("Done");

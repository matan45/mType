// MYT-281: `isClassOf` returns precise booleans for arrays — both
// positive and negative cases. The negation form `!(arr isClassOf T[])`
// is the typical way scripts narrow a downcast guard.
print("Testing array isClassOf negation");

int[] a = new int[1];

if (a isClassOf int[]) {
    print("positive int[]: yes");
}

if (!(a isClassOf string[])) {
    print("negation string[]: ok");
}

if (a isClassOf Object) {
    print("positive Object: yes");
}

string[] s = new string[1];

if (!(s isClassOf int[])) {
    print("negation int[] for string[]: ok");
}

if (s isClassOf string[]) {
    print("positive string[] for string[]: yes");
}

print("Done");

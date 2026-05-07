// MYT-137: strict declaration-time array invariance. `Animal[] a = new
// Dog[1]` errors at compile time — the array-store soundness hole (once
// the Animal[] alias forms, runtime element-type tagging cannot be relied
// on for invariant store-checks) is closed by rejecting the assignment
// before the alias can form. Array literals (`Animal[] = [new Dog()]`)
// remain accepted via target-type-guided inference at the decl site
// (StatementCompiler.cpp ~line 892); explicit runtime casts (`(Animal[])dogs`)
// are unaffected.
class Animal {
    string name;
    constructor(string n) { name = n; }
}

class Dog extends Animal {
    constructor(string n) : super(n) {}
}

Animal[] a = new Dog[1];  // Strict invariance: must error at compile time.

print("This should not be reached");

// PASS: MYT-137 strict invariance still permits covariance via the
// array-literal form, where the declared target type guides per-element
// validation at the assignment site (StatementCompiler ~line 892). The
// pre-MYT-137 variable-to-variable form (`Animal[] a = dogs;`) is now
// rejected — see arrayDeclCovarianceStrict_error.mt for that spec.

class Animal {
    public string name;

    constructor(string n) {
        name = n;
    }
}

class Dog extends Animal {
    public string breed;

    constructor(string n, string b) : super(n) {
        breed = b;
    }
}

function main(): void {
    // Array-literal covariance: each element is validated as Dog is-a
    // Animal, then the literal is retyped as Animal[] for the assignment.
    Animal[] animals = [new Dog("Buddy", "Labrador"), new Dog("Rex", "Husky")];

    print("Covariant literal: " + animals[0].name);
}

main();

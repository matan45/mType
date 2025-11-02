// Arrays + Generics Test 2: Arrays of complex generic types
@Script

class Pair<K, V> {
    field key: K;
    field value: V;

    constructor(k: K, v: V) {
        this.key = k;
        this.value = v;
    }

    fun getKey(): K {
        return this.key;
    }

    fun getValue(): V {
        return this.value;
    }
}

class Triple<A, B, C> {
    field first: A;
    field second: B;
    field third: C;

    constructor(a: A, b: B, c: C) {
        this.first = a;
        this.second = b;
        this.third = c;
    }
}

// Array of Pairs
let pairs: Pair<Int, String>[] = Pair<Int, String>[3];
pairs[0] = Pair<Int, String>(1, "one");
pairs[1] = Pair<Int, String>(2, "two");
pairs[2] = Pair<Int, String>(3, "three");

print("Pairs:");
let i: Int = 0;
while (i < 3) {
    print(pairs[i].getKey());
    print(pairs[i].getValue());
    i = i + 1;
}

// Array of Triples with nested generics
let triples: Triple<Int, Pair<String, Bool>, Float>[] = Triple<Int, Pair<String, Bool>, Float>[2];
triples[0] = Triple<Int, Pair<String, Bool>, Float>(100, Pair<String, Bool>("test", true), 3.14);
triples[1] = Triple<Int, Pair<String, Bool>, Float>(200, Pair<String, Bool>("demo", false), 2.71);

print("Triples:");
i = 0;
while (i < 2) {
    print(triples[i].first);
    print(triples[i].second.getKey());
    print(triples[i].second.getValue());
    print(triples[i].third);
    i = i + 1;
}

// Test compound assignment operators on class member fields

class Counter {
    public int value;
    public float ratio;
    public string label;

    public constructor(int v, float r, string l) {
        value = v;
        ratio = r;
        label = l;
    }
}

class Parent {
    public int score;

    public constructor(int s) {
        score = s;
    }
}

class Child extends Parent {
    public int bonus;

    public constructor(int s, int b) : super(s) {
        bonus = b;
    }

    public function addBonus(int amount): void {
        super.score += amount;
    }

    public function subtractBonus(int amount): void {
        super.score -= amount;
    }
}


function main(): void {
    Counter c = new Counter(10, 2.5, "test");

    // Test += on int member
    c.value += 5;
    print("value after +=: " + c.value);

    // Test -= on int member
    c.value -= 3;
    print("value after -=: " + c.value);

    // Test *= on int member
    c.value *= 2;
    print("value after *=: " + c.value);

    // Test /= on int member
    c.value /= 4;
    print("value after /=: " + c.value);

    // Test %= on int member
    c.value = 17;
    c.value %= 5;
    print("value after %=: " + c.value);

    // Test += on float member
    c.ratio += 1.5;
    print("ratio after +=: " + c.ratio);

    // Test -= on float member
    c.ratio -= 0.5;
    print("ratio after -=: " + c.ratio);

    // Test *= on float member
    c.ratio *= 2.0;
    print("ratio after *=: " + c.ratio);

    // Test /= on float member
    c.ratio /= 3.5;
    print("ratio after /=: " + c.ratio);

    // Test += on string member (concatenation)
    c.label += "_updated";
    print("label after +=: " + c.label);

    // Test compound assignment with this inside a method-like context
    Counter c2 = new Counter(100, 1.0, "counter");
    c2.value += c.value;
    print("c2.value after += c.value: " + c2.value);

    // Test multiple compound operations in sequence
    Counter c3 = new Counter(0, 0.0, "");
    c3.value += 10;
    c3.value -= 3;
    c3.value *= 2;
    c3.value /= 7;
    print("c3 after chain: " + c3.value);

    // Test super member compound assignment
    Child child = new Child(50, 10);
    child.addBonus(20);
    print("score after super +=: " + child.score);

    child.subtractBonus(5);
    print("score after super -=: " + child.score);
}

main();

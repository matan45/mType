// Tier C: variable typed as the BASE class cannot directly call a
// subclass-only method, even though the dynamic value is a subclass
// instance. Should error at compile time.
// Targets: TypeInferenceEngine.cpp:inferMethodCallType + member resolution.

class Animal {
    protected string name;

    public constructor(string n) {
        this.name = n;
    }

    public function getName(): string {
        return this.name;
    }
}

class Dog extends Animal {
    public constructor(string n) : super(n) {}

    public function fetch(): void {
        print(this.name + " is fetching");
    }
}

Animal a = new Dog("Buddy");

// fetch() is Dog-only; calling it through an Animal-typed variable should
// fail compile-time member resolution.
a.fetch();

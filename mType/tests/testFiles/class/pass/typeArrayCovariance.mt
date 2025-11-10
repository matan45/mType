// Test: Array covariance with type checking
// Expected: Pass - demonstrates array type relationships

class Fruit {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }

    public function describe(): void {
        print("Fruit: " + this.name);
    }
}

class Apple extends Fruit {
    private string variety;

    public constructor(string variety) : super("Apple") {
        this.variety = variety;
    }

    public function describe(): void {
        print("Apple variety: " + this.variety);
    }

    public function getVariety(): string {
        return this.variety;
    }
}

class Orange extends Fruit {
    private bool seedless;

    public constructor(bool seedless) : super("Orange") {
        this.seedless = seedless;
    }

    public function describe(): void {
        string type = this.seedless ? "seedless" : "seeded";
        print("Orange: " + type);
    }
}

// Test array covariance
print("Test 1: Fruit array with mixed types");
Fruit[] basket = new Fruit[4];
basket[0] = new Apple("Granny Smith");
basket[1] = new Orange(true);
basket[2] = new Apple("Red Delicious");
basket[3] = new Fruit("Generic");

print("\nTest 2: Iterate and describe");
int i = 0;
while (i < basket.length) {
    print("Item " + i + ":");
    basket[i].describe();
    i = i + 1;
}

print("\nTest 3: Type-specific access");
i = 0;
while (i < basket.length) {
    if (basket[i] isClassOf Apple) {
        Apple a = (Apple)basket[i];
        print("Found apple: " + a.getVariety());
    }
    i = i + 1;
}

print("\nTest 4: Apple array to Fruit array");
Apple[] apples = new Apple[2];
apples[0] = new Apple("Fuji");
apples[1] = new Apple("Gala");

// Covariant assignment
Apple[] moreFruit = apples;
i = 0;
while (i < moreFruit.length) {
    moreFruit[i].describe();
    i = i + 1;
}

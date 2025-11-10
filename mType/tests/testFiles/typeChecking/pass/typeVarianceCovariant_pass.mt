// Test: Covariant return types in method overrides
// Expected: Pass - child class methods can return more specific types

class Food {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public function getName(): string {
        return this.name;
    }

    public function describe(): void {
        print("Food: " + this.name);
    }
}

class Fruit extends Food {
    private string color;

    public constructor(string name, string color) : super(name) {
        this.color = color;
    }

    public function describe(): void {
        print("Fruit: " + this.name + " (" + this.color + ")");
    }

    public function getColor(): string {
        return this.color;
    }
}

class Apple extends Fruit {
    private string variety;

    public constructor(string variety) : super("Apple", "Red") {
        this.variety = variety;
    }

    public function describe(): void {
        print("Apple: " + this.variety);
    }

    public function getVariety(): string {
        return this.variety;
    }
}

// Test covariant return types through factories
class FoodFactory {
    public function createFood(): Food {
        return new Food("Generic Food");
    }

    public function displayFood(): void {
        Food f = this.createFood();
        f.describe();
    }
}

class FruitFactory extends FoodFactory {
    public function createFood(): Fruit {
        return new Fruit("Generic Fruit", "Yellow");
    }

    public function displayFood(): void {
        Fruit f = this.createFood();
        f.describe();
        print("Color: " + f.getColor());
    }
}

class AppleFactory extends FruitFactory {
    public function createFood(): Apple {
        return new Apple("Granny Smith");
    }

    public function displayFood(): void {
        Apple a = this.createFood();
        a.describe();
        print("Variety: " + a.getVariety());
    }
}

// Test 1: Direct covariant return type usage
print("Test 1: Covariant return types");
FoodFactory factory1 = new FoodFactory();
factory1.displayFood();

print("\nTest 2: Fruit factory");
FruitFactory factory2 = new FruitFactory();
factory2.displayFood();

print("\nTest 3: Apple factory");
AppleFactory factory3 = new AppleFactory();
factory3.displayFood();

// Test 4: Polymorphic factory usage
print("\nTest 4: Polymorphic factory array");
FoodFactory[] factories = new FoodFactory[3];
factories[0] = new FoodFactory();
factories[1] = new FruitFactory();
factories[2] = new AppleFactory();

int i = 0;
while (i < 3) {
    print("\nFactory " + i + ":");
    factories[i].displayFood();
    i = i + 1;
}

// Test 5: Covariance with method chaining
class Builder {
    protected string data;

    public constructor() {
        this.data = "";
    }

    public function addData(string d): Builder {
        this.data = this.data + d;
        return this;
    }

    public function build(): string {
        return this.data;
    }
}

class EnhancedBuilder extends Builder {
    public function addData(string d): EnhancedBuilder {
        this.data = this.data + "[" + d + "]";
        return this;
    }

    public function build(): string {
        return "Enhanced: " + this.data;
    }
}

print("\nTest 5: Covariant method chaining");
Builder b1 = new Builder();
string result1 = b1.addData("A").addData("B").build();
print("Builder result: " + result1);

EnhancedBuilder b2 = new EnhancedBuilder();
string result2 = b2.addData("X").addData("Y").build();
print("Enhanced result: " + result2);

print("\nCovariant type tests completed");

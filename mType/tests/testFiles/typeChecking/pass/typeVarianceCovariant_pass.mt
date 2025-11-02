// Test: Covariant return types in method overrides
// Expected: Pass - child class methods can return more specific types

class Food {
    protected string name;

    public constructor(string name) {
        this.name = name;
    }

    public string getName() {
        return this.name;
    }

    public void describe() {
        print("Food: " + this.name);
    }
}

class Fruit extends Food {
    private string color;

    public constructor(string name, string color) : super(name) {
        this.color = color;
    }

    public void describe() {
        print("Fruit: " + this.name + " (" + this.color + ")");
    }

    public string getColor() {
        return this.color;
    }
}

class Apple extends Fruit {
    private string variety;

    public constructor(string variety) : super("Apple", "Red") {
        this.variety = variety;
    }

    public void describe() {
        print("Apple: " + this.variety);
    }

    public string getVariety() {
        return this.variety;
    }
}

// Test covariant return types through factories
class FoodFactory {
    public Food createFood() {
        return new Food("Generic Food");
    }

    public void displayFood() {
        Food f = this.createFood();
        f.describe();
    }
}

class FruitFactory extends FoodFactory {
    public Fruit createFood() {
        return new Fruit("Generic Fruit", "Yellow");
    }

    public void displayFood() {
        Fruit f = this.createFood();
        f.describe();
        print("Color: " + f.getColor());
    }
}

class AppleFactory extends FruitFactory {
    public Apple createFood() {
        return new Apple("Granny Smith");
    }

    public void displayFood() {
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

    public Builder addData(string d) {
        this.data = this.data + d;
        return this;
    }

    public string build() {
        return this.data;
    }
}

class EnhancedBuilder extends Builder {
    public EnhancedBuilder addData(string d) {
        this.data = this.data + "[" + d + "]";
        return this;
    }

    public string build() {
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

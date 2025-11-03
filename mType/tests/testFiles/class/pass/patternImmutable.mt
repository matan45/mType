// Test: Immutable class pattern
// Expected: Pass - demonstrates immutable object design

class ImmutablePoint {
    private final int x;
    private final int y;

    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
        print("Created ImmutablePoint(" + x + ", " + y + ")");
    }

    public function getX(): int {
        return this.x;
    }

    public function getY(): int {
        return this.y;
    }

    // Returns new instance instead of modifying this
    public function add(int dx, int dy): ImmutablePoint {
        print("Creating new point: (" + this.x + "+" + dx + ", " + this.y + "+" + dy + ")");
        return new ImmutablePoint(this.x + dx, this.y + dy);
    }

    public function multiply(int factor): ImmutablePoint{
        print("Creating scaled point: (" + this.x + "*" + factor + ", " + this.y + "*" + factor + ")");
        return new ImmutablePoint(this.x * factor, this.y * factor);
    }

    public function equals(ImmutablePoint other): bool {
        if (other == null) {
            return false;
        }
        ImmutablePoint p = other;
        return this.x == p.x && this.y == p.y;
    }

    public function toString(): string {
        return "(" + this.x + ", " + this.y + ")";
    }
}

class ImmutablePerson {
    private final string name;
    private final int age;
    private final ImmutablePoint location;

    public constructor(string name, int age, ImmutablePoint location) {
        this.name = name;
        this.age = age;
        this.location = location;
        print("Created ImmutablePerson: " + name + ", age " + age);
    }

    public function getName(): string {
        return this.name;
    }

    public function getAge(): int {
        return this.age;
    }

    public function getLocation(): ImmutablePoint {
        return this.location;
    }

    // Returns new instance with different age
    public function withAge(int newAge): ImmutablePerson  {
        print("Creating person with new age: " + newAge);
        return new ImmutablePerson(this.name, newAge, this.location);
    }

    // Returns new instance with different location
    public function withLocation(ImmutablePoint newLocation): ImmutablePerson  {
        print("Creating person with new location");
        return new ImmutablePerson(this.name, this.age, newLocation);
    }

    public function toString(): string {
        return this.name + " (age " + this.age + ") at " + this.location.toString();
    }
}

// Test immutable pattern
print("Test 1: Immutable point operations");
ImmutablePoint p1 = new ImmutablePoint(10, 20);
print("p1: " + p1.toString());

ImmutablePoint p2 = p1.add(5, 3);
print("p2 (p1 + (5,3)): " + p2.toString());
print("p1 unchanged: " + p1.toString());

ImmutablePoint p3 = p1.multiply(2);
print("p3 (p1 * 2): " + p3.toString());
print("p1 still unchanged: " + p1.toString());

print("\nTest 2: Immutable person");
ImmutablePoint loc1 = new ImmutablePoint(100, 200);
ImmutablePerson person1 = new ImmutablePerson("Alice", 30, loc1);
print("person1: " + person1.toString());

print("\nTest 3: Create modified person");
ImmutablePerson person2 = person1.withAge(31);
print("person2: " + person2.toString());
print("person1 unchanged: " + person1.toString());

print("\nTest 4: Change location");
ImmutablePoint loc2 = new ImmutablePoint(300, 400);
ImmutablePerson person3 = person1.withLocation(loc2);
print("person3: " + person3.toString());
print("person1 still unchanged: " + person1.toString());

// Test: Using Object as a type (variable, parameter, return type, array)
// Expected: Pass - Object works as a universal type like in Java

class Animal {
    public string name;

    public constructor(string name) {
        this.name = name;
    }

    public function toString(): string {
        return "Animal:" + this.name;
    }
}

class Dog extends Animal {
    public string breed;

    public constructor(string name, string breed) : super(name) {
        this.breed = breed;
    }

    public function toString(): string {
        return "Dog:" + this.name + "(" + this.breed + ")";
    }
}

// Test 1: Object variable holding any class instance
Object obj1 = new Animal("Cat");
Object obj2 = new Dog("Rex", "Shepherd");
print("obj1: " + obj1.toString());
print("obj2: " + obj2.toString());

// Test 2: Object as function parameter
function describe(Object item): string {
    return "Item: " + item.toString();
}

print(describe(obj1));
print(describe(obj2));

// Test 3: Object as return type
function createSomething(bool isDog): Object {
    if (isDog) {
        return new Dog("Buddy", "Labrador");
    }
    return new Animal("Kitty");
}

Object result1 = createSomething(true);
Object result2 = createSomething(false);
print("result1: " + result1.toString());
print("result2: " + result2.toString());

// Test 4: Object array holding mixed types
Object[] items = new Object[3];
items[0] = new Animal("Fish");
items[1] = new Dog("Max", "Poodle");
items[2] = new Animal("Bird");

for (int i = 0; i < 3; i = i + 1) {
    print("items[" + i + "]: " + items[i].toString());
}

// Test 5: Object methods (equals, hashCode) on Object-typed variable
Object a = new Animal("Cat");
Object b = new Animal("Cat");
print("a equals b: " + a.equals(b));
print("a has hashCode: " + (a.hashCode() != 0));

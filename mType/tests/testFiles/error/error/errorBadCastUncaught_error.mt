// A failing downcast to a sibling subclass without a surrounding
// try/catch must surface as a "Cannot cast" runtime exception.
class Animal { public string name; constructor(string n) { name = n; } }
class Cat extends Animal { constructor(string n) : super(n) {} }
class Dog extends Animal { constructor(string n) : super(n) {} }

function main(): void {
    Animal a = new Dog("Rex");
    Cat c = (Cat)a;
    print(c.name);
}
main();

// PASS: Whole-array covariance (Dog[] -> Animal[]) is allowed.
// A Dog[] can be aliased as an Animal[] since Dog extends Animal.

class Animal {
    string name;
}

class Dog extends Animal {
    string breed;
}

function main(): void {
    Dog[] dogs = new Dog[1];
    dogs[0] = new Dog();
    dogs[0].name = "Buddy";
    dogs[0].breed = "Labrador";

    Animal[] animals = dogs;

    print("Covariant alias: " + animals[0].name);
}

main();

// ERROR: Array covariance violation

class Animal {
    string name;
}

class Dog extends Animal {
    string breed;
}

class Cat extends Animal {
    bool indoor;
}

function main(): void {
    // Create Dog array
    Dog[] dogs = new Dog[2];
    dogs[0] = new Dog();
    dogs[0].name = "Buddy";
    dogs[0].breed = "Labrador";

    // ERROR: Cannot assign Dog[] to Animal[] (array covariance)
    Animal[] animals = dogs;

    // This would be unsafe: animals[1] = new Cat();
    // Would corrupt the dogs array with a Cat!

    print("This should not execute");
}

main();

// ERROR: Lambda contravariance violation

interface Function<T> {
    function apply(T input): T;
}

class Animal {
    string name;
}

class Dog extends Animal {
    string breed;
}

function main(): void {
    // Function that accepts Animal
    Function<Animal> animalFunction = (Animal a) -> {
        a.name = "Modified";
        return a;
    };

    // ERROR: Cannot assign Function<Animal> to Function<Dog> (contravariance)
    Function<Dog> dogFunction = animalFunction;

    print("This should not execute");
}

main();

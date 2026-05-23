// MYT-361 — class implementing a generic interface with a concrete (class)
// type argument must be accepted when its method's parameter type matches
// the substituted interface signature.
class Animal {
    public string name;

    public constructor(string n) {
        this.name = n;
    }
}

class Plant {
    public string species;

    public constructor(string s) {
        this.species = s;
    }
}

interface Predicate<T> {
    function test(T t): bool;
}

class HasNameAnimalPredicate implements Predicate<Animal> {
    public function test(Animal value): bool {
        return value.name != "";
    }
}

class HasSpeciesPlantPredicate implements Predicate<Plant> {
    public function test(Plant value): bool {
        return value.species != "";
    }
}

HasNameAnimalPredicate animalPredicate = new HasNameAnimalPredicate();
print(animalPredicate.test(new Animal("Rex")));
print(animalPredicate.test(new Animal("")));

HasSpeciesPlantPredicate plantPredicate = new HasSpeciesPlantPredicate();
print(plantPredicate.test(new Plant("Oak")));
print(plantPredicate.test(new Plant("")));

print("Concrete generic interface implementation successful");

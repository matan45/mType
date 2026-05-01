// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Exercises isClassOf checks and successful downcasts in a hot loop.

class CastAnimal {
    public function value(): int {
        return 1;
    }
}

class CastDog extends CastAnimal {
    public function value(): int {
        return 3;
    }
}

class CastCat extends CastAnimal {
    public function value(): int {
        return 5;
    }
}

CastAnimal[] animals = [new CastDog(), new CastCat(), new CastAnimal()];

int N = 2000000;
int dogs = 0;
int cats = 0;
int acc = 0;

for (int i = 0; i < N; i = i + 1) {
    CastAnimal animal = animals[i % 3];
    if (animal isClassOf CastDog) {
        CastDog dog = (CastDog)animal;
        dogs = dogs + 1;
        acc = acc + dog.value();
    } else if (animal isClassOf CastCat) {
        CastCat cat = (CastCat)animal;
        cats = cats + 1;
        acc = acc + cat.value();
    } else {
        acc = acc + animal.value();
    }
}

print("cast_hot dogs=" + dogs + " cats=" + cats + " acc=" + acc);

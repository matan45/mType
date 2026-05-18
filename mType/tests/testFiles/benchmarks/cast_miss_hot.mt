// TARGET: ~1-3s on dev machine. Adjust N if first run lands outside that range.
// Companion to cast_hot.mt — most isClassOf checks FAIL here (4-of-5 are
// CastAnimal, 1-of-5 is CastDog). Measures the failure-path cost of
// isClassOf when the inline-cache shape misses (no successful cast to short
// the chain).

class CastAnimal {
    public function value(): int { return 1; }
}

class CastDog extends CastAnimal {
    @Override
    public function value(): int { return 3; }
}

CastAnimal[] animals = [
    new CastAnimal(),
    new CastAnimal(),
    new CastDog(),
    new CastAnimal(),
    new CastAnimal()
];

int N = 2000000;
int dogs = 0;
int acc = 0;
for (int i = 0; i < N; i = i + 1) {
    CastAnimal animal = animals[i % 5];
    if (animal isClassOf CastDog) {
        CastDog d = (CastDog)animal;
        dogs = dogs + 1;
        acc = acc + d.value();
    } else {
        acc = acc + animal.value();
    }
}

print("cast_miss_hot dogs=" + dogs + " acc=" + acc);

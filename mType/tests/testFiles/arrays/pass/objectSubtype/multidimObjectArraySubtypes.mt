// MYT-378: multi-dimensional object arrays (built at runtime as nested 1D
// SoA NativeArrays) must also hold subtypes. Each inner row has 16 elements
// (>= SoA threshold); storing Dog/Cat into an Animal[2][16] must fall back
// to heterogeneous storage per row and keep polymorphic dispatch.
print("Testing multidim subtype array");

class Animal {
    public function makeSound(): string {
        return "...";
    }
}

class Dog extends Animal {
    public function makeSound(): string {
        return "Woof";
    }
}

class Cat extends Animal {
    public function makeSound(): string {
        return "Meow";
    }
}

Animal[][] grid = new Animal[2][16];
for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 16; j++) {
        if (i < 1) {
            grid[i][j] = new Dog();
        } else {
            grid[i][j] = new Cat();
        }
    }
}

print(grid[0][0].makeSound());
print(grid[0][15].makeSound());
print(grid[1][0].makeSound());
print(grid[1][15].makeSound());

print("Done");

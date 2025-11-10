// Test self-returning interface (Builder pattern)
// @Script

interface Builder {
    function withName(string name): Builder;
    function withAge(int age): Builder;
    function build(): Person;
}

class Person {
    private string name;
    private int age;

    public constructor() {
        this.name = "Unknown";
        this.age = 0;
    }

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function display(): void {
        print("Person: " + this.name + ", Age: " + this.age);
    }
}

class PersonBuilder implements Builder {
    private string name;
    private int age;

    public constructor() {
        this.name = "Unknown";
        this.age = 0;
    }

    public function withName(string name): Builder {
        this.name = name;
        return this;  // Return self for chaining
    }

    public function withAge(int age): Builder {
        this.age = age;
        return this;  // Return self for chaining
    }

    public function build(): Person {
        return new Person(this.name, this.age);
    }
}

// Fluent interface / method chaining
Person person = new PersonBuilder()
    .withName("Alice")
    .withAge(30)
    .build();

person.display();

// Another example with different order
Person person2 = new PersonBuilder()
    .withAge(25)
    .withName("Bob")
    .build();

person2.display();

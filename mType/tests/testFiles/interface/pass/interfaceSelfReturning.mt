// Test self-returning interface (Builder pattern)
// @Script

interface Builder {
    func withName(name: String): Builder;
    func withAge(age: Int): Builder;
    func build(): Person;
}

class Person {
    var name: String;
    var age: Int;

    func init(name: String, age: Int) {
        this.name = name;
        this.age = age;
    }

    func display(): void {
        print("Person: " + this.name + ", Age: " + this.age.toString());
    }
}

class PersonBuilder implements Builder {
    var name: String;
    var age: Int;

    func init() {
        this.name = "Unknown";
        this.age = 0;
    }

    func withName(name: String): Builder {
        this.name = name;
        return this;  // Return self for chaining
    }

    func withAge(age: Int): Builder {
        this.age = age;
        return this;  // Return self for chaining
    }

    func build(): Person {
        return new Person(this.name, this.age);
    }
}

// Fluent interface / method chaining
var person = new PersonBuilder()
    .withName("Alice")
    .withAge(30)
    .build();

person.display();

// Another example with different order
var person2 = new PersonBuilder()
    .withAge(25)
    .withName("Bob")
    .build();

person2.display();

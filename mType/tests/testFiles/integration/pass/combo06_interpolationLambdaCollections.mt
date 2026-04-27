// Combo 06: String Interpolation + Lambda + Collections + For-Each
// Tests: interpolation with lambda results over collections

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Formatter {
    function format(string value): string;
}

class Person {
    private string name;
    private int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function getName(): string { return this.name; }
    public function getAge(): int { return this.age; }

    public function toString(): string {
        return $"{this.name} (age {this.age})";
    }
}

function main(): void {
    print("=== Combo 06: Interpolation + Lambda + Collections ===");

    // Lambdas with interpolation
    Formatter upper = value -> $"[{value}]";
    Formatter tagged = value -> $"<tag>{value}</tag>";

    print(upper.format("hello"));
    print(tagged.format("world"));

    // Collection with for-each + interpolation
    print("--- People list ---");
    ArrayList<Person> people = new ArrayList<Person>();
    people.add(new Person("Alice", 30));
    people.add(new Person("Bob", 25));
    people.add(new Person("Charlie", 35));
    people.add(new Person("Diana", 28));

    for (Person p : people) {
        string greeting = $"Hello, {p.getName()}! You are {p.getAge()} years old.";
        print(greeting);
    }

    // Lambda that uses interpolation in its body
    print("--- Formatted output ---");
    Formatter personFormatter = name -> $"*** {name} ***";
    for (Person p : people) {
        print(personFormatter.format(p.getName()));
    }

    // Interpolation with expressions
    print("--- Computed values ---");
    for (Person p : people) {
        int birthYear = 2026 - p.getAge();
        string info = $"{p.getName()} was born around {birthYear}";
        print(info);
    }

    // Nested interpolation with lambda
    print("--- Summary ---");
    int total = people.size();
    int ageSum = 0;
    for (Person p : people) {
        ageSum = ageSum + p.getAge();
    }
    print($"Total people: {total}, average age: {ageSum / total}");

    print("=== Combo 06 Complete ===");
}

main();

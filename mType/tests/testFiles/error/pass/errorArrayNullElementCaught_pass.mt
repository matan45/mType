// Test: calling a method on a null array element throws a runtime
// NullPointerException that is catchable as Exception. The loop
// accumulates a count of non-null hits and a count of caught nulls.
import * from "../../lib/exceptions/Exception.mt";

class Person {
    string name;
    constructor(string n) { name = n; }
    public function describe(): string { return "Person(" + name + ")"; }
}

function main(): void {
    Person[] people = new Person[4];
    people[0] = new Person("Alice");
    people[1] = null;
    people[2] = new Person("Bob");
    people[3] = null;
    int hits = 0;
    int nulls = 0;
    for (int i = 0; i < people.length; i++) {
        try {
            string s = people[i].describe();
            print(s);
            hits++;
        } catch (Exception e) {
            nulls++;
        }
    }
    print("hits=" + hits + " nulls=" + nulls);
}
main();

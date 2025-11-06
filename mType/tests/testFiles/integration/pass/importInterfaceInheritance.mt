// Test: Cross-file interface inheritance
// @Script

import "modules/BaseInterface.mt";
import "modules/DerivedInterface.mt";

class Person implements Nameable {
    private Int id;
    private String name;

    constructor(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    public function getId() : Int {
        return this.id;
    }

    public function getName() : String {
        return this.name;
    }
}

function testInterface(obj: Identifiable) : Int {
    return obj.getId();
}

function testNameable(obj: Nameable) : String {
    return obj.getName() + " (ID: " + obj.getId().toString() + ")";
}

function main() : void {
    Person person = new Person(42, "Charlie");

    Int id = testInterface(person);
    print("ID: " + id.toString());
    assert(id == 42, "Should get ID through base interface");

    String info = testNameable(person);
    print(info);
    assert(info == "Charlie (ID: 42)", "Should get full info through derived interface");
}

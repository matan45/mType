// Test: Cross-file interface inheritance
// @Script

import "modules/BaseInterface.mt";
import "modules/DerivedInterface.mt";

class Person : Nameable {
    private id: Int;
    private name: String;

    constructor(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    getId() : Int {
        return this.id;
    }

    getName() : String {
        return this.name;
    }
}

testInterface(obj: Identifiable) : Int {
    return obj.getId();
}

testNameable(obj: Nameable) : String {
    return obj.getName() + " (ID: " + obj.getId().toString() + ")";
}

main() : Void {
    let person = new Person(42, "Charlie");

    let id = testInterface(person);
    print("ID: " + id.toString());
    assert(id == 42, "Should get ID through base interface");

    let info = testNameable(person);
    print(info);
    assert(info == "Charlie (ID: 42)", "Should get full info through derived interface");
}

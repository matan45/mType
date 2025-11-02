// Test casting from base interface to derived interface
// @Script

interface Base {
    func getId(): Int;
}

interface Extended extends Base {
    func getName(): String;
}

class Implementation implements Extended {
    var id: Int;
    var name: String;

    func init(id: Int, name: String) {
        this.id = id;
        this.name = name;
    }

    func getId(): Int {
        return this.id;
    }

    func getName(): String {
        return this.name;
    }
}

// Create as Extended
var extended: Extended = new Implementation(1, "Test");
print(extended.getId());
print(extended.getName());

// Cast to Base (upcast - always safe)
var base: Base = extended as Base;
print(base.getId());

// Cast back to Extended (downcast - needs runtime check)
if (base instanceof Extended) {
    var extended2: Extended = base as Extended;
    print(extended2.getName());
}

// Test with polymorphic function
func processBase(base: Base): void {
    print("Processing base with id: " + base.getId().toString());

    if (base instanceof Extended) {
        var ext: Extended = base as Extended;
        print("  Name: " + ext.getName());
    } else {
        print("  Not an extended interface");
    }
}

processBase(extended);

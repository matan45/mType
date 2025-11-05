// Test casting from base interface to derived interface
// @Script

interface Base {
    function getId(): int;
}

interface Extended extends Base {
    function getName(): string;
}

class Implementation implements Extended {
    private int id;
    private string name;

    public constructor(int id, string name) {
        this.id = id;
        this.name = name;
    }

    public function getId(): int {
        return this.id;
    }

    public function getName(): string {
        return this.name;
    }
}

// Create as Extended
Extended extended = new Implementation(1, "Test");
print(extended.getId());
print(extended.getName());

// Cast to Base (upcast - always safe)
Base base = (Base)extended;
print(base.getId());

// Cast back to Extended (downcast - needs runtime check)
if (base instanceof Extended) {
    Extended extended2 = (Extended)base;
    print(extended2.getName());
}

// Test with polymorphic function
function processBase(Base base): void {
    print("Processing base with id: " + base.getId());

    if (base instanceof Extended) {
        Extended ext = (Extended)base;
        print("  Name: " + ext.getName());
    } else {
        print("  Not an extended interface");
    }
}

processBase(extended);

// Test file for O2 optimization - Unused Class Dependencies
// Tests removal of classes that are only referenced by other unused classes

// ===== USED CODE (should be kept) =====

class UsedHelper {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

class UsedMain {
    UsedHelper helper;
    string name;

    constructor(string n, int val) {
        this.name = n;
        this.helper = new UsedHelper(val);
    }

    public function getInfo(): string {
        return this.name + ": " + this.helper.getValue();
    }
}

// ===== UNUSED CODE WITH DEPENDENCIES (should all be removed) =====

// Helper class only used by UnusedMain
class UnusedHelper {
    string data;

    constructor(string d) {
        this.data = d;
    }

    function getData(): string {
        return this.data;
    }
}

// Another helper only used by UnusedMain
class UnusedData {
    int count;

    constructor(int c) {
        this.count = c;
    }

    function getCount(): int {
        return this.count;
    }
}

// Main unused class that references the helpers above
class UnusedMain {
    UnusedHelper helper;
    UnusedData data;
    string label;

    constructor(string l) {
        this.label = l;
        this.helper = new UnusedHelper("test");
        this.data = new UnusedData(42);
    }

    function getLabel(): string {
        return this.label;
    }
}

// Completely isolated unused class
class IsolatedUnused {
    int x;

    constructor(int val) {
        this.x = val;
    }
}

// Chain of unused classes: C -> B -> A
class UnusedA {
    int value;

    constructor(int v) {
        this.value = v;
    }
}

class UnusedB {
    UnusedA a;

    constructor(int v) {
        this.a = new UnusedA(v);
    }
}

class UnusedC {
    UnusedB b;

    constructor(int v) {
        this.b = new UnusedB(v);
    }
}

// ===== ENTRY POINT CODE =====

// Only use UsedMain and UsedHelper
UsedMain main = new UsedMain("Test", 100);
print(main.getInfo());

// Note: We never use:
// - UnusedHelper (only referenced by UnusedMain)
// - UnusedData (only referenced by UnusedMain)
// - UnusedMain (not used at all)
// - IsolatedUnused (not used at all)
// - UnusedA, UnusedB, UnusedC (chain of unused classes)

print("Unused Class Dependencies Test Complete!");

// Expected O2 optimization results:
// - Keep: UsedMain, UsedHelper (both used)
// - Remove: UnusedHelper, UnusedData, UnusedMain (all unused)
// - Remove: IsolatedUnused (unused)
// - Remove: UnusedA, UnusedB, UnusedC (all unused, even though they reference each other)
//
// Total: Should remove 7 classes

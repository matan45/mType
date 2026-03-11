// Test: Mixed reachability - some objects in cycles, some still reachable
// Purpose: Verify GC correctly distinguishes reachable objects from unreachable cycles

class Item {
    private string name;
    private Item link;

    constructor(string n) {
        this.name = n;
        this.link = null;
    }

    public function setLink(Item other): void {
        this.link = other;
    }

    public function getName(): string {
        return this.name;
    }
}

function createUnreachableCycle(): void {
    // These two items form a cycle and become unreachable when function returns
    Item x = new Item("Unreachable1");
    Item y = new Item("Unreachable2");
    x.setLink(y);
    y.setLink(x);
    print("Unreachable cycle created inside function");
}

function main(): void {
    // This item remains reachable in outer scope
    Item reachable = new Item("Reachable");

    // Create an unreachable cycle inside a function call
    createUnreachableCycle();

    // Verify the reachable item is still intact
    print(reachable.getName());
    print("Mixed reachability test passed");
}
main();

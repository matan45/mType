// Test: resource pattern - try opens, await throws, catch handles, finally cleans up
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/primitives/Int.mt";

class Resource {
    string name;
    bool open;

    public constructor(string n) {
        this.name = n;
        this.open = false;
    }

    public function openIt(): void {
        this.open = true;
        print("open");
    }

    public function close(): void {
        this.open = false;
        print("cleanup");
    }
}

function async failingWork(): Promise<Int> {
    print("await");
    throw new Exception("work failed");
    return new Int(0);
}

function async runTest(): Promise<Int> {
    Resource r = new Resource("r");
    Int value = new Int(-1);
    try {
        r.openIt();
        value = await failingWork();
        print("unreachable");
    } catch (Exception e) {
        print("caught");
        value = new Int(0);
    } finally {
        r.close();
    }
    return value;
}

function async main(): Promise<void> {
    Int r = await runTest();
    print("done: " + r.getValue());
}

main();

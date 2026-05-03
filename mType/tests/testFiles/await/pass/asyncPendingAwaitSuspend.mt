// Regression test for MYT-265.
// Forces AWAIT to observe a pending AsyncPromiseValue from script code via
// delay(ms). Pins the resolve sub-path of executeAwait's suspend branch.

import { Int } from "../../lib/primitives/Int.mt";

print("=== Pending Await Suspend ===");

class Payload {
    public string name;
    public Int count;

    public constructor(string name, Int count) {
        this.name = name;
        this.count = count;
    }
}

function async main(): Promise<Int> {
    print("main: before delayed await");
    await delay(1);
    Payload payload = new Payload("custom", new Int(42));
    print("main: after delayed await name=" + payload.name + " value=" + payload.count);
    print("OK");
    return payload.count;
}

main();

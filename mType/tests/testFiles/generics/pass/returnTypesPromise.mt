import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";


function async createIntPromise(): Promise<Int> {
    return new Int(999);
}

function async createStringPromise(): Promise<String> {
    return new String("promised");
}

// Test functions
Int p1 = await createIntPromise();
print(p1.toString());

String p2 = await createStringPromise();
print(p2.toString());

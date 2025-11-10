// Test exception with complex async call stack

import { Int } from "../../lib/primitives/Int.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Complex Call Stack Exception Test ===");

class StackException extends Exception {
   
    public constructor(string loc): super(loc) {  }
    
}

class Value {
    int num;
    public constructor(int n) { this.num = n; }
    public function getNum(): int { return this.num; }
}

function async deepFunction5(): Promise<Value> {
    print("Deep 5");
    throw new StackException("Deep level 5");
    Value v = new Value(5);
    return v;
}

function async deepFunction4(): Promise<Value> {
    print("Deep 4");
    Value v = await deepFunction5();
    return v;
}

function async deepFunction3(): Promise<Value> {
    print("Deep 3");
    Value v = await deepFunction4();
    return v;
}

function async deepFunction2(): Promise<Value> {
    print("Deep 2");
    Value v = await deepFunction3();
    return v;
}

function async deepFunction1(): Promise<Value> {
    print("Deep 1");
    Value v;
    try {
        v = await deepFunction2();
    } catch (StackException e) {
        print("Caught at deep 1: " + e.getMessage());
        v = new Value(888);
    }
    return v;
}

function async main(): Promise<Int> {
    Value v = await deepFunction1();
    print("Final value: " + v.getNum());
    return new Int(v.getNum());
}

main();

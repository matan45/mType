import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";
import * from "../../../lib/primitives/Bool.mt";

// Test lambda return type inference

// Function that takes a lambda and applies it
function applyInt(function(Int): Int lambda, Int value): Int {
    return lambda(value);
}

function applyString(function(String): String lambda, String value): String {
    return lambda(value);
}

function applyBool(function(Bool): Bool lambda, Bool value): Bool {
    return lambda(value);
}

// Function that returns a lambda - return type inferred
function getDoubler(): function(Int): Int {
    return (Int x): Int => {
        Int two = new Int(2);
        return new Int(x.getValue() * two.getValue());
    };
}

function getGreeter(): function(String): String {
    return (String name): String => {
        return new String("Hello, " + name.getValue());
    };
}

function getNegator(): function(Bool): Bool {
    return (Bool b): Bool => {
        return new Bool(!b.getValue());
    };
}

function main(): void {
    print("Testing lambda return type inference");

    // Test lambda with Int return type inference
    Int result1 = applyInt((Int x): Int => {
        Int three = new Int(3);
        return new Int(x.getValue() * three.getValue());
    }, new Int(10));
    print("Lambda triple(10): " + result1);

    // Test lambda with String return type inference
    String result2 = applyString((String s): String => {
        return new String(s.getValue() + " World");
    }, new String("Hello"));
    print("Lambda concat(Hello): " + result2);

    // Test lambda with Bool return type inference
    Bool result3 = applyBool((Bool b): Bool => {
        return b;
    }, new Bool(true));
    print("Lambda identity(true): " + result3);

    // Test function returning lambda with inferred type
    function(Int): Int doubler = getDoubler();
    Int doubled = doubler(new Int(21));
    print("Doubler(21): " + doubled);

    // Test function returning String lambda
    function(String): String greeter = getGreeter();
    String greeting = greeter(new String("mType"));
    print("Greeter(mType): " + greeting);

    // Test function returning Bool lambda
    function(Bool): Bool negator = getNegator();
    Bool negated = negator(new Bool(false));
    print("Negator(false): " + negated);

    // Test nested lambda return type inference
    Int result4 = applyInt((Int x): Int => {
        return applyInt((Int y): Int => {
            return new Int(y.getValue() + new Int(1).getValue());
        }, x);
    }, new Int(5));
    print("Nested lambda increment(5): " + result4);

    print("Lambda return type inference test completed");
}

main();

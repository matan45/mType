// KNOWN-BUG MYT-137 prerequisite — DO NOT REGISTER until MYT-137 lands.
//
// Strict declaration-time invariance: `Animal[] a = new Dog[1]` must error
// at compile time. Today it is intentionally allowed (see
// arrays/pass/arrayCovariance.mt) because mType follows the Java model
// where covariance is allowed at the declaration and invariance is enforced
// at the array-store. Flipping this to strict-invariance is the MYT-137
// design decision; this test file is the spec.
//
// This file is committed but unregistered. When MYT-137 lands, register it
// in ArrayTestSuite::setupTests() as ERROR_EXPECTED.
class Animal {
    string name;
    constructor(string n) { name = n; }
}

class Dog extends Animal {
    constructor(string n) : super(n) {}
}

Animal[] a = new Dog[1];  // Strict invariance: must error at compile time.

print("This should not be reached");

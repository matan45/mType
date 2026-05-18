// Diamond-interface ambiguity: a class implements two unrelated interfaces, each
// with its own overload of the same function. Both candidates match at distance 1.
// Companion to the passing nullAmbiguityTwoClasses test (which uses null).
interface Walker {}
interface Swimmer {}

class Duck implements Walker, Swimmer {
    constructor() {}
}

function describe(Walker w): string {
    return "walks";
}

function describe(Swimmer s): string {
    return "swims";
}

function main(): void {
    Duck d = new Duck();
    // ERROR: ambiguous call to 'describe' - Duck is both Walker and Swimmer at distance 1
    print(describe(d));
}

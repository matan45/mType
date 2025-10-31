// ERROR: Method cannot be both final and abstract

abstract class Base {
    // ERROR: Cannot declare a method as both final and abstract
    // Final means "cannot be overridden"
    // Abstract means "must be overridden"
    // These are contradictory
    public abstract final function process(): string;
}

function main(): void {
    print("This should not execute");
}

main();

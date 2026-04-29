// ERROR: Interface methods cannot be declared final.
// Interface methods are abstract by default and must be overridable
// by implementing classes.

interface I {
    final function foo(): void;
}

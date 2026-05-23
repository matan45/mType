// MYT-360: primitive type rejected in `interface I extends Parent<primitive>`.

interface List<T> {
    function add(T value): void;
}

interface BoolList extends List<bool> {
    function size(): int;
}

function main(): void {
    print("unreachable - parse should fail");
}

main();

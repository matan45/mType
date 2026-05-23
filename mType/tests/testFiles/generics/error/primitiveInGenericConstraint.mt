// MYT-360: primitive type rejected inside a generic constraint's type arg
// — `<T extends Bar<primitive>>`.

interface Bar<T> {
    function process(T value): void;
}

class Foo<T extends Bar<string>> {
    T worker;

    constructor(T w) {
        this.worker = w;
    }
}

function main(): void {
    print("unreachable - parse should fail");
}

main();

// Generic class with multiple type parameters
class Pair<T, U> {
    T first;
    U second;

    function setFirst(T value): void {
        first = value;
    }

    function setSecond(U value): void {
        second = value;
    }

    function getFirst(): T {
        return first;
    }

    function getSecond(): U {
        return second;
    }
}

function main(): void {
    Pair<int, string> pair = new Pair<int, string>();
    pair.setFirst(100);
    pair.setSecond("Hello");

    int firstValue = pair.getFirst();
    string secondValue = pair.getSecond();

    print("First: " + firstValue);
    print("Second: " + secondValue);
}

main();
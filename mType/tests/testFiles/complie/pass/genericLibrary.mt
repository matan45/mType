// Generic library file for import testing
class SimpleContainer<T> {
    T item;
    bool hasItem;

    constructor() {
        hasItem = false;
    }

    function store(T newItem): void {
        item = newItem;
        hasItem = true;
    }

    function retrieve(): T {
        if (hasItem) {
            return item;
        }
        return null;
    }

    function isEmpty(): bool {
        return !hasItem;
    }

    function clear(): void {
        hasItem = false;
    }
}

class Validator<T> {
    function isValid(T value): bool {
        return value != null;
    }

    function validateAndProcess(T value): string {
        if (value != null) {
            return "Valid: " + value;
        } else {
            return "Invalid value";
        }
    }
}
class StringProcessor {
    public string data;

    public constructor(string initial) {
        data = initial;
    }

    public function append(string suffix): void {
        data = data + suffix;
    }

    public function prepend(string prefix): void {
        data = prefix + data;
    }

    public function getData(): string {
        return data;
    }

    public function getLength(): int {
        // Simulated length (would need actual string length function)
        return 10;
    }
}

StringProcessor sp = new StringProcessor("Hello");
sp.append(" World");
print(sp.getData()); // Hello World

sp.prepend("Say: ");
print(sp.getData()); // Say: Hello World

print(sp.getLength()); // 10 (simulated)
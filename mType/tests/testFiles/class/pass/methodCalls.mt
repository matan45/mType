class StringProcessor {
    string data;
    
    constructor(string initial) {
        data = initial;
    }
    
    function append(string suffix): void {
        data = data + suffix;
    }
    
    function prepend(string prefix): void {
        data = prefix + data;
    }
    
    function getData(): string {
        return data;
    }
    
    function getLength(): int {
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
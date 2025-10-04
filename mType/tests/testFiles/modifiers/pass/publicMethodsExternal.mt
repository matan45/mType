// Test: Public methods accessible from external context
class Greeter {
    public function greet(string name): string {
        return "Hello, " + name;
    }

    public function farewell(string name): string {
        return "Goodbye, " + name;
    }
}

// External calls to public methods
Greeter g = new Greeter();
print(g.greet("World"));      // Expected: Hello, World
print(g.farewell("Friend"));  // Expected: Goodbye, Friend

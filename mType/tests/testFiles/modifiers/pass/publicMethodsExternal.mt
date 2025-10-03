// Test: Public methods accessible from external context
class Greeter {
    public string greet(string name) {
        return "Hello, " + name;
    }

    public string farewell(string name) {
        return "Goodbye, " + name;
    }
}

// External calls to public methods
Greeter g = new Greeter();
print(g.greet("World"));      // Expected: Hello, World
print(g.farewell("Friend"));  // Expected: Goodbye, Friend

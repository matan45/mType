// Empty lambda body test - functional interface with no statements
interface Runnable {
    function run(): void;
}

print("=== Empty Lambda Body Test ===");

Runnable r = () -> {};
r.run();

print("Done");

// Runnable interface lambda test
interface Runnable {
    function run() : void;
}

print("=== Runnable Interface Test ===");

Runnable task1 = () -> print("Hello from task1!");
Runnable task2 = () -> print("Greetings from task2!");

task1.run();
task2.run();
// Comprehensive iteration test for all collection types

// Test Array iteration
Array<int> numbers = new Array<int>();
numbers.add(1);
numbers.add(2);
numbers.add(3);

print("Array iteration:");
for (int num : numbers) {
    print(num);
}

// Test Set iteration
Set<string> fruits = new Set<string>();
fruits.add("apple");
fruits.add("banana");
fruits.add("cherry");

print("Set iteration:");
for (string fruit : fruits) {
    print(fruit);
}

// Test Stack iteration (bottom to top)
Stack<float> prices = new Stack<float>();
prices.push(1.50);
prices.push(2.25);
prices.push(3.75);

print("Stack iteration:");
for (float price : prices) {
    print(price);
}

// Test Queue iteration (front to back)
Queue<bool> flags = new Queue<bool>();
flags.enqueue(true);
flags.enqueue(false);
flags.enqueue(true);

print("Queue iteration:");
for (bool flag : flags) {
    print(flag);
}

// Test Map iteration (over values)
Map<string, int> ages = new Map<string, int>();
ages.put("Alice", 25);
ages.put("Bob", 30);
ages.put("Charlie", 35);

print("Map iteration:");
for (int age : ages) {
    print(age);
}

print("Comprehensive iteration test passed");
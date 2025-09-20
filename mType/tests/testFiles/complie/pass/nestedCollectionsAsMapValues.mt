// Test nested collections as Map values
class Person {
    string name;
    int age;
    
    constructor(string n, int a) {
        name = n;
        age = a;
    }
    
    function getName(): string {
        return name;
    }
    
    function getAge(): int {
        return age;
    }
}

function testNestedCollectionsInMap(): void {
    print("=== Testing Nested Collections as Map Values ===");
    
    // Test 1: Map<string, Array<int>>
    print("\n1. Testing Map<string, Array<int>>");
    Map<string, Array<int>> numbersByCategory = new Map<string, Array<int>>();
    
    Array<int> evenNumbers = new Array<int>();
    evenNumbers.add(2);
    evenNumbers.add(4);
    evenNumbers.add(6);
    
    Array<int> oddNumbers = new Array<int>();
    oddNumbers.add(1);
    oddNumbers.add(3);
    oddNumbers.add(5);
    
    numbersByCategory.put("even", evenNumbers);
    numbersByCategory.put("odd", oddNumbers);
    
    print("Map size: " + numbersByCategory.size());
    
    Array<int> retrievedEven = numbersByCategory.get("even");
    print("Even numbers count: " + retrievedEven.size());
    for (int i = 0; i < retrievedEven.size(); i++) {
        print("Even[" + i + "]: " + retrievedEven.get(i));
    }
    
    // Test 2: Map<string, Set<Person>>
    print("\n2. Testing Map<string, Set<Person>>");
    Map<string, Set<Person>> peopleByDepartment = new Map<string, Set<Person>>();
    
    Set<Person> engineers = new Set<Person>();
    engineers.add(new Person("Alice", 30));
    engineers.add(new Person("Bob", 28));
    
    Set<Person> managers = new Set<Person>();
    managers.add(new Person("Charlie", 45));
    managers.add(new Person("Diana", 38));
    
    peopleByDepartment.put("Engineering", engineers);
    peopleByDepartment.put("Management", managers);
    
    print("Department count: " + peopleByDepartment.size());
    
    Set<Person> retrievedEngineers = peopleByDepartment.get("Engineering");
    print("Engineers count: " + retrievedEngineers.size());
    
    // Test 3: Map<int, Stack<string>>
    print("\n3. Testing Map<int, Stack<string>>");
    Map<int, Stack<string>> tasksByPriority = new Map<int, Stack<string>>();
    
    Stack<string> highPriorityTasks = new Stack<string>();
    highPriorityTasks.push("Fix critical bug");
    highPriorityTasks.push("Deploy hotfix");
    
    Stack<string> lowPriorityTasks = new Stack<string>();
    lowPriorityTasks.push("Update documentation");
    lowPriorityTasks.push("Refactor legacy code");
    
    tasksByPriority.put(1, highPriorityTasks);
    tasksByPriority.put(3, lowPriorityTasks);
    
    print("Priority levels: " + tasksByPriority.size());
    
    Stack<string> highPriority = tasksByPriority.get(1);
    print("High priority tasks: " + highPriority.size());
    print("Next task: " + highPriority.top());
    
    // Test 4: Map<string, Queue<int>>
    print("\n4. Testing Map<string, Queue<int>>");
    Map<string, Queue<int>> ordersByStatus = new Map<string, Queue<int>>();
    
    Queue<int> pendingOrders = new Queue<int>();
    pendingOrders.enqueue(101);
    pendingOrders.enqueue(102);
    pendingOrders.enqueue(103);
    
    Queue<int> processingOrders = new Queue<int>();
    processingOrders.enqueue(201);
    processingOrders.enqueue(202);
    
    ordersByStatus.put("pending", pendingOrders);
    ordersByStatus.put("processing", processingOrders);
    
    print("Order status types: " + ordersByStatus.size());
    
    Queue<int> pending = ordersByStatus.get("pending");
    print("Pending orders: " + pending.size());
    print("First pending order: " + pending.front());
    
    print("\n=== All nested collection tests completed! ===");
}

testNestedCollectionsInMap();
// Map with objects test

class Employee {
    string name;
    int salary;
    
    constructor(string n, int s) {
        name = n;
        salary = s;
    }
    
    function getName(): string {
        return name;
    }
    
    function getSalary(): int {
        return salary;
    }
}

// Create map with string keys and Employee objects as values
Map<string, Employee> employees = new Map<string, Employee>();

// Add employees
Employee alice = new Employee("Alice Johnson", 75000);
Employee bob = new Employee("Bob Smith", 82000);
Employee charlie = new Employee("Charlie Brown", 68000);

employees.put("emp001", alice);
employees.put("emp002", bob);
employees.put("emp003", charlie);

print(employees.size()); // 3

// Retrieve employees
Employee emp1 = employees.get("emp001");
print(emp1.getName());   // "Alice Johnson"
print(emp1.getSalary()); // 75000

Employee emp2 = employees.get("emp002");
print(emp2.getName());   // "Bob Smith"
print(emp2.getSalary()); // 82000

// Test contains
print(employees.containsKey("emp001")); // true
print(employees.containsKey("emp999")); // false

// Update employee
Employee newAlice = new Employee("Alice Williams", 85000);
employees.put("emp001", newAlice); // Replace existing

Employee updatedEmp = employees.get("emp001");
print(updatedEmp.getName());   // "Alice Williams"
print(updatedEmp.getSalary()); // 85000

print("Map objects test passed");
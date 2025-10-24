// Test: Abstract class with mix of abstract and concrete methods
// Abstract classes can have both abstract and concrete methods

abstract class Employee {
    private string name;
    private float salary;

    constructor(string n, float s) {
        name = n;
        salary = s;
    }

    // Concrete methods
    public function getName(): string {
        return name;
    }

    public function getSalary(): float {
        return salary;
    }

    public function setSalary(float s): void {
        salary = s;
    }

    // Abstract method - must be implemented by subclasses
    abstract function calculateBonus(): float;

    // Concrete method using abstract method
    public function getTotalCompensation(): float {
        return salary + calculateBonus();
    }
}

class Manager extends Employee {
    private int teamSize;

    constructor(string n, float s, int size) : super(n, s) {
        teamSize = size;
    }

    public function calculateBonus(): float {
        // Managers get 10% bonus + 1% per team member
        float baseSalary = super.getSalary();
        return baseSalary * 0.10 + (baseSalary * 0.01 * teamSize);
    }
}

class Developer extends Employee {
    private int projectsCompleted;

    constructor(string n, float s, int projects) : super(n, s) {
        projectsCompleted = projects;
    }

    public function calculateBonus(): float {
        // Developers get 5% bonus + $1000 per project
        return super.getSalary() * 0.05 + (1000.0 * projectsCompleted);
    }
}

// Test the implementation
Manager mgr = new Manager("Alice", 100000.0, 5);
print("Manager: " + mgr.getName());
print("Salary: " + mgr.getSalary());
print("Bonus: " + mgr.calculateBonus());
print("Total: " + mgr.getTotalCompensation());

Developer dev = new Developer("Bob", 80000.0, 3);
print("Developer: " + dev.getName());
print("Salary: " + dev.getSalary());
print("Bonus: " + dev.calculateBonus());
print("Total: " + dev.getTotalCompensation());

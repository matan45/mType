class Employee {
    string name;
    int salary;
    
    constructor(string n, int s) {
        name = n;
        salary = s;
    }
}

function giveSalaryRaise(Employee emp, int raise): void {
    emp.salary = emp.salary + raise;
}

function compareEmployees(Employee e1, Employee e2): bool {
    return e1.salary > e2.salary;
}

function swapEmployees(Employee e1, Employee e2): void {
    // This swaps references, not the objects themselves
    Employee temp = e1;
    e1 = e2;
    e2 = temp;
}

Employee emp1 = new Employee("Alice", 50000);
Employee emp2 = new Employee("Bob", 60000);

print(emp1.salary); // 50000
giveSalaryRaise(emp1, 5000);
print(emp1.salary); // 55000

bool higherPaid = compareEmployees(emp2, emp1);
print(higherPaid); // true

// Note: swap won't work as expected due to pass-by-value of references
swapEmployees(emp1, emp2);
print(emp1.name); // Still Alice
print(emp2.name); // Still Bob
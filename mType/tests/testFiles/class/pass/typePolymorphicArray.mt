// Test: Polymorphic arrays with type operations
// Expected: Pass - demonstrates polymorphic array handling

class Employee {
    protected string name;
    protected int id;

    public constructor(string name, int id) {
        this.name = name;
        this.id = id;
    }

    public function work(): void {
        print("Employee " + this.name + " is working");
    }

    public function getInfo(): string {
        return "ID: " + this.id + ", Name: " + this.name;
    }
}

class Manager extends Employee {
    private int teamSize;

    public constructor(string name, int id, int teamSize) : super(name, id) {
        this.teamSize = teamSize;
    }

    public function work(): void {
        print("Manager " + this.name + " is managing team of " + this.teamSize);
    }

    public function holdMeeting(): void {
        print("Manager " + this.name + " is holding a meeting");
    }
}

class Developer extends Employee {
    private string language;

    public constructor(string name, int id, string language) : super(name, id) {
        this.language = language;
    }

    public function work(): void {
        print("Developer " + this.name + " is coding in " + this.language);
    }

    public function debug(): void {
        print("Developer " + this.name + " is debugging");
    }
}

// Test polymorphic arrays
print("Test 1: Create mixed employee array");
Employee[] team = new Employee[4];
team[0] = new Manager("Alice", 1, 5);
team[1] = new Developer("Bob", 2, "Java");
team[2] = new Developer("Charlie", 3, "Python");
team[3] = new Employee("Dave", 4);

print("\nTest 2: Polymorphic iteration");
int i = 0;
while (i < team.length) {
    team[i].work();
    i = i + 1;
}

print("\nTest 3: Type-specific operations");
i = 0;
while (i < team.length) {
    print("Processing employee " + i + ":");
    if (team[i] isClassOf Manager) {
        Manager m = (Manager)team[i];
        m.holdMeeting();
    } else if (team[i] isClassOf Developer) {
        Developer d = (Developer)team[i];
        d.debug();
    } else {
        print("Generic employee: " + team[i].getInfo());
    }
    i = i + 1;
}

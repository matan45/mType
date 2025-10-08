// Test: Global functions with generic interface constraints
// Expected: Should compile and run successfully
import "../../lib/collections/List.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

interface Hashable {
    function hash(): int;
}

class Student implements Comparable<Student> {
    private string name;
    private int grade;

    constructor(string n, int g) {
        this.name = n;
        this.grade = g;
    }

    public function compareTo(Student other): int {
        if (this.grade < other.grade) {
            return -1;
        } else if (this.grade > other.grade) {
            return 1;
        }
        return 0;
    }

    public function getName(): string {
        return this.name;
    }

    public function getGrade(): int {
        return this.grade;
    }
}

// Global function with constraint
function <T extends Comparable<T>> max(T a, T b): T {
    if (a.compareTo(b) > 0) {
        return a;
    }
    return b;
}

// Global function with constraint
function <T extends Comparable<T>> min(T a, T b): T {
    if (a.compareTo(b) < 0) {
        return a;
    }
    return b;
}

// Global function with constraint and multiple parameters
function <T extends Comparable<T>> isSorted(List<T> items): bool {
    for (int i = 0; i < items.size() - 1; i = i + 1) {
        T current = items.get(i);
        T next = items.get(i + 1);
        if (current.compareTo(next) > 0) {
            return false;
        }
    }
    return true;
}

// Test global functions with constraints
Student alice = new Student("Alice", 85);
Student bob = new Student("Bob", 92);
Student charlie = new Student("Charlie", 78);

Student topStudent = max<Student>(alice, bob);
Student bottomStudent = min<Student>(alice, charlie);

print("Top student: " + topStudent.getName() + " (Grade: " + topStudent.getGrade() + ")");
print("Bottom student: " + bottomStudent.getName() + " (Grade: " + bottomStudent.getGrade() + ")");

List<Student> students = new List<Student>();
students.add(charlie);
students.add(alice);
students.add(bob);

bool sorted = isSorted<Student>(students);
print("Is list sorted? " + sorted);

print("Constrained global functions test passed!");

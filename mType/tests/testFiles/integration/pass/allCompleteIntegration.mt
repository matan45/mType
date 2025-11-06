// All Three Test 4: Complete integration scenario
@Script

interface Comparable<T> {
    function compareTo(other: T): Int;
}

class Student implements Comparable<Student> {
    Int id;
    String name;
    Float grade;

    constructor(studentId: Int, studentName: String, studentGrade: Float) {
        this.id = studentId;
        this.name = studentName;
        this.grade = studentGrade;
    }

    function getId(): Int {
        return this.id;
    }

    function getName(): String {
        return this.name;
    }

    function getGrade(): Float {
        return this.grade;
    }

    function compareTo(other: Student): Int {
        if (this.grade < other.grade) {
            return -1;
        } else if (this.grade > other.grade) {
            return 1;
        }
        return 0;
    }

    function display(): void {
        print(this.id);
        print(this.name);
        print(this.grade);
    }
}

class Course<T extends Comparable<T>> {
    String name;
    T[] students;
    Int count;
    Int capacity;

    constructor(courseName: String, maxStudents: Int) {
        this.name = courseName;
        this.capacity = maxStudents;
        this.students = T[maxStudents];
        this.count = 0;
    }

    function enroll(student: T): Bool {
        if (this.count >= this.capacity) {
            return false;
        }
        this.students[this.count] = student;
        this.count = this.count + 1;
        return true;
    }

    function getStudent(index: Int): T {
        return this.students[index];
    }

    function getCount(): Int {
        return this.count;
    }

    function getName(): String {
        return this.name;
    }

    function getTopStudents(n: Int): T[] {
        this.sortStudents();
        T[] result = T[n];
        Int i = 0;
        while (i < n) {
            result[i] = this.students[this.count - 1 - i];
            i = i + 1;
        }
        return result;
    }

    function sortStudents(): void {
        Int i = 0;
        while (i < this.count - 1) {
            Int j = 0;
            while (j < this.count - i - 1) {
                if (this.students[j].compareTo(this.students[j + 1]) > 0) {
                    T temp = this.students[j];
                    this.students[j] = this.students[j + 1];
                    this.students[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }

    function calculateAverage(): Float {
        Float sum = 0.0;
        Int i = 0;
        while (i < this.count) {
            sum = sum + this.students[i].getGrade();
            i = i + 1;
        }
        return sum / this.count;
    }

    function listAll(): void {
        Int i = 0;
        while (i < this.count) {
            this.students[i].display();
            i = i + 1;
        }
    }
}

class Department {
    Course<Student>[] courses;
    Int courseCount;

    constructor(maxCourses: Int) {
        this.courses = Course<Student>[maxCourses];
        this.courseCount = 0;
    }

    function addCourse(course: Course<Student>): void {
        this.courses[this.courseCount] = course;
        this.courseCount = this.courseCount + 1;
    }

    function getCourse(index: Int): Course<Student> {
        return this.courses[index];
    }

    function getTotalStudents(): Int {
        Int total = 0;
        Int i = 0;
        while (i < this.courseCount) {
            total = total + this.courses[i].getCount();
            i = i + 1;
        }
        return total;
    }

    function listAllCourses(): void {
        Int i = 0;
        while (i < this.courseCount) {
            print(this.courses[i].getName());
            print(this.courses[i].getCount());
            i = i + 1;
        }
    }
}

print("Creating Computer Science Department:");
Department cs = Department(3);

print("Creating courses:");
Course<Student> algo = Course<Student>("Algorithms", 4);
Course<Student> db = Course<Student>("Databases", 3);

print("Enrolling students in Algorithms:");
print(algo.enroll(Student(101, "Alice", 95.5)));
print(algo.enroll(Student(102, "Bob", 87.0)));
print(algo.enroll(Student(103, "Charlie", 92.5)));
print(algo.enroll(Student(104, "Diana", 88.5)));

print("Enrolling students in Databases:");
db.enroll(Student(201, "Eve", 90.0));
db.enroll(Student(202, "Frank", 85.5));
db.enroll(Student(203, "Grace", 93.0));

print("Adding courses to department:");
cs.addCourse(algo);
cs.addCourse(db);

print("Department overview:");
cs.listAllCourses();

print("Total students:");
print(cs.getTotalStudents());

print("All students in Algorithms:");
algo.listAll();

print("Average grade in Algorithms:");
print(algo.calculateAverage());

print("Top 2 students in Algorithms:");
Student[] top = algo.getTopStudents(2);
Int i = 0;
while (i < 2) {
    top[i].display();
    i = i + 1;
}

print("Sorted Databases students:");
db.sortStudents();
db.listAll();

print("Access specific student:");
Student student = cs.getCourse(0).getStudent(1);
student.display();

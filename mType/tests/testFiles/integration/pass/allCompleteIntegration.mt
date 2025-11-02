// All Three Test 4: Complete integration scenario
@Script

interface Comparable<T> {
    fun compareTo(other: T): Int;
}

class Student implements Comparable<Student> {
    field id: Int;
    field name: String;
    field grade: Float;

    constructor(studentId: Int, studentName: String, studentGrade: Float) {
        this.id = studentId;
        this.name = studentName;
        this.grade = studentGrade;
    }

    fun getId(): Int {
        return this.id;
    }

    fun getName(): String {
        return this.name;
    }

    fun getGrade(): Float {
        return this.grade;
    }

    fun compareTo(other: Student): Int {
        if (this.grade < other.grade) {
            return -1;
        } else if (this.grade > other.grade) {
            return 1;
        }
        return 0;
    }

    fun display(): Void {
        print(this.id);
        print(this.name);
        print(this.grade);
    }
}

class Course<T extends Comparable<T>> {
    field name: String;
    field students: T[];
    field count: Int;
    field capacity: Int;

    constructor(courseName: String, maxStudents: Int) {
        this.name = courseName;
        this.capacity = maxStudents;
        this.students = T[maxStudents];
        this.count = 0;
    }

    fun enroll(student: T): Bool {
        if (this.count >= this.capacity) {
            return false;
        }
        this.students[this.count] = student;
        this.count = this.count + 1;
        return true;
    }

    fun getStudent(index: Int): T {
        return this.students[index];
    }

    fun getCount(): Int {
        return this.count;
    }

    fun getName(): String {
        return this.name;
    }

    fun getTopStudents(n: Int): T[] {
        this.sortStudents();
        let result: T[] = T[n];
        let i: Int = 0;
        while (i < n) {
            result[i] = this.students[this.count - 1 - i];
            i = i + 1;
        }
        return result;
    }

    fun sortStudents(): Void {
        let i: Int = 0;
        while (i < this.count - 1) {
            let j: Int = 0;
            while (j < this.count - i - 1) {
                if (this.students[j].compareTo(this.students[j + 1]) > 0) {
                    let temp: T = this.students[j];
                    this.students[j] = this.students[j + 1];
                    this.students[j + 1] = temp;
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }

    fun calculateAverage(): Float {
        let sum: Float = 0.0;
        let i: Int = 0;
        while (i < this.count) {
            sum = sum + this.students[i].getGrade();
            i = i + 1;
        }
        return sum / this.count;
    }

    fun listAll(): Void {
        let i: Int = 0;
        while (i < this.count) {
            this.students[i].display();
            i = i + 1;
        }
    }
}

class Department {
    field courses: Course<Student>[];
    field courseCount: Int;

    constructor(maxCourses: Int) {
        this.courses = Course<Student>[maxCourses];
        this.courseCount = 0;
    }

    fun addCourse(course: Course<Student>): Void {
        this.courses[this.courseCount] = course;
        this.courseCount = this.courseCount + 1;
    }

    fun getCourse(index: Int): Course<Student> {
        return this.courses[index];
    }

    fun getTotalStudents(): Int {
        let total: Int = 0;
        let i: Int = 0;
        while (i < this.courseCount) {
            total = total + this.courses[i].getCount();
            i = i + 1;
        }
        return total;
    }

    fun listAllCourses(): Void {
        let i: Int = 0;
        while (i < this.courseCount) {
            print(this.courses[i].getName());
            print(this.courses[i].getCount());
            i = i + 1;
        }
    }
}

print("Creating Computer Science Department:");
let cs: Department = Department(3);

print("Creating courses:");
let algo: Course<Student> = Course<Student>("Algorithms", 4);
let db: Course<Student> = Course<Student>("Databases", 3);

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
let top: Student[] = algo.getTopStudents(2);
let i: Int = 0;
while (i < 2) {
    top[i].display();
    i = i + 1;
}

print("Sorted Databases students:");
db.sortStudents();
db.listAll();

print("Access specific student:");
let student: Student = cs.getCourse(0).getStudent(1);
student.display();

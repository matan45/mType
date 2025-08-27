// Class with different field types
class Student {
    string name;
    int age;
    float gpa;
    bool isActive;
    
    constructor(string n, int a) {
        name = n;
        age = a;
        gpa = 0.01;
        isActive = true;
    }
    
    constructor(string n, int a, float g) {
        name = n;
        age = a;
        gpa = g;
        isActive = true;
    }
    
    function displayInfo(): string {
        return this.name + " (Age: " + toString(this.age) + ", GPA: " + toString(this.gpa) + ")";
    }
}

// Class with static fields
class University {
    static string universityName = "Tech University";
    static int totalStudents = 0;
    static final int maxCapacity = 10000;
    
    static function incrementStudents(): void {
        University::totalStudents = University::totalStudents + 1;
    }
    
    static function getInfo(): string {
        return University::universityName + " - Students: " + toString(University::totalStudents) + "/" + toString(University::maxCapacity);
    }
}
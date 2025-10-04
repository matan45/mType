class Student {
    public string name;
    public int id;
    public float gpa;

    public constructor(string n, int i, float g) {
        name = n;
        id = i;
        gpa = g;
    }

    public function display(): void {
        print(name);
        print(id);
        print(gpa);
    }
}

Student s = new Student("Alice", 12345, 3.8);
s.display(); // Should print Alice, 12345, 3.8
class Department {
        public string name;

        constructor(string n) {
            name = n;
        }

        public function getName(): string {
            return name;
        }
    }

class Company {

    public Department dept;

    constructor(string deptName) {
        dept = new Department(deptName);
    }

    public function getDepartment(): Department {
        return dept;
    }
}

Company comp = new Company("Engineering");
Department dept = comp.getDepartment();
string deptName = dept.getName();

print("Nested object returns test passed");
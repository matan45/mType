class Department {
        string name;
        
        constructor(string n) {
            name = n;
        }
        
        function getName(): string {
            return name;
        }
    }

class Company {
    
    Department dept;
    
    constructor(string deptName) {
        dept = new Department(deptName);
    }
    
    function getDepartment(): Department {
        return dept;
    }
}

Company comp = new Company("Engineering");
Department dept = comp.getDepartment();
string deptName = dept.getName();

print("Nested object returns test passed");
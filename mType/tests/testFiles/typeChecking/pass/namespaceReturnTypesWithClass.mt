namespace business {
    class Employee {
        string id;
        
        constructor(string i) {
            id = i;
        }
        
        function getId(): string {
            return id;
        }
    }
    
    function createEmployee(string id): business::Employee {
        return new business::Employee(id);
    }
}

business::Employee emp = business::createEmployee("E001");
string empId = emp.getId();

print("Namespace with class test passed");
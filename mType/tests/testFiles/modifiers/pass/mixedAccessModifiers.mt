// Test: Class with mixed access modifiers
class Employee {
    private string ssn;
    protected int employeeID;
    public string name;

    public Employee(string n, int id, string social) {
        name = n;
        employeeID = id;
        ssn = social;
    }

    private string maskSSN() {
        return "***-**-****";
    }

    protected int getID() {
        return employeeID;
    }

    public string getPublicInfo() {
        return name + " (ID: " + getID().toString() + ", SSN: " + maskSSN() + ")";
    }
}

Employee emp = new Employee("John Doe", 12345, "123-45-6789");
print(emp.name);              // Expected: John Doe (public field)
print(emp.getPublicInfo());   // Expected: John Doe (ID: 12345, SSN: ***-**-****)

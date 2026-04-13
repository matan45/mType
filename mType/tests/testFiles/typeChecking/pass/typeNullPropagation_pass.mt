// Test null propagation through expressions
// Validates that null propagates correctly through complex expressions

class Location {
    string city;
    string country;

    constructor(string c, string co) {
        city = c;
        country = co;
    }

    public function getCity(): string {
        return city;
    }

    public function getCountry(): string {
        return country;
    }

    public function getFullLocation(): string {
        return city + ", " + country;
    }
}

class Company {
    string name;
    Location? location;

    constructor(string n) {
        name = n;
        location = null;
    }

    public function setLocation(Location loc): void {
        location = loc;
    }

    public function getLocation(): Location? {
        return location;
    }

    public function getName(): string {
        return name;
    }

    public function getLocationInfo(): string {
        if (location != null) {
            return location.getFullLocation();
        } else {
            return "Location unknown";
        }
    }
}

class Employee {
    string name;
    Company? company;

    constructor(string n) {
        name = n;
        company = null;
    }

    public function setCompany(Company c): void {
        company = c;
    }

    public function getCompany(): Company? {
        return company;
    }

    public function getCompanyLocation(): string {
        if (company != null) {
            return company.getLocationInfo();
        } else {
            return "No company";
        }
    }

    public function getCompanyCity(): string {
        if (company != null) {
            Location loc = company.getLocation();
            if (loc != null) {
                return loc.getCity();
            } else {
                return "No location";
            }
        } else {
            return "No company";
        }
    }
}

function main(): void {
    print("Testing null propagation through expressions");

    // Create employee with no company
    Employee emp1 = new Employee("John");
    print("Employee 1 company location: " + emp1.getCompanyLocation());
    print("Employee 1 company city: " + emp1.getCompanyCity());

    // Create employee with company but no location
    Employee emp2 = new Employee("Jane");
    Company comp1 = new Company("TechCorp");
    emp2.setCompany(comp1);
    print("Employee 2 company location: " + emp2.getCompanyLocation());
    print("Employee 2 company city: " + emp2.getCompanyCity());

    // Create employee with full chain
    Employee emp3 = new Employee("Bob");
    Company comp2 = new Company("GlobalSoft");
    Location loc = new Location("New York", "USA");
    comp2.setLocation(loc);
    emp3.setCompany(comp2);
    print("Employee 3 company location: " + emp3.getCompanyLocation());
    print("Employee 3 company city: " + emp3.getCompanyCity());

    // Test null propagation in conditional chains
    Employee? emp4 = null;
    if (emp4 != null) {
        print("Employee 4 company: " + emp4.getCompany().getName());
    } else {
        print("Employee 4 is null");
    }

    // Test propagation with multiple checks
    if (emp3.getCompany() != null) {
        Company c = emp3.getCompany();
        if (c.getLocation() != null) {
            Location l = c.getLocation();
            print("Full details: " + c.getName() + " - " + l.getFullLocation());
        }
    }

    print("Null propagation test completed");
}

main();

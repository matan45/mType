// Test returning 'this' from methods and method chaining

// Example 1: Simple this return for self-reference
class Vehicle {
    public string brand;
    public int year;

    public constructor(string b, int y) {
        brand = b;
        year = y;
    }

    public function getVehicle(): Vehicle {
        return this;
    }

    public function setBrand(string newBrand): Vehicle {
        this.brand = newBrand;
        return this;
    }

    public function setYear(int newYear): Vehicle {
        this.year = newYear;
        return this;
    }

    public function getInfo(): string {
        return brand + " (" + year + ")";
    }
}

// Example 2: Builder pattern with method chaining
class StringBuilder {
    public string content;

    public constructor() {
        content = "";
    }

    public function append(string text): StringBuilder {
        content = content + text;
        return this;
    }

    public function appendNumber(int num): StringBuilder {
        content = content + num;
        return this;
    }

    public function clear(): StringBuilder {
        content = "";
        return this;
    }

    public function toString(): string {
        return content;
    }
}

// Example 3: Configuration object with fluent interface
class Config {
    public string host;
    public int port;
    public bool secure;

    public constructor() {
        host = "localhost";
        port = 8080;
        secure = false;
    }

    public function setHost(string h): Config {
        this.host = h;
        return this;
    }

    public function setPort(int p): Config {
        this.port = p;
        return this;
    }

    public function setSecure(bool s): Config {
        this.secure = s;
        return this;
    }

    public function getConnectionString(): string {
        string protocol = secure ? "https" : "http";
        return protocol + "://" + host + ":" + port;
    }
}

// Test 1: Basic this return
Vehicle car = new Vehicle("Toyota", 2020);
Vehicle sameCar = car.getVehicle();
print("Original: " + car.getInfo());
print("Returned this: " + sameCar.getInfo());

// Test 2: Method chaining with Vehicle
Vehicle truck = new Vehicle("Ford", 2018);
truck.setBrand("Chevrolet").setYear(2022);
print("After chaining: " + truck.getInfo());

// Test 3: StringBuilder method chaining
StringBuilder sb = new StringBuilder();
sb.append("Hello").append(" ").append("World").appendNumber(123);
print("Built string: " + sb.toString());

// Test 4: More complex chaining
StringBuilder sb2 = new StringBuilder();
string result = sb2.append("Count: ").appendNumber(42).append(" items").toString();
print("Complex chain result: " + result);

// Test 5: Clear and reuse
sb2.clear().append("New").append(" ").append("Content");
print("After clear and reuse: " + sb2.toString());

// Test 6: Config object fluent interface
Config config = new Config();
config.setHost("example.com").setPort(443).setSecure(true);
print("Config URL: " + config.getConnectionString());

// Test 7: Chain return validation - should be same object
Vehicle testVehicle = new Vehicle("Test", 2023);
Vehicle chainedResult = testVehicle.setBrand("Updated");
print("Original after chain: " + testVehicle.getInfo());
print("Chained result: " + chainedResult.getInfo());

// Test 8: Fluent interface with constructor chaining  
Config nestedConfig = new Config();
nestedConfig.setHost("api.server.com").setPort(9000).setSecure(false);
print("Nested chain URL: " + nestedConfig.getConnectionString());
// Test returning 'this' from methods and method chaining

// Example 1: Simple this return for self-reference
class Vehicle {
    string brand;
    int year;
    
    constructor(string b, int y) {
        brand = b;
        year = y;
    }
    
    function getVehicle(): Vehicle {
        return this;
    }
    
    function setBrand(string newBrand): Vehicle {
        this.brand = newBrand;
        return this;
    }
    
    function setYear(int newYear): Vehicle {
        this.year = newYear;
        return this;
    }
    
    function getInfo(): string {
        return brand + " (" + toString(year) + ")";
    }
}

// Example 2: Builder pattern with method chaining
class StringBuilder {
    string content;
    
    constructor() {
        content = "";
    }
    
    function append(string text): StringBuilder {
        content = content + text;
        return this;
    }
    
    function appendNumber(int num): StringBuilder {
        content = content + toString(num);
        return this;
    }
    
    function clear(): StringBuilder {
        content = "";
        return this;
    }
    
    function toString(): string {
        return content;
    }
}

// Example 3: Configuration object with fluent interface
class Config {
    string host;
    int port;
    bool secure;
    
    constructor() {
        host = "localhost";
        port = 8080;
        secure = false;
    }
    
    function setHost(string h): Config {
        this.host = h;
        return this;
    }
    
    function setPort(int p): Config {
        this.port = p;
        return this;
    }
    
    function setSecure(bool s): Config {
        this.secure = s;
        return this;
    }
    
    function getConnectionString(): string {
        string protocol = secure ? "https" : "http";
        return protocol + "://" + host + ":" + toString(port);
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
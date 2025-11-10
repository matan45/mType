// Integration Test 17: Builder Pattern with Method Chaining
// Tests: Classes + Method chaining (return this) + Final fields

import * from "../../../lib/primitives/String.mt";
import * from "../../../lib/primitives/Int.mt";

// Product class with many fields
class Computer {
    private string cpu;
    private string gpu;
    private int ram;
    private int storage;
    private string os;
    private bool hasWifi;

    constructor(string c, string g, int r, int s, string o, bool w) {
        this.cpu = c;
        this.gpu = g;
        this.ram = r;
        this.storage = s;
        this.os = o;
        this.hasWifi = w;
    }

    public function describe(): void {
        print("Computer Configuration:");
        print("  CPU: " + this.cpu);
        print("  GPU: " + this.gpu);
        print("  RAM: " + this.ram + "GB");
        print("  Storage: " + this.storage + "GB");
        print("  OS: " + this.os);
        print("  WiFi: " + this.hasWifi);
    }
}

// Builder class with method chaining
class ComputerBuilder {
    private string cpu;
    private string gpu;
    private int ram;
    private int storage;
    private string os;
    private bool hasWifi;

    constructor() {
        // Default values
        this.cpu = "Generic CPU";
        this.gpu = "Integrated GPU";
        this.ram = 8;
        this.storage = 256;
        this.os = "Linux";
        this.hasWifi = true;
    }

    public function setCPU(string c): ComputerBuilder {
        this.cpu = c;
        return this;  // Method chaining
    }

    public function setGPU(string g): ComputerBuilder {
        this.gpu = g;
        return this;
    }

    public function setRAM(int r): ComputerBuilder {
        this.ram = r;
        return this;
    }

    public function setStorage(int s): ComputerBuilder {
        this.storage = s;
        return this;
    }

    public function setOS(string o): ComputerBuilder {
        this.os = o;
        return this;
    }

    public function setWifi(bool w): ComputerBuilder {
        this.hasWifi = w;
        return this;
    }

    public function build(): Computer {
        print("Building computer...");
        return new Computer(this.cpu, this.gpu, this.ram, this.storage, this.os, this.hasWifi);
    }
}

// Fluent API for string building
class StringBuilder {
    private string content;

    constructor() {
        this.content = "";
    }

    public function append(string text): StringBuilder {
        this.content = this.content + text;
        return this;
    }

    public function appendLine(string text): StringBuilder {
        this.content = this.content + text + "\n";
        return this;
    }

    public function appendNumber(int num): StringBuilder {
        this.content = this.content + num;
        return this;
    }

    public function clear(): StringBuilder {
        this.content = "";
        return this;
    }

    public function toString(): string {
        return this.content;
    }
}

// Query builder pattern
class Query {
    private string table;
    private string whereClause;
    private string orderBy;
    private int limit;

    constructor() {
        this.table = "";
        this.whereClause = "";
        this.orderBy = "";
        this.limit = -1;
    }

    public function from(string t): Query {
        this.table = t;
        return this;
    }

    public function where(string condition): Query {
        this.whereClause = condition;
        return this;
    }

    public function orderBy(string column): Query {
        this.orderBy = column;
        return this;
    }

    public function limit(int l): Query {
        this.limit = l;
        return this;
    }

    public function build(): string {
        string sql = "SELECT * FROM " + this.table;

        if (this.whereClause != "") {
            sql = sql + " WHERE " + this.whereClause;
        }

        if (this.orderBy != "") {
            sql = sql + " ORDER BY " + this.orderBy;
        }

        if (this.limit > 0) {
            sql = sql + " LIMIT " + this.limit;
        }

        return sql;
    }
}

// Configuration builder with validation
class ServerConfig {
    private string host;
    private int port;
    private int timeout;

    constructor(string h, int p, int t) {
        this.host = h;
        this.port = p;
        this.timeout = t;
    }

    public function describe(): void {
        print("Server: " + this.host + ":" + this.port + " (timeout: " + this.timeout + "s)");
    }
}

class ServerConfigBuilder {
    private string host;
    private int port;
    private int timeout;

    constructor() {
        this.host = "localhost";
        this.port = 8080;
        this.timeout = 30;
    }

    public function withHost(string h): ServerConfigBuilder {
        this.host = h;
        return this;
    }

    public function withPort(int p): ServerConfigBuilder {
        this.port = p;
        return this;
    }

    public function withTimeout(int t): ServerConfigBuilder {
        this.timeout = t;
        return this;
    }

    public function build(): ServerConfig {
        print("Building server config...");
        return new ServerConfig(this.host, this.port, this.timeout);
    }
}

// Main test execution
print("=== Test 17: Builder Pattern with Method Chaining ===");

// Test 1: Computer builder with full customization
print("--- Test 1: Full computer build ---");
Computer gaming = new ComputerBuilder()
    .setCPU("Intel i9")
    .setGPU("NVIDIA RTX 4090")
    .setRAM(64)
    .setStorage(2000)
    .setOS("Windows 11")
    .setWifi(true)
    .build();

gaming.describe();

// Test 2: Computer builder with defaults
print("--- Test 2: Minimal computer build ---");
Computer office = new ComputerBuilder()
    .setCPU("Intel i5")
    .setRAM(16)
    .build();

office.describe();

// Test 3: StringBuilder chaining
print("--- Test 3: StringBuilder ---");
StringBuilder sb = new StringBuilder();
string result = sb
    .append("Hello")
    .append(" ")
    .append("World")
    .appendNumber(123)
    .toString();

print("Built string: " + result);

// Test 4: Query builder
print("--- Test 4: Query builder ---");
Query query1 = new Query();
string sql1 = query1
    .from("users")
    .where("age > 18")
    .orderBy("name")
    .limit(10)
    .build();

print("Query 1: " + sql1);

Query query2 = new Query();
string sql2 = query2
    .from("products")
    .where("price < 100")
    .limit(5)
    .build();

print("Query 2: " + sql2);

// Test 5: Server config builder
print("--- Test 5: Server config ---");
ServerConfig prod = new ServerConfigBuilder()
    .withHost("api.example.com")
    .withPort(443)
    .withTimeout(60)
    .build();

prod.describe();

ServerConfig dev = new ServerConfigBuilder()
    .withHost("localhost")
    .withPort(3000)
    .build();

dev.describe();

// Test 6: Multiple builders
print("--- Test 6: Multiple builds ---");
ComputerBuilder builder = new ComputerBuilder();

Computer comp1 = builder
    .setCPU("AMD Ryzen 9")
    .setRAM(32)
    .build();

print("Computer 1:");
comp1.describe();

// Reuse builder (note: will use accumulated values)
Computer comp2 = new ComputerBuilder()
    .setCPU("Apple M2")
    .setGPU("Integrated")
    .setRAM(16)
    .setOS("macOS")
    .build();

print("Computer 2:");
comp2.describe();

print("=== Test 17 Complete ===");

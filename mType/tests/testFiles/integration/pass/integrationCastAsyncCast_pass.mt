// Integration Test: Casting in async/await contexts
// Tests casting behavior with promises and async functions
import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/Bool.mt";

class DataSource {
    public Int id;

    public constructor(Int i) {
        this.id = i;
    }

    public function async fetchData(): Promise<String> {
        return "DataSource data";
    }

    public function getType(): String {
        return "DataSource";
    }
}

class DatabaseSource extends DataSource {
    public String dbName;

    public constructor(Int i, String name) {
        super(i);
        this.dbName = name;
    }

    public function async fetchData(): Promise<String> {
        return "Database: " + this.dbName;
    }

    public function getType(): String {
        return "DatabaseSource";
    }

    public function async connect(): Promise<Bool> {
        return new Bool(true);
    }
}

class APISource extends DataSource {
    public String endpoint;

    public constructor(Int i, String ep) {
        super(i);
        this.endpoint = ep;
    }

    public function async fetchData(): Promise<String> {
        return "API: " + this.endpoint;
    }

    public function getType(): String {
        return "APISource";
    }

    public function async authenticate(): Promise<String> {
        return "token_" + this.id;
    }
}

// Test 1: Basic async with casting
print("Test 1: Basic async with casting");

function async testBasicAsyncCast(): Promise<String> {
    DataSource source = new DatabaseSource(1, "users_db");

    if (source isClassOf DatabaseSource) {
        DatabaseSource db = (DatabaseSource)source;
        Bool connected = await db.connect();
        print("Database connected: " + connected);

        String data = await db.fetchData();
        print("Fetched: " + data);
        return data;
    }

    return "No data";
}

await testBasicAsyncCast();

// Test 2: Async loop with casting
print("Test 2: Async loop with casting");

function async testAsyncLoopCast(): Promise<void> {
    DataSource[] sources = new DataSource[3];
    sources[0] = new DatabaseSource(10, "products_db");
    sources[1] = new APISource(20, "https://api.example.com");
    sources[2] = new DatabaseSource(30, "orders_db");

    for (Int i = 0; i < 3; i = i + 1) {
        if (sources[i] isClassOf DatabaseSource) {
            DatabaseSource db = (DatabaseSource)sources[i];
            Bool conn = await db.connect();
            String result = await db.fetchData();
            print("DB Result: " + result);
        } else if (sources[i] isClassOf APISource) {
            APISource api = (APISource)sources[i];
            String token = await api.authenticate();
            print("API Token: " + token);
            String result = await api.fetchData();
            print("API Result: " + result);
        }
    }
}

await testAsyncLoopCast();

// Test 3: Nested async with casting
print("Test 3: Nested async with casting");

function async fetchAndProcess(DataSource src): Promise<String> {
    if (src isClassOf DatabaseSource) {
        DatabaseSource db = (DatabaseSource)src;
        Bool connected = await db.connect();
        if (connected) {
            return await db.fetchData();
        }
        return "Connection failed";
    }

    if (src isClassOf APISource) {
        APISource api = (APISource)src;
        String token = await api.authenticate();
        if (token.length() > 0) {
            return await api.fetchData();
        }
        return "Auth failed";
    }

    return await src.fetchData();
}

function async testNestedAsync(): Promise<void> {
    DataSource src1 = new DatabaseSource(100, "analytics_db");
    String result1 = await fetchAndProcess(src1);
    print("Nested result 1: " + result1);

    DataSource src2 = new APISource(200, "https://data.api.com");
    String result2 = await fetchAndProcess(src2);
    print("Nested result 2: " + result2);
}

await testNestedAsync();

// Test 4: Async conditional with multiple casts
print("Test 4: Async conditional with multiple casts");

function async testAsyncConditional(): Promise<Int> {
    DataSource primary = new DatabaseSource(500, "main_db");
    DataSource backup = new APISource(501, "https://backup.api.com");

    if (primary isClassOf DatabaseSource) {
        DatabaseSource db = (DatabaseSource)primary;
        Bool connected = await db.connect();

        if (connected.getValue()) {
            print("Using primary database");
            String data = await db.fetchData();
            print(data);
            return new Int(1);
        } else {
            print("Primary failed, trying backup");
            if (backup isClassOf APISource) {
                APISource api = (APISource)backup;
                String token = await api.authenticate();
                print("Backup token: " + token);
                String data = await api.fetchData();
                print(data);
                return new Int(2);
            }
        }
    }

    return new Int(0);
}

Int status = await testAsyncConditional();
print("Final status: " + status.getValue());

// Test 5: Async with casting in promise chain
print("Test 5: Async promise chain with casting");

function async processSource(DataSource src): Promise<String> {
    String type = src.getType();
    print("Processing: " + type);

    if (src isClassOf DatabaseSource) {
        DatabaseSource db = (DatabaseSource)src;
        await db.connect();
        return await db.fetchData();
    }

    if (src isClassOf APISource) {
        APISource api = (APISource)src;
        await api.authenticate();
        return await api.fetchData();
    }

    return await src.fetchData();
}

function async chainProcessing(): Promise<String> {
    DataSource[] sources = new DataSource[2];
    sources[0] = new DatabaseSource(700, "cache_db");
    sources[1] = new APISource(800, "https://external.api.com");

    String result1 = await processSource(sources[0]);
    print("Chain 1: " + result1);

    String result2 = await processSource(sources[1]);
    print("Chain 2: " + result2);

    return result1 + " | " + result2;
}

String chainResult = await chainProcessing();
print("Chain final: " + chainResult);

// Test 6: Async exception handling with casting
print("Test 6: Async with cast validation");

function async safeAsyncCast(DataSource src): Promise<String> {
    print("Safe casting source ID: " + src.id);

    if (src isClassOf DatabaseSource) {
        DatabaseSource db = (DatabaseSource)src;
        print("Cast to DatabaseSource successful");
        Bool connected = await db.connect();
        if (connected) {
            return await db.fetchData();
        }
        return "Connection error";
    } else if (src isClassOf APISource) {
        APISource api = (APISource)src;
        print("Cast to APISource successful");
        String token = await api.authenticate();
        if (token.length() > 5) {
            return await api.fetchData();
        }
        return "Auth error";
    }

    return "Unknown source type";
}

function async testSafeAsync(): Promise<void> {
    DataSource test1 = new DatabaseSource(999, "test_db");
    String res1 = await safeAsyncCast(test1);
    print("Safe result 1: " + res1);

    DataSource test2 = new APISource(1000, "https://test.api.com");
    String res2 = await safeAsyncCast(test2);
    print("Safe result 2: " + res2);
}

await testSafeAsync();

print("All async casting tests completed");

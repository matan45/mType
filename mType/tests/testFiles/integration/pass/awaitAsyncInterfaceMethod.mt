// Test: Interface with async methods
// @Script

interface AsyncService {
    async fetchData(id: Int) : Promise<String>;
    async saveData(data: String) : Promise<Bool>;
}

class DataService : AsyncService {
    private storage: String[];

    constructor() {
        this.storage = [];
    }

    async fetchData(id: Int) : Promise<String> {
        await delay(10);
        if (id >= 0 && id < this.storage.length()) {
            return this.storage[id];
        }
        return "Not found";
    }

    async saveData(data: String) : Promise<Bool> {
        await delay(10);
        this.storage.push(data);
        print("Saved: " + data);
        return true;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async testService(service: AsyncService) : Promise<String> {
    let saved = await service.saveData("Test data");
    if (saved) {
        let retrieved = await service.fetchData(0);
        return retrieved;
    }
    return "Failed";
}

async main() : Promise<Void> {
    let service: AsyncService = new DataService();

    let result1 = await service.saveData("Hello");
    assert(result1, "Should save successfully");

    let result2 = await service.fetchData(0);
    print("Fetched: " + result2);
    assert(result2 == "Hello", "Should fetch correct data");

    let result3 = await testService(service);
    assert(result3 == "Test data", "Polymorphic async should work");
}

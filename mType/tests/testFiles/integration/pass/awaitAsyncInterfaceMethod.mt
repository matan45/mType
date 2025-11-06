// Test: Interface with async methods
// @Script
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/collections/List.mt";
interface AsyncService {
    function async fetchData(Int id) : Promise<String>;
    function async saveData(String data) : Promise<Bool>;
}

class DataService implements AsyncService {
    private List<String> storage;

    constructor() {
        this.storage = new List<String>();
    }

    public function async fetchData(Int id) : Promise<String> {
        await delay(10);
        if (id >= 0 && id < this.storage.size()) {
            return this.storage.get(id);
        }
        return new String("Not found");
    }

    public function async saveData(String data) : Promise<Bool> {
        await delay(10);
        this.storage.add(new String(data));
        print("Saved: " + data);
        return new Bool(true);
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async testService(AsyncService service ) : Promise<String> {
    Bool saved = await service.saveData("Test data");
    if (saved.value) {
        String retrieved = await service.fetchData(0);
        return retrieved;
    }
    return new String("Failed");
}

function async main() : Promise<void> {
    AsyncService service  = new DataService();

    Bool result1 = await service.saveData("Hello");
    print("Should save successfully: "+result1.value);

    String result2 = await service.fetchData(0);
    print("Fetched: " + result2);

    String result3 = await testService(service);
}
main();
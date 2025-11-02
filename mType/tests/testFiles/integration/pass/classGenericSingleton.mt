// Class + Generics Test 6: Generic singleton pattern
@Script

class Singleton<T> {
    field value: T;
    field initialized: Bool;
    static field instanceCount: Int = 0;

    constructor(val: T) {
        this.value = val;
        this.initialized = true;
        Singleton.instanceCount = Singleton.instanceCount + 1;
    }

    fun getValue(): T {
        return this.value;
    }

    fun setValue(val: T): Void {
        this.value = val;
    }

    fun isInitialized(): Bool {
        return this.initialized;
    }

    static fun getInstanceCount(): Int {
        return Singleton.instanceCount;
    }
}

class Config<T> {
    field setting: T;
    field name: String;

    constructor(settingName: String, defaultValue: T) {
        this.name = settingName;
        this.setting = defaultValue;
    }

    fun getSetting(): T {
        return this.setting;
    }

    fun setSetting(value: T): Void {
        this.setting = value;
    }

    fun getName(): String {
        return this.name;
    }

    fun display(): Void {
        print(this.name);
        print(this.setting);
    }
}

print("Creating singleton instances:");
let intSingleton: Singleton<Int> = Singleton<Int>(42);
print("Int singleton value:");
print(intSingleton.getValue());
print("Is initialized:");
print(intSingleton.isInitialized());

intSingleton.setValue(100);
print("Updated value:");
print(intSingleton.getValue());

let strSingleton: Singleton<String> = Singleton<String>("default");
print("String singleton value:");
print(strSingleton.getValue());

strSingleton.setValue("updated");
print("Updated string value:");
print(strSingleton.getValue());

print("Instance count:");
print(Singleton.getInstanceCount());

print("Creating Config instances:");
let intConfig: Config<Int> = Config<Int>("MaxConnections", 10);
intConfig.display();

intConfig.setSetting(20);
print("After update:");
intConfig.display();

let boolConfig: Config<Bool> = Config<Bool>("EnableLogging", true);
boolConfig.display();

boolConfig.setSetting(false);
print("After toggle:");
boolConfig.display();

let floatConfig: Config<Float> = Config<Float>("Timeout", 30.5);
floatConfig.display();

print("Another singleton:");
let boolSingleton: Singleton<Bool> = Singleton<Bool>(false);
print(boolSingleton.getValue());

print("Final instance count:");
print(Singleton.getInstanceCount());

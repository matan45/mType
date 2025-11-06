// Class + Generics Test 6: Generic singleton pattern
@Script

class Singleton<T> {
    T value;
    Bool initialized;
    static Int instanceCount = 0;

    constructor(T val) {
        this.value = val;
        this.initialized = true;
        Singleton.instanceCount = Singleton.instanceCount + 1;
    }

    public function getValue(): T {
        return this.value;
    }

    public function setValue(T val): void {
        this.value = val;
    }

    public function isInitialized(): Bool {
        return this.initialized;
    }

    public static function getInstanceCount(): Int {
        return Singleton.instanceCount;
    }
}

class Config<T> {
    T setting;
    String name;

    constructor(String settingName, T defaultValue) {
        this.name = settingName;
        this.setting = defaultValue;
    }

    public function getSetting(): T {
        return this.setting;
    }

    public function setSetting(T value): void {
        this.setting = value;
    }

    public function getName(): String {
        return this.name;
    }

    public function display(): void {
        print(this.name);
        print(this.setting);
    }
}

print("Creating singleton instances:");
Singleton<Int> intSingleton = Singleton<Int>(42);
print("Int singleton value:");
print(intSingleton.getValue());
print("Is initialized:");
print(intSingleton.isInitialized());

intSingleton.setValue(100);
print("Updated value:");
print(intSingleton.getValue());

Singleton<String> strSingleton = Singleton<String>("default");
print("String singleton value:");
print(strSingleton.getValue());

strSingleton.setValue("updated");
print("Updated string value:");
print(strSingleton.getValue());

print("Instance count:");
print(Singleton.getInstanceCount());

print("Creating Config instances:");
Config<Int> intConfig = Config<Int>("MaxConnections", 10);
intConfig.display();

intConfig.setSetting(20);
print("After update:");
intConfig.display();

Config<Bool> boolConfig = Config<Bool>("EnableLogging", true);
boolConfig.display();

boolConfig.setSetting(false);
print("After toggle:");
boolConfig.display();

Config<Float> floatConfig = Config<Float>("Timeout", 30.5);
floatConfig.display();

print("Another singleton:");
Singleton<Bool> boolSingleton = Singleton<Bool>(false);
print(boolSingleton.getValue());

print("Final instance count:");
print(Singleton.getInstanceCount());

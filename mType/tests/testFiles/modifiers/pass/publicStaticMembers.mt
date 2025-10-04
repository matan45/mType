// Test: Public static members accessible externally
class Config {
    public static string appName = "MyApp";
    public static int version = 1;

    public static function getInfo(): string {
        return appName + " v" + version;
    }
}

// External access to public static members
print(Config::appName);           // Expected: MyApp
print(Config::version);           // Expected: 1
print(Config::getInfo());         // Expected: MyApp v1

// Modify public static fields
Config::appName = "NewApp";
Config::version = 2;
print(Config::getInfo());         // Expected: NewApp v2

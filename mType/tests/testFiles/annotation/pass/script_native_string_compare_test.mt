// Test: Native C++ string comparison in @Script callback
// Reproduces the bug where strings passed from C++ (std::string) fail
// comparison against script literals (InternedString) in == and != checks.
// The test framework calls onInteropTest(string) with C++ std::string values:
//   "NewGameBtn", "SettingsBtn", "UnknownBtn"

@Script
public class NativeStringCompareTest {

    constructor() {
    }

    public function onStart(): void {
        print("onStart called");
    }

    public function onUpdate(float deltaTime): void {
    }

    public function onDestroy(): void {
    }

    // Called by the C++ test framework with native std::string arguments
    public function onInteropTest(string entityName): void {
        print("received: " + entityName);

        // Test == comparison (native string vs script literal)
        if (entityName == "NewGameBtn") {
            print("matched NewGameBtn");
        } else if (entityName == "SettingsBtn") {
            print("matched SettingsBtn");
        } else {
            print("no match: " + entityName);
        }

        // Test != comparison
        if (entityName != "NewGameBtn") {
            print("not NewGameBtn");
        }
        if (entityName != "SettingsBtn") {
            print("not SettingsBtn");
        }
    }
}

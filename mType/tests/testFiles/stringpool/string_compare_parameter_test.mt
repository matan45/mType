// String comparison test for parameter/callback string patterns
// Tests the pattern where a string received as a function parameter
// is compared against a literal (e.g., entityName == "NewGameBtn")

function assertTest(bool condition, string testName): void {
    if (condition) {
        print("PASS: " + testName);
    } else {
        print("FAIL: " + testName);
    }
}

// Simulates a callback dispatching on a string parameter
function handleButton(string entityName): string {
    if (entityName == "NewGameBtn") {
        return "newgame";
    } else if (entityName == "SettingsBtn") {
        return "settings";
    } else if (entityName == "ExitBtn") {
        return "exit";
    } else {
        return "unknown";
    }
}

// Test 1: Basic parameter-to-literal comparison
print("--- Parameter to literal comparison ---");
assertTest(handleButton("NewGameBtn") == "newgame", "param == literal match");
assertTest(handleButton("SettingsBtn") == "settings", "param == literal second branch");
assertTest(handleButton("ExitBtn") == "exit", "param == literal third branch");
assertTest(handleButton("Other") == "unknown", "param == literal no match");

// Test 2: Variable passed as parameter
print("--- Variable passed as parameter ---");
string btnName = "NewGameBtn";
assertTest(handleButton(btnName) == "newgame", "variable passed to param");

string btnName2 = "SettingsBtn";
assertTest(handleButton(btnName2) == "settings", "variable passed to param 2");

// Test 3: Concatenated string passed as parameter
print("--- Concatenated string as parameter ---");
string prefix = "NewGame";
string suffix = "Btn";
string combined = prefix + suffix;
assertTest(handleButton(combined) == "newgame", "concatenated param match");
assertTest(combined == "NewGameBtn", "concatenated equals literal");

// Test 4: String from function return compared to literal
print("--- Function return compared to literal ---");
function getButtonName(): string {
    return "NewGameBtn";
}

string fromFunc = getButtonName();
assertTest(fromFunc == "NewGameBtn", "function return == literal");
assertTest(handleButton(getButtonName()) == "newgame", "function return as param");

// Test 5: Class field compared to literal
print("--- Class field comparison ---");
class ButtonEvent {
    public string entityName;
    public int entityId;

    constructor(string name, int id) {
        this.entityName = name;
        this.entityId = id;
    }
}

ButtonEvent evt = new ButtonEvent("NewGameBtn", 42);
assertTest(evt.entityName == "NewGameBtn", "class field == literal");
assertTest(handleButton(evt.entityName) == "newgame", "class field as param");

ButtonEvent evt2 = new ButtonEvent("SettingsBtn", 43);
assertTest(evt2.entityName == "SettingsBtn", "class field == literal 2");
assertTest(evt2.entityName != "NewGameBtn", "class field != different literal");

// Test 6: Chained if-else with parameter (exact game pattern)
print("--- Chained if-else dispatch ---");
function onButtonClicked(int buttonEntityId, string entityName): string {
    string result = "none";

    if (entityName == "NewGameBtn") {
        result = "startNewGame";
    } else if (entityName == "SettingsBtn") {
        result = "showSettings";
    } else if (entityName == "BackBtn") {
        result = "hideSettings";
    } else if (entityName == "ExitBtn") {
        result = "exit";
    } else if (entityName == "AudioTabBtn") {
        result = "audioTab";
    } else if (entityName == "GraphicsTabBtn") {
        result = "graphicsTab";
    } else if (entityName == "ControlsTabBtn") {
        result = "controlsTab";
    }

    return result;
}

assertTest(onButtonClicked(1, "NewGameBtn") == "startNewGame", "dispatch NewGameBtn");
assertTest(onButtonClicked(2, "SettingsBtn") == "showSettings", "dispatch SettingsBtn");
assertTest(onButtonClicked(3, "BackBtn") == "hideSettings", "dispatch BackBtn");
assertTest(onButtonClicked(4, "ExitBtn") == "exit", "dispatch ExitBtn");
assertTest(onButtonClicked(5, "AudioTabBtn") == "audioTab", "dispatch AudioTabBtn");
assertTest(onButtonClicked(6, "GraphicsTabBtn") == "graphicsTab", "dispatch GraphicsTabBtn");
assertTest(onButtonClicked(7, "ControlsTabBtn") == "controlsTab", "dispatch ControlsTabBtn");
assertTest(onButtonClicked(8, "UnknownBtn") == "none", "dispatch unknown falls through");

// Test 7: Multiple comparisons on same parameter
print("--- Multiple comparisons on same variable ---");
string name = "SettingsBtn";
assertTest(name != "NewGameBtn", "same var != first");
assertTest(name == "SettingsBtn", "same var == second");
assertTest(name != "ExitBtn", "same var != third");

// Test 8: Parameter compared after reassignment
print("--- Reassigned variable comparison ---");
string current = "NewGameBtn";
assertTest(current == "NewGameBtn", "before reassign");
current = "SettingsBtn";
assertTest(current == "SettingsBtn", "after reassign");
assertTest(current != "NewGameBtn", "old value no longer matches");

// Test 9: Similar prefixes
print("--- Similar prefix strings ---");
assertTest("NewGame" != "NewGameBtn", "shorter != longer");
assertTest("NewGameBtn" != "NewGameBtnX", "prefix != extended");
assertTest("NewGameBtn" != "NewGamebt", "case difference");
assertTest("newgamebtn" != "NewGameBtn", "lowercase != mixed case");

print("--- All string comparison tests completed ---");

// Test: Deep class hierarchies with all features integration
// @Script

interface Identifiable {
    function getId() : String;
}

interface Describable {
    function getDescription() : String;
}

interface Processable {
    function async process() : Promise<Bool>;
}

class Entity<T> implements Identifiable {
    protected String id;
    protected T data;

    constructor(String i, T d) {
        this.id = i;
        this.data = d;
    }

    function getId() : String {
        return this.id;
    }

    function getData() : T {
        return this.data;
    }

    function setData(T d) : void {
        this.data = d;
    }
}

class DocumentEntity extends Entity<String> implements Describable {
    private String title;
    private Int version;

    constructor(String i, String d, String t, Int v) {
        super(i, d);
        this.title = t;
        this.version = v;
    }

    function getDescription() : String {
        return this.title + " v" + this.version.toString();
    }

    function getTitle() : String {
        return this.title;
    }

    function incrementVersion() : void {
        this.version = this.version + 1;
    }

    function getVersion() : Int {
        return this.version;
    }
}

class ProcessableDocument extends DocumentEntity implements Processable {
    private Bool isProcessed;
    private String errorMessage;

    constructor(String i, String d, String t, Int v) {
        super(i, d, t, v);
        this.isProcessed = false;
        this.errorMessage = "";
    }

    function async process() : Promise<Bool> {
        await delay(5);

        try {
            // Validate document
            if (this.getData().length() == 0) {
                throw "Document data is empty";
            }

            if (this.getTitle().length() == 0) {
                throw "Document title is empty";
            }

            print("Processing: " + this.getDescription());
            this.isProcessed = true;
            return true;

        } catch (String e) {
            this.errorMessage = e;
            print("Processing failed: " + e);
            return false;
        }
    }

    function getIsProcessed() : Bool {
        return this.isProcessed;
    }

    function getErrorMessage() : String {
        return this.errorMessage;
    }
}

class VersionedDocument extends ProcessableDocument {
    private String[] history;
    private Int historySize;

    constructor(String i, String d, String t, Int v, Int maxHistory) {
        super(i, d, t, v);
        this.history = new String[maxHistory];
        this.historySize = 0;
    }

    function async process() : Promise<Bool> {
        Bool result = await super.process();

        if (result) {
            this.addToHistory("Processed at version " + this.getVersion().toString());
        }

        return result;
    }

    function addToHistory(String entry) : void {
        if (this.historySize >= this.history.length()) {
            throw "History is full";
        }
        this.history[this.historySize] = entry;
        this.historySize = this.historySize + 1;
    }

    function getHistory() : String[] {
        String[] result = new String[this.historySize];
        Int i = 0;
        while (i < this.historySize) {
            result[i] = this.history[i];
            i = i + 1;
        }
        return result;
    }

    function getHistorySize() : Int {
        return this.historySize;
    }
}

class SecureDocument extends VersionedDocument {
    private Int accessLevel;
    private Bool isEncrypted;

    constructor(String i, String d, String t, Int v, Int maxHistory, Int level) {
        super(i, d, t, v, maxHistory);
        this.accessLevel = level;
        this.isEncrypted = false;
    }

    function async process() : Promise<Bool> {
        try {
            if (this.accessLevel < 3) {
                throw "Insufficient access level";
            }

            Bool result = await super.process();

            if (result && !this.isEncrypted) {
                this.encrypt();
            }

            return result;

        } catch (String e) {
            print("Secure processing failed: " + e);
            return false;
        }
    }

    function encrypt() : void {
        print("Encrypting document: " + this.getId());
        this.isEncrypted = true;
        this.addToHistory("Document encrypted");
    }

    function decrypt() : void {
        if (this.accessLevel < 5) {
            throw "Insufficient access level for decryption";
        }
        print("Decrypting document: " + this.getId());
        this.isEncrypted = false;
        this.addToHistory("Document decrypted");
    }

    function getAccessLevel() : Int {
        return this.accessLevel;
    }

    function getIsEncrypted() : Bool {
        return this.isEncrypted;
    }
}

function async delay(Int ms) : Promise<void> {
    // Simulated delay
}

function async processHierarchy<T extends Entity<String>>(T entity, Int depth) : Promise<void> {
    String indent = "";
    Int i = 0;
    while (i < depth) {
        indent = indent + "  ";
        i = i + 1;
    }

    print(indent + "Entity ID: " + entity.getId());

    // Try casting to different levels
    Describable? describable = entity as Describable;
    if (describable != null) {
        print(indent + "Description: " + describable.getDescription());
    }

    Processable? processable = entity as Processable;
    if (processable != null) {
        print(indent + "Attempting to process...");
        Bool result = await processable.process();
        print(indent + "Process result: " + result.toString());
    }

    VersionedDocument? versioned = entity as VersionedDocument;
    if (versioned != null) {
        print(indent + "History entries: " + versioned.getHistorySize().toString());
    }

    SecureDocument? secure = entity as SecureDocument;
    if (secure != null) {
        print(indent + "Access level: " + secure.getAccessLevel().toString());
        print(indent + "Encrypted: " + secure.getIsEncrypted().toString());
    }
}

function testCrossLevelCasting(Entity<String> entity) : void {
    print("Testing cross-level casting for: " + entity.getId());

    try {
        // Try to cast to various types in the hierarchy
        DocumentEntity? doc = entity as DocumentEntity;
        if (doc != null) {
            print("  Cast to DocumentEntity successful");
            print("  Title: " + doc.getTitle());
        }

        ProcessableDocument? procDoc = entity as ProcessableDocument;
        if (procDoc != null) {
            print("  Cast to ProcessableDocument successful");
        }

        VersionedDocument? verDoc = entity as VersionedDocument;
        if (verDoc != null) {
            print("  Cast to VersionedDocument successful");
            print("  History size: " + verDoc.getHistorySize().toString());
        }

        SecureDocument? secDoc = entity as SecureDocument;
        if (secDoc != null) {
            print("  Cast to SecureDocument successful");

            // Try operations that might fail
            try {
                secDoc.decrypt();
            } catch (String e) {
                print("  Decryption error: " + e);
            }
        }

        // Cast to interfaces
        Identifiable identifiable = entity as Identifiable;
        print("  Interface cast successful: " + identifiable.getId());

    } catch (String e) {
        print("  Casting error: " + e);
    }
}

function async main() : Promise<void> {
    print("=== Complex Hierarchy Test ===");

    // Test 1: Create hierarchy at different levels
    print("\n--- Test 1: Hierarchy Levels ---");

    Entity<String>[] entities = new Entity<String>[4];

    entities[0] = new Entity<String>("E1", "Basic entity data");
    entities[1] = new DocumentEntity("D1", "Doc data", "Document 1", 1);
    entities[2] = new ProcessableDocument("P1", "Processable data", "Process Doc", 2);
    entities[3] = new SecureDocument("S1", "Secure data", "Secure Doc", 3, 10, 5);

    Int i = 0;
    while (i < entities.length()) {
        await processHierarchy<Entity<String>>(entities[i], 0);
        print("");
        i = i + 1;
    }

    // Test 2: Versioned document with history
    print("--- Test 2: Version History ---");
    VersionedDocument versionedDoc = new VersionedDocument(
        "V1", "Version data", "Versioned Doc", 1, 5
    );

    versionedDoc.addToHistory("Created");
    versionedDoc.incrementVersion();
    versionedDoc.addToHistory("Updated to v2");

    Bool processed = await versionedDoc.process();
    print("Processed: " + processed.toString());

    String[] history = versionedDoc.getHistory();
    print("History:");
    Int idx = 0;
    while (idx < history.length()) {
        print("  " + history[idx]);
        idx = idx + 1;
    }

    // Test 3: Secure document access control
    print("\n--- Test 3: Secure Document ---");
    SecureDocument secureDoc = new SecureDocument(
        "SEC1", "Top secret data", "Classified", 1, 10, 3
    );

    Bool secProcessed = await secureDoc.process();
    print("Secure processed: " + secProcessed.toString());

    // Try with insufficient access
    SecureDocument lowAccessDoc = new SecureDocument(
        "SEC2", "Data", "Low Access Doc", 1, 10, 2
    );

    Bool lowProcessed = await lowAccessDoc.process();
    print("Low access processed: " + lowProcessed.toString());

    // Test 4: Cross-level casting
    print("\n--- Test 4: Cross-Level Casting ---");
    testCrossLevelCasting(entities[3]);

    // Test 5: Error handling at different levels
    print("\n--- Test 5: Error Handling ---");
    ProcessableDocument emptyDoc = new ProcessableDocument("E2", "", "Empty", 1);
    Bool emptyProcessed = await emptyDoc.process();
    print("Empty doc processed: " + emptyProcessed.toString());
    print("Error message: " + emptyDoc.getErrorMessage());

    // Test history overflow
    print("\n--- Test 6: History Overflow ---");
    VersionedDocument smallHistoryDoc = new VersionedDocument(
        "H1", "Data", "Small History", 1, 2
    );

    try {
        smallHistoryDoc.addToHistory("Entry 1");
        smallHistoryDoc.addToHistory("Entry 2");
        print("Added 2 entries successfully");

        smallHistoryDoc.addToHistory("Entry 3");
        print("Should not reach here");
    } catch (String e) {
        print("Caught expected error: " + e);
    }

    print("\n=== All tests completed ===");
}

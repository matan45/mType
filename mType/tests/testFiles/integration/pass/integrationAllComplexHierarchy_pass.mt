// Test: Deep class hierarchies with all features integration
// @Script

interface Identifiable {
    getId() : String;
}

interface Describable {
    getDescription() : String;
}

interface Processable {
    async process() : Promise<Bool>;
}

class Entity<T> : Identifiable {
    protected id: String;
    protected data: T;

    constructor(i: String, d: T) {
        this.id = i;
        this.data = d;
    }

    getId() : String {
        return this.id;
    }

    getData() : T {
        return this.data;
    }

    setData(d: T) : Void {
        this.data = d;
    }
}

class DocumentEntity extends Entity<String> : Describable {
    private title: String;
    private version: Int;

    constructor(i: String, d: String, t: String, v: Int) {
        super(i, d);
        this.title = t;
        this.version = v;
    }

    getDescription() : String {
        return this.title + " v" + this.version.toString();
    }

    getTitle() : String {
        return this.title;
    }

    incrementVersion() : Void {
        this.version = this.version + 1;
    }

    getVersion() : Int {
        return this.version;
    }
}

class ProcessableDocument extends DocumentEntity : Processable {
    private isProcessed: Bool;
    private errorMessage: String;

    constructor(i: String, d: String, t: String, v: Int) {
        super(i, d, t, v);
        this.isProcessed = false;
        this.errorMessage = "";
    }

    async process() : Promise<Bool> {
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

        } catch (e: String) {
            this.errorMessage = e;
            print("Processing failed: " + e);
            return false;
        }
    }

    getIsProcessed() : Bool {
        return this.isProcessed;
    }

    getErrorMessage() : String {
        return this.errorMessage;
    }
}

class VersionedDocument extends ProcessableDocument {
    private history: String[];
    private historySize: Int;

    constructor(i: String, d: String, t: String, v: Int, maxHistory: Int) {
        super(i, d, t, v);
        this.history = new String[maxHistory];
        this.historySize = 0;
    }

    async process() : Promise<Bool> {
        let result: Bool = await super.process();

        if (result) {
            this.addToHistory("Processed at version " + this.getVersion().toString());
        }

        return result;
    }

    addToHistory(entry: String) : Void {
        if (this.historySize >= this.history.length()) {
            throw "History is full";
        }
        this.history[this.historySize] = entry;
        this.historySize = this.historySize + 1;
    }

    getHistory() : String[] {
        let result: String[] = new String[this.historySize];
        let i: Int = 0;
        while (i < this.historySize) {
            result[i] = this.history[i];
            i = i + 1;
        }
        return result;
    }

    getHistorySize() : Int {
        return this.historySize;
    }
}

class SecureDocument extends VersionedDocument {
    private accessLevel: Int;
    private isEncrypted: Bool;

    constructor(i: String, d: String, t: String, v: Int, maxHistory: Int, level: Int) {
        super(i, d, t, v, maxHistory);
        this.accessLevel = level;
        this.isEncrypted = false;
    }

    async process() : Promise<Bool> {
        try {
            if (this.accessLevel < 3) {
                throw "Insufficient access level";
            }

            let result: Bool = await super.process();

            if (result && !this.isEncrypted) {
                this.encrypt();
            }

            return result;

        } catch (e: String) {
            print("Secure processing failed: " + e);
            return false;
        }
    }

    encrypt() : Void {
        print("Encrypting document: " + this.getId());
        this.isEncrypted = true;
        this.addToHistory("Document encrypted");
    }

    decrypt() : Void {
        if (this.accessLevel < 5) {
            throw "Insufficient access level for decryption";
        }
        print("Decrypting document: " + this.getId());
        this.isEncrypted = false;
        this.addToHistory("Document decrypted");
    }

    getAccessLevel() : Int {
        return this.accessLevel;
    }

    getIsEncrypted() : Bool {
        return this.isEncrypted;
    }
}

async delay(ms: Int) : Promise<Void> {
    // Simulated delay
}

async processHierarchy<T extends Entity<String>>(entity: T, depth: Int) : Promise<Void> {
    let indent: String = "";
    let i: Int = 0;
    while (i < depth) {
        indent = indent + "  ";
        i = i + 1;
    }

    print(indent + "Entity ID: " + entity.getId());

    // Try casting to different levels
    let describable: Describable? = entity as Describable;
    if (describable != null) {
        print(indent + "Description: " + describable.getDescription());
    }

    let processable: Processable? = entity as Processable;
    if (processable != null) {
        print(indent + "Attempting to process...");
        let result: Bool = await processable.process();
        print(indent + "Process result: " + result.toString());
    }

    let versioned: VersionedDocument? = entity as VersionedDocument;
    if (versioned != null) {
        print(indent + "History entries: " + versioned.getHistorySize().toString());
    }

    let secure: SecureDocument? = entity as SecureDocument;
    if (secure != null) {
        print(indent + "Access level: " + secure.getAccessLevel().toString());
        print(indent + "Encrypted: " + secure.getIsEncrypted().toString());
    }
}

testCrossLevelCasting(entity: Entity<String>) : Void {
    print("Testing cross-level casting for: " + entity.getId());

    try {
        // Try to cast to various types in the hierarchy
        let doc: DocumentEntity? = entity as DocumentEntity;
        if (doc != null) {
            print("  Cast to DocumentEntity successful");
            print("  Title: " + doc.getTitle());
        }

        let procDoc: ProcessableDocument? = entity as ProcessableDocument;
        if (procDoc != null) {
            print("  Cast to ProcessableDocument successful");
        }

        let verDoc: VersionedDocument? = entity as VersionedDocument;
        if (verDoc != null) {
            print("  Cast to VersionedDocument successful");
            print("  History size: " + verDoc.getHistorySize().toString());
        }

        let secDoc: SecureDocument? = entity as SecureDocument;
        if (secDoc != null) {
            print("  Cast to SecureDocument successful");

            // Try operations that might fail
            try {
                secDoc.decrypt();
            } catch (e: String) {
                print("  Decryption error: " + e);
            }
        }

        // Cast to interfaces
        let identifiable: Identifiable = entity as Identifiable;
        print("  Interface cast successful: " + identifiable.getId());

    } catch (e: String) {
        print("  Casting error: " + e);
    }
}

async main() : Promise<Void> {
    print("=== Complex Hierarchy Test ===");

    // Test 1: Create hierarchy at different levels
    print("\n--- Test 1: Hierarchy Levels ---");

    let entities: Entity<String>[] = new Entity<String>[4];

    entities[0] = new Entity<String>("E1", "Basic entity data");
    entities[1] = new DocumentEntity("D1", "Doc data", "Document 1", 1);
    entities[2] = new ProcessableDocument("P1", "Processable data", "Process Doc", 2);
    entities[3] = new SecureDocument("S1", "Secure data", "Secure Doc", 3, 10, 5);

    let i: Int = 0;
    while (i < entities.length()) {
        await processHierarchy<Entity<String>>(entities[i], 0);
        print("");
        i = i + 1;
    }

    // Test 2: Versioned document with history
    print("--- Test 2: Version History ---");
    let versionedDoc: VersionedDocument = new VersionedDocument(
        "V1", "Version data", "Versioned Doc", 1, 5
    );

    versionedDoc.addToHistory("Created");
    versionedDoc.incrementVersion();
    versionedDoc.addToHistory("Updated to v2");

    let processed: Bool = await versionedDoc.process();
    print("Processed: " + processed.toString());

    let history: String[] = versionedDoc.getHistory();
    print("History:");
    let idx: Int = 0;
    while (idx < history.length()) {
        print("  " + history[idx]);
        idx = idx + 1;
    }

    // Test 3: Secure document access control
    print("\n--- Test 3: Secure Document ---");
    let secureDoc: SecureDocument = new SecureDocument(
        "SEC1", "Top secret data", "Classified", 1, 10, 3
    );

    let secProcessed: Bool = await secureDoc.process();
    print("Secure processed: " + secProcessed.toString());

    // Try with insufficient access
    let lowAccessDoc: SecureDocument = new SecureDocument(
        "SEC2", "Data", "Low Access Doc", 1, 10, 2
    );

    let lowProcessed: Bool = await lowAccessDoc.process();
    print("Low access processed: " + lowProcessed.toString());

    // Test 4: Cross-level casting
    print("\n--- Test 4: Cross-Level Casting ---");
    testCrossLevelCasting(entities[3]);

    // Test 5: Error handling at different levels
    print("\n--- Test 5: Error Handling ---");
    let emptyDoc: ProcessableDocument = new ProcessableDocument("E2", "", "Empty", 1);
    let emptyProcessed: Bool = await emptyDoc.process();
    print("Empty doc processed: " + emptyProcessed.toString());
    print("Error message: " + emptyDoc.getErrorMessage());

    // Test history overflow
    print("\n--- Test 6: History Overflow ---");
    let smallHistoryDoc: VersionedDocument = new VersionedDocument(
        "H1", "Data", "Small History", 1, 2
    );

    try {
        smallHistoryDoc.addToHistory("Entry 1");
        smallHistoryDoc.addToHistory("Entry 2");
        print("Added 2 entries successfully");

        smallHistoryDoc.addToHistory("Entry 3");
        print("Should not reach here");
    } catch (e: String) {
        print("Caught expected error: " + e);
    }

    print("\n=== All tests completed ===");
}

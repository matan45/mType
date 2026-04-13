import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test: Type constraint propagation through nested generics
// Constraints should propagate correctly through multiple levels

interface Identifiable {
    public function getId(): int;
}

interface Named {
    public function getName(): string;
}

class Entity implements Identifiable, Named {
    protected int id;
    protected string name;

    constructor(int entityId, string entityName) {
        id = entityId;
        name = entityName;
    }

    public function getId(): int {
        return id;
    }

    public function getName(): string {
        return name;
    }

    public function describe(): string {
        return "Entity{id=" + id + ", name=" + name + "}";
    }
}

class Document extends Entity {
    string content;

    constructor(int docId, string docName, string docContent): super(docId, docName) {
        content = docContent;
    }

    public function getContent(): string {
        return content;
    }

    public function describe(): string {
        return "Document{id=" + id + ", name=" + name + ", content=" + content + "}";
    }
}

// First level: Container with bound
class Container<T extends Identifiable> {
    T item;

    constructor(T i) {
        item = i;
    }

    public function getItem(): T {
        return item;
    }

    public function getItemId(): int {
        return item.getId();
    }
}

// Second level: Wrapper that propagates the bound
class Wrapper<T extends Identifiable> {
    Container<T> container;

    constructor(Container<T> c) {
        container = c;
    }

    public function getContainer(): Container<T> {
        return container;
    }

    public function unwrap(): T {
        return container.getItem();
    }

    public function getId(): int {
        return container.getItemId();
    }
}

// Third level: Repository that further propagates bounds with additional constraint
class Repository<T extends Identifiable, T extends Named> {
    Wrapper<T>[] wrappers;
    int count;

    constructor(int capacity) {
        wrappers = new Wrapper<T>[capacity];
        count = 0;
    }

    public function add(T item): void {
        Container<T> container = new Container<T>(item);
        Wrapper<T> wrapper = new Wrapper<T>(container);
        wrappers[count] = wrapper;
        count = count + 1;
    }

    public function get(int index): T? {
        if (index >= 0 && index < count) {
            return wrappers[index].unwrap();
        }
        return null;
    }

    public function findById(int searchId): T? {
        int i = 0;
        while (i < count) {
            T item = wrappers[i].unwrap();
            if (item.getId() == searchId) {
                return item;
            }
            i = i + 1;
        }
        return null;
    }

    public function findByName(string searchName): T? {
        int i = 0;
        while (i < count) {
            T item = wrappers[i].unwrap();
            if (item.getName() == searchName) {
                return item;
            }
            i = i + 1;
        }
        return null;
    }

    public function getCount(): int {
        return count;
    }
}

// Generic function with propagated bounds
function <T extends Identifiable, T extends Named> processEntity(T entity): string {
    return "Processing [ID=" + entity.getId() + ", Name=" + entity.getName() + "]";
}

function main(): void {
    print("Testing type constraint propagation through nested generics");

    Entity entity1 = new Entity(1, "First Entity");
    Entity entity2 = new Entity(2, "Second Entity");
    Document doc1 = new Document(10, "Document A", "Content A");
    Document doc2 = new Document(11, "Document B", "Content B");

    // Test basic container with propagated bounds
    Container<Entity> entityContainer = new Container<Entity>(entity1);
    print("Entity container ID: " + entityContainer.getItemId());

    Wrapper<Entity> entityWrapper = new Wrapper<Entity>(entityContainer);
    print("Entity from wrapper: " + entityWrapper.unwrap().describe());

    // Test repository with multiple bound propagation
    Repository<Entity,Entity> entityRepo = new Repository<Entity,Entity>(10);
    entityRepo.add(entity1);
    entityRepo.add(entity2);

    print("Entity repository count: " + entityRepo.getCount());

    Entity? found1 = entityRepo.findById(1);
    if (found1 != null) {
        print("Found entity by ID 1: " + found1.describe());
    }

    Entity? found2 = entityRepo.findByName("Second Entity");
    if (found2 != null) {
        print("Found entity by name: " + found2.describe());
    }

    // Test with derived type (Document extends Entity)
    Repository<Document,Document> docRepo = new Repository<Document,Document>(10);
    docRepo.add(doc1);
    docRepo.add(doc2);

    print("Document repository count: " + docRepo.getCount());

    Document? foundDoc = docRepo.findById(10);
    if (foundDoc != null) {
        print("Found document by ID 10: " + foundDoc.describe());
    }

    Document? foundDocByName = docRepo.findByName("Document B");
    if (foundDocByName != null) {
        print("Found document by name: " + foundDocByName.describe());
    }

    // Test generic function with propagated bounds
    print(processEntity<Entity,Entity>(entity1));
    print(processEntity<Entity,Entity>(entity2));
    print(processEntity<Document,Document>(doc1));
    print(processEntity<Document,Document>(doc2));

    print("Type constraint propagation test completed successfully");
}

main();

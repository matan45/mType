import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Wildcard lower bound simulation (? super T)
class Entity {
    String id;

    public function Entity(String i) {
        id = i;
    }

    public function getId(): String {
        return id;
    }
}

class User extends Entity {
    public function User(String i) {
        super(i);
    }
}

class Sink<T> {
    T[] items;
    Int count;

    public function Sink() {
        items = new T[10];
        count = new Int(0);
    }

    public function add(T item): void {
        items[count.value] = item;
        count = new Int(count.value + 1);
    }

    public function size(): Int {
        return count;
    }
}

function addUser(Sink<Entity> sink, User user): void {
    // Can add User to Entity sink (contravariant)
    sink.add(user);
}

function main(): void {
    Sink<Entity> entitySink = new Sink<Entity>();
    addUser(entitySink, new User(new String("U1")));
    addUser(entitySink, new User(new String("U2")));

    print("Added: " + entitySink.size());
}

main();

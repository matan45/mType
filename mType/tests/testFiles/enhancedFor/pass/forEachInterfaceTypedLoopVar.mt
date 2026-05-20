// Edge: collection holds concrete impls; the loop variable is typed as the
// shared interface. Body dispatches through the interface method.
import * from "../../lib/collections/ArrayList.mt";

interface Greeter {
    function greet(): string;
}

class English implements Greeter {
    public function greet(): string {
        return "hello";
    }
}

class Italian implements Greeter {
    public function greet(): string {
        return "ciao";
    }
}

function main(): void {
    ArrayList<Greeter> greeters = new ArrayList<Greeter>();
    greeters.add(new English());
    greeters.add(new Italian());

    for (Greeter g : greeters) {
        print(g.greet());
    }
}

main();

// Expected output:
// hello
// ciao

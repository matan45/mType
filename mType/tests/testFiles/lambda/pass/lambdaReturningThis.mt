// Lambda capturing 'this' that returns the receiver after invoking a method on it
interface Builder {
    function build(): Container;
}

class Container {
    public int total;

    constructor() {
        this.total = 0;
    }

    public function add(int v): void {
        this.total = this.total + v;
    }

    public function makeBuilder(int amount): Builder {
        // Lambda captures 'this' and uses it inside the body
        return () -> {
            this.add(amount);
            return this;
        };
    }
}

print("=== Lambda Returning This Test ===");

Container c = new Container();
Builder b = c.makeBuilder(7);

Container same1 = b.build();
Container same2 = b.build();

print("total: " + c.total);
print("same1 total: " + same1.total);
print("same2 total: " + same2.total);

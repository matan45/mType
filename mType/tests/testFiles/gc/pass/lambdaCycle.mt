// Test: Lambda capture creating a circular reference
// Purpose: Verify that GC handles object -> lambda -> captured object cycles

interface Action {
    function execute(): void;
}

class Handler {
    private string name;
    private Action? callback;

    constructor(string n) {
        this.name = n;
        this.callback = null;
    }

    public function setCallback(Action? cb): void {
        this.callback = cb;
    }

    public function getName(): string {
        return this.name;
    }

    public function run(): void {
        if (this.callback != null) {
            this.callback.execute();
        }
    }
}

function main(): void {
    Handler handler = new Handler("TestHandler");

    // Create a cycle: handler -> callback lambda -> captured handler
    handler.setCallback(() -> {
        print("Callback executed for: " + handler.getName());
    });

    handler.run();
    print("Lambda cycle created successfully");
}
main();

// ERROR: Diamond problem where I1 and I2 have conflicting method signatures

interface I1 {
    function process(int x): string;
}

interface I2 {
    function process(string x): string;  // Same name, different signature
}

// ERROR: Cannot implement both interfaces with conflicting methods
class Conflict implements I1, I2 {
    public function process(int x): string {
        return "I1 implementation";
    }

    public function process(string x): string {
        return "I2 implementation";
    }
}

function main(): void {
    Conflict obj = new Conflict();
    print("This should not execute");
}

main();

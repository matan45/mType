// ERROR: Implementing interface method as protected

interface MyInterface {
    function process(): string;
}

class Implementation implements MyInterface {
    // ERROR: Interface methods must be public, not protected
    protected function process(): string {
        return "Protected implementation";
    }
}

function main(): void {
    Implementation impl = new Implementation();
    print("This should not execute");
}

main();

// Test diamond problem with multiple interfaces
// I1 and I2 both extend I3; Class implements I1 and I2

interface Base {
    function process(): string;
}

interface Left extends Base {
    function leftMethod(): string;
}

interface Right extends Base {
    function rightMethod(): string;
}

class Diamond implements Left, Right {
    public function process(): string {
        return "Diamond process";
    }

    public function leftMethod(): string {
        return "Left method";
    }

    public function rightMethod(): string {
        return "Right method";
    }
}

function main(): void {
    print("Testing diamond implementation");

    Diamond diamond = new Diamond();

    print("diamond.process(): " + diamond.process());
    print("diamond.leftMethod(): " + diamond.leftMethod());
    print("diamond.rightMethod(): " + diamond.rightMethod());

    // Cast to Base through Left
    Left leftRef = diamond;
    print("leftRef.leftMethod(): " + leftRef.leftMethod());

    // Cast to Base through Right
    Right rightRef = diamond;
    print("rightRef.rightMethod(): " + rightRef.rightMethod());

    // Cast to Base directly
    Base baseRef = diamond;
    print("baseRef.process(): " + baseRef.process());

    print("Diamond implementation test completed");
}

main();

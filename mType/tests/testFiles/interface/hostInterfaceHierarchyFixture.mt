// Fixture for the host-API interface hierarchy test (interpreter callback):
// IGreeter is implemented by GreeterBase; GreeterChild inherits the
// implementation through extends. IFarewell is declared but implemented by
// nobody, guarding against false positives. Declarations only — no output.

interface IGreeter {
    function greet(): void;
}

interface IFarewell {
    function farewell(): void;
}

public class GreeterBase implements IGreeter {
    public constructor() { }

    @Override
    public function greet(): void {
        print("hello");
    }
}

public class GreeterChild extends GreeterBase {
    public constructor() : super() { }
}

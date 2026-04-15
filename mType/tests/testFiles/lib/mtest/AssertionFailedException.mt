// AssertionFailedException — dedicated subtype thrown by Assertions so the
// TestRunner can distinguish "assertion fired" from "code under test threw".
import * from "../exceptions/Exception.mt";

public class AssertionFailedException extends Exception {
    public constructor(string msg) : super(msg) { }

    public function toString(): string {
        return "AssertionFailedException: " + this.message;
    }
}

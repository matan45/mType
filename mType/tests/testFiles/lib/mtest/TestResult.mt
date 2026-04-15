// TestResult — outcome for a single @Test method. Status is an int code:
//   0 = pending, 1 = passed, 2 = failed, 3 = skipped.
public class TestResult {
    private string _className;
    private string _methodName;
    private int _status;
    private string _failureMessage;
    private string _skipReason;
    private string _expectedException;
    private string _actualException;
    private int _declaredTimeoutMs;

    public constructor(string className, string methodName) {
        this._className = className;
        this._methodName = methodName;
        this._status = 0;
        this._failureMessage = "";
        this._skipReason = "";
        this._expectedException = "";
        this._actualException = "";
        this._declaredTimeoutMs = 0;
    }

    public function getClassName(): string { return this._className; }
    public function getMethodName(): string { return this._methodName; }
    public function isPending(): bool { return this._status == 0; }
    public function isPassed(): bool { return this._status == 1; }
    public function isFailed(): bool { return this._status == 2; }
    public function isSkipped(): bool { return this._status == 3; }
    public function getFailureMessage(): string { return this._failureMessage; }
    public function getSkipReason(): string { return this._skipReason; }
    public function getExpectedException(): string { return this._expectedException; }
    public function getActualException(): string { return this._actualException; }
    public function getDeclaredTimeoutMs(): int { return this._declaredTimeoutMs; }

    public function setExpectedException(string name): void {
        this._expectedException = name;
    }

    public function setDeclaredTimeoutMs(int ms): void {
        this._declaredTimeoutMs = ms;
    }

    public function markPassed(): void {
        this._status = 1;
    }

    public function markFailed(string msg): void {
        this._status = 2;
        this._failureMessage = msg;
    }

    public function markFailedWithActual(string msg, string actualExcName): void {
        this._status = 2;
        this._failureMessage = msg;
        this._actualException = actualExcName;
    }

    public function markSkipped(string reason): void {
        this._status = 3;
        this._skipReason = reason;
    }

    public function formatLine(): string {
        string head = "[" + this._className + "] " + this._methodName;
        if (this._status == 1) {
            string suffix = "";
            if (this._expectedException != "") {
                suffix = "  (expected=" + this._expectedException + ")";
            }
            if (this._declaredTimeoutMs > 0) {
                suffix = suffix + "  (timeout=" + this._declaredTimeoutMs + "ms unenforced)";
            }
            return head + " PASSED" + suffix;
        }
        if (this._status == 2) {
            return head + " FAILED -- " + this._failureMessage;
        }
        if (this._status == 3) {
            if (this._skipReason == "") {
                return head + " SKIPPED";
            }
            return head + " SKIPPED (" + this._skipReason + ")";
        }
        return head + " PENDING";
    }
}

// Test: three user-defined Exception subclasses dispatched via
// multi-catch; each branch handles its own discriminator and the
// trailing Exception catch is unused (intentional dead branch).
import * from "../../lib/exceptions/Exception.mt";

class NotFoundException extends Exception {
    public constructor(string m) : super(m) {}
}
class ConflictException extends Exception {
    public constructor(string m) : super(m) {}
}
class AuthException extends Exception {
    public constructor(string m) : super(m) {}
}

function fetch(int code): void {
    if (code == 404) {
        throw new NotFoundException("missing-" + code);
    }
    if (code == 409) {
        throw new ConflictException("conflict-" + code);
    }
    if (code == 401) {
        throw new AuthException("auth-" + code);
    }
    print("ok-" + code);
}

function describe(int code): void {
    try {
        fetch(code);
    } catch (NotFoundException nf) {
        print("404: " + nf.getMessage());
    } catch (ConflictException cf) {
        print("409: " + cf.getMessage());
    } catch (AuthException ae) {
        print("401: " + ae.getMessage());
    } catch (Exception e) {
        print("misc: " + e.getMessage());
    }
}

function main(): void {
    describe(200);
    describe(404);
    describe(409);
    describe(401);
}
main();

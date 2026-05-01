// Test: a private field representing "is connected" is set true on
// connect(), false on close(); a transfer that throws still triggers
// finally which calls close(). Verify via a public getter that the
// private field is reset after both success and failure.
import * from "../../lib/exceptions/Exception.mt";

class Connection {
    private bool open;

    public constructor() { open = false; }

    public function connect(): void { open = true; }

    public function isOpen(): bool { return open; }

    public function close(): void { open = false; }

    public function transfer(int n): void {
        if (n < 0) {
            throw new Exception("negative transfer");
        }
        print("transferred " + n);
    }
}

function safeTransfer(Connection c, int n): void {
    c.connect();
    try {
        c.transfer(n);
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    } finally {
        c.close();
    }
}

function main(): void {
    Connection c = new Connection();
    safeTransfer(c, 100);
    print("open after #1: " + c.isOpen());
    safeTransfer(c, -5);
    print("open after #2: " + c.isOpen());
}
main();

// Test: Transaction rollback on exception
// Expected: Should rollback all changes when transaction fails
import * from "../../lib/exceptions/Exception.mt";

class TransactionException extends Exception {
    constructor(string message): super(message) {
    }
}

class DatabaseTransaction {
    public string transactionId;
    public Bool isActive;
    public Int operationCount;

    public constructor(string txId) {
        transactionId = txId;
        isActive = false;
        operationCount = 0;
    }

    public function begin(): void {
        print("BEGIN TRANSACTION " + transactionId);
        isActive = true;
        operationCount = 0;
    }

    public function execute(string operation): void {
        if (!isActive) {
            throw new TransactionException("Transaction not active");
        }
        print("EXEC: " + operation);
        operationCount = operationCount + 1;
    }

    public function commit(): void {
        if (!isActive) {
            throw new TransactionException("Cannot commit inactive transaction");
        }
        print("COMMIT TRANSACTION " + transactionId + " (" + operationCount + " operations)");
        isActive = false;
    }

    public function rollback(): void {
        if (isActive) {
            print("ROLLBACK TRANSACTION " + transactionId + " (" + operationCount + " operations undone)");
            isActive = false;
            operationCount = 0;
        }
    }
}

function testSuccessfulTransaction(): void {
    print("Test 1: Successful transaction");
    DatabaseTransaction tx = new DatabaseTransaction("TX001");

    try {
        tx.begin();
        tx.execute("INSERT INTO users VALUES (1, 'Alice')");
        tx.execute("UPDATE accounts SET balance = 1000 WHERE user_id = 1");
        tx.commit();
        print("Transaction completed successfully");
    } catch (Exception e) {
        print("Error: " + e.getMessage());
        tx.rollback();
    }
}

function testFailedTransaction(): void {
    print("Test 2: Failed transaction with rollback");
    DatabaseTransaction tx = new DatabaseTransaction("TX002");

    try {
        tx.begin();
        tx.execute("INSERT INTO users VALUES (2, 'Bob')");
        tx.execute("UPDATE accounts SET balance = 500 WHERE user_id = 2");
        throw new TransactionException("Constraint violation: insufficient funds");
        tx.commit();
    } catch (TransactionException e) {
        print("Transaction failed: " + e.getMessage());
        tx.rollback();
    }
    print("Transaction rolled back successfully");
}

function testNestedTransactions(): void {
    print("Test 3: Nested transactions with partial rollback");
    DatabaseTransaction outerTx = new DatabaseTransaction("OUTER");
    DatabaseTransaction innerTx = new DatabaseTransaction("INNER");

    try {
        outerTx.begin();
        outerTx.execute("CREATE SAVEPOINT outer_start");

        try {
            innerTx.begin();
            innerTx.execute("INSERT INTO logs VALUES ('operation1')");
            innerTx.execute("INSERT INTO logs VALUES ('operation2')");
            throw new Exception("Inner transaction validation failed");
            innerTx.commit();
        } catch (Exception e) {
            print("Inner transaction error: " + e.getMessage());
            innerTx.rollback();
        }

        outerTx.execute("CONTINUE after inner rollback");
        outerTx.commit();
        print("Outer transaction committed successfully");
    } catch (Exception e) {
        print("Outer transaction error: " + e.getMessage());
        outerTx.rollback();
    }
}

function testMultipleRollbacks(): void {
    print("Test 4: Multiple transactions with individual rollbacks");
    DatabaseTransaction tx1 = new DatabaseTransaction("TX_A");
    DatabaseTransaction tx2 = new DatabaseTransaction("TX_B");

    try {
        tx1.begin();
        tx1.execute("Operation A1");
        tx1.commit();
        print("TX_A committed");
    } catch (Exception e) {
        tx1.rollback();
    }

    try {
        tx2.begin();
        tx2.execute("Operation B1");
        throw new Exception("TX_B failed");
        tx2.commit();
    } catch (Exception e) {
        print("TX_B error: " + e.getMessage());
        tx2.rollback();
        print("TX_B rolled back");
    }
}

function main(): void {
    print("Testing transaction rollback on exceptions");
    testSuccessfulTransaction();
    print("---");
    testFailedTransaction();
    print("---");
    testNestedTransactions();
    print("---");
    testMultipleRollbacks();
    print("Transaction test completed");
}

main();

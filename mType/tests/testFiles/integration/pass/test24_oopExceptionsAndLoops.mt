// Test 24: OOP, Exceptions, and Loops
// Features: Inheritance, polymorphism, static methods, modifiers, final variables/classes, exceptions (try/catch/finally), loops (for, while, for each).

import * from "../../lib/collections/ArrayList.mt";

class ConnectionException extends Exception {
    public constructor(string message) : super(message) {
    }
}

abstract class Connection {
    protected string host;
    protected int port;
    
    public constructor(string host, int port) {
        this.host = host;
        this.port = port;
    }
    
    public abstract function connect(): void;
    public abstract function disconnect(): void;
    public abstract function isConnected(): bool;
}

final class SecureConnection extends Connection {
    private bool connected;
    private final string protocol = "TLSv1.3";
    
    public constructor(string host, int port) : super(host, port) {
        this.connected = false;
    }
    
    public function connect(): void {
        print("Connecting to " + this.host + ":" + this.port + " via " + this.protocol);
        if (this.port == 443) {
            this.connected = true;
            print("Connected securely.");
        } else {
            throw new ConnectionException("Port " + this.port + " is not secure.");
        }
    }
    
    public function disconnect(): void {
        this.connected = false;
        print("Disconnected.");
    }
    
    public function isConnected(): bool {
        return this.connected;
    }
}

class ConnectionManager {
    private final ArrayList<Connection> connections;
    private static int instanceCount = 0;
    
    public constructor() {
        this.connections = new ArrayList<Connection>();
        instanceCount++;
    }
    
    public static function getInstanceCount(): int {
        return instanceCount;
    }
    
    public function addConnection(Connection conn): void {
        this.connections.add(conn);
    }
    
    public function testConnections(): void {
        Connection[] connArray = this.connections.toArray();
        print("--- Testing via Standard For Loop ---");
        for (int i = 0; i < connArray.length; i = i + 1) {
            Connection conn = connArray[i];
            this.attemptConnection(conn);
        }
        
        print("--- Disconnecting via While Loop ---");
        int j = 0;
        while (j < connArray.length) {
            Connection conn = connArray[j];
            if (conn.isConnected()) {
                conn.disconnect();
            }
            j = j + 1;
        }
        
        print("--- Testing via Enhanced For-Each Loop ---");
        for (Connection conn : this.connections) {
            this.attemptConnection(conn);
            if (conn.isConnected()) {
                conn.disconnect();
            }
        }
    }
    
    private function attemptConnection(Connection conn): void {
        try {
            conn.connect();
        } catch (ConnectionException e) {
            print("Connection Failed: " + e.getMessage());
        } catch (Exception e) {
            print("Generic Error: " + e.getMessage());
        } finally {
            print("Connection attempt finished.");
        }
    }
}

function main(): void {
    print("--- Test 24: OOP, Exceptions, and Loops ---");
    
    ConnectionManager manager = new ConnectionManager();
    
    manager.addConnection(new SecureConnection("api.trusted.com", 443));
    manager.addConnection(new SecureConnection("api.legacy.com", 80));
    
    print("Manager count: " + ConnectionManager::getInstanceCount());
    manager.testConnections();
}

main();
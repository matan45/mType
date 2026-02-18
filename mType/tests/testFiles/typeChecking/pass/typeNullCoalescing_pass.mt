// Test null coalescing type checking
// Validates that null coalescing returns the correct type

class Configuration {
    string host;
    int port;

    constructor(string h, int p) {
        host = h;
        port = p;
    }

    public function getHost(): string {
        return host;
    }

    public function getPort(): int {
        return port;
    }
}

function getDefaultHost(): string {
    return "localhost";
}

function getDefaultPort(): int {
    return 8080;
}

function getConfigOrDefault(Configuration config): string {
    if (config != null) {
        return config.getHost();
    } else {
        return getDefaultHost();
    }
}

function getPortOrDefault(Configuration config): int {
    if (config != null) {
        return config.getPort();
    } else {
        return getDefaultPort();
    }
}

function main(): void {
    print("Testing null coalescing type checking");

    Configuration? config1 = null;
    Configuration config2 = new Configuration("example.com", 3000);

    // Coalesce null config to default host
    string host1 = getConfigOrDefault(config1);
    print("Host 1: " + host1);

    int port1 = getPortOrDefault(config1);
    print("Port 1: " + port1);

    // Use existing config values
    string host2 = getConfigOrDefault(config2);
    print("Host 2: " + host2);

    int port2 = getPortOrDefault(config2);
    print("Port 2: " + port2);

    // Type checking with fallback values
    string finalHost = config1 != null ? config1.getHost() : "default.com";
    print("Final host: " + finalHost);

    int finalPort = config2 != null ? config2.getPort() : 9000;
    print("Final port: " + finalPort);

    print("Null coalescing type checking test completed");
}

main();

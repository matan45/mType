namespace config {
int maxUsers = 100;
bool enableLogging = true;
string appName = "TestApp";

function getMaxUsers(): int {
    return maxUsers;
}

function isLoggingEnabled(): bool {
    return enableLogging;
}

function getAppName(): string {
    return appName;
}

namespace database {
    string host = "localhost";
    int port = 5432;
    bool useSSL = false;
    
    function getConnectionInfo(): int {
        return port;
    }
    
    function getHost(): string {
        return host;
    }
}
}

namespace settings {
int timeout = 30;
string theme = "dark";

function getTimeout(): int {
    return timeout;
}

function getTheme(): string {
    return theme;
}
}

// Test accessing namespace variables through functions
int users = config::getMaxUsers();
print(users);

bool logging = config::isLoggingEnabled();
print(logging);

int dbPort = config::database::getConnectionInfo();
print(dbPort);

int timeoutVal = settings::getTimeout();
print(timeoutVal);
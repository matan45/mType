// Simplified import test (without actual import statement due to .mt format limitations)
// This test simulates what would happen after importing math utilities

final float PI = 3.14159;

class Calculator {
    public static int operationCount = 0;

    public static function add(int a, int b): int {
        operationCount = operationCount + 1;
        return a + b;
    }

    public static function multiply(float a, float b): float {
        operationCount = operationCount + 1;
        return a * b;
    }

    public static function getOperationCount(): int {
        return operationCount;
    }
}

function circleArea(float radius): float {
    return PI * radius * radius;
}

final string APP_NAME = "Advanced Calculator";

class CalculationSession {
    public string sessionName;
    public int totalCalculations;
    public final int MAX_CALCULATIONS = 100;

    public constructor(string name) {
        sessionName = name;
        totalCalculations = 0;
    }

    public function performCalculation(int a, int b): int {
        if (totalCalculations >= MAX_CALCULATIONS) {
            return -1; // Session limit reached
        }
        
        totalCalculations = totalCalculations + 1;
        return Calculator::add(a, b);
    }
    
    public function calculateCircleArea(float radius): float {
        if (totalCalculations >= MAX_CALCULATIONS) {
            return -1.0;
        }

        totalCalculations = totalCalculations + 1;
        return circleArea(radius);
    }

    public function getSessionInfo(): string {
        return sessionName + ": " + totalCalculations + "/" + MAX_CALCULATIONS;
    }
}

function generateReport(CalculationSession session): string {
    string report = "Session Report for " + session.sessionName;
    report = report + " | Operations: " + Calculator::getOperationCount();
    return report;
}

// Test complex integration
CalculationSession session1 = new CalculationSession("UserSession1");
CalculationSession session2 = new CalculationSession("UserSession2");

int result1 = session1.performCalculation(10, 20);
int result2 = session2.performCalculation(15, 25);
print(result1);
print(result2);

float area1 = session1.calculateCircleArea(5.0);
float area2 = session2.calculateCircleArea(3.0);
print(area1);
print(area2);

print(session1.getSessionInfo());
print(session2.getSessionInfo());

string report1 = generateReport(session1);
string report2 = generateReport(session2);
print(report1);
print(report2);

print(Calculator::getOperationCount());
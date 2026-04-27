// Combo 07: Value Class + Interface + Casting + Pattern Matching
// Tests: value class implementing interface, cast + pattern matched

interface Measurable {
    function measure(): float;
    function unit(): string;
}

interface Comparable {
    function compareTo(Measurable other): int;
}

value class Distance implements Measurable {
    private float meters;

    public constructor(float meters) {
        this.meters = meters;
    }

    public function measure(): float {
        return this.meters;
    }

    public function unit(): string {
        return "m";
    }

    public function getMeters(): float {
        return this.meters;
    }

    public function toKilometers(): float {
        return this.meters / 1000.0;
    }

    public function toString(): string {
        return this.meters + "m";
    }
}

value class Weight implements Measurable {
    private float kilograms;

    public constructor(float kilograms) {
        this.kilograms = kilograms;
    }

    public function measure(): float {
        return this.kilograms;
    }

    public function unit(): string {
        return "kg";
    }

    public function getKilograms(): float {
        return this.kilograms;
    }

    public function toString(): string {
        return this.kilograms + "kg";
    }
}

value class Duration implements Measurable {
    private float seconds;

    public constructor(float seconds) {
        this.seconds = seconds;
    }

    public function measure(): float {
        return this.seconds;
    }

    public function unit(): string {
        return "s";
    }

    public function getSeconds(): float {
        return this.seconds;
    }

    public function toMinutes(): float {
        return this.seconds / 60.0;
    }

    public function toString(): string {
        return this.seconds + "s";
    }
}

function describeMeasurement(Measurable m): string {
    string result = "";
    match (m) {
        case Distance d -> {
            result = $"Distance: {d.getMeters()}m ({d.toKilometers()}km)";
        }
        case Weight w -> {
            result = $"Weight: {w.getKilograms()}kg";
        }
        case Duration dur -> {
            result = $"Duration: {dur.getSeconds()}s ({dur.toMinutes()}min)";
        }
        default -> {
            result = "Unknown: " + m.measure() + m.unit();
        }
    }
    return result;
}

function main(): void {
    print("=== Combo 07: Value Class + Interface + Cast + Match ===");

    // Create value class instances through interface
    Measurable[] measurements = [
        new Distance(1500.0),
        new Weight(72.5),
        new Duration(3600.0),
        new Distance(42195.0),
        new Weight(0.5)
    ];

    // Pattern match on interface type
    print("--- Describe each ---");
    for (int i = 0; i < measurements.length; i++) {
        print(describeMeasurement(measurements[i]));
    }

    // Casting from interface to value class
    print("--- Cast + access ---");
    for (int i = 0; i < measurements.length; i++) {
        Measurable m = measurements[i];
        if (m isClassOf Distance) {
            Distance d = (Distance)m;
            print($"Distance in km: {d.toKilometers()}");
        }
    }

    // Value semantics check: copy independence
    print("--- Value copy ---");
    Distance d1 = new Distance(100.0);
    Distance d2 = d1; // value copy
    print("d1: " + d1.toString());
    print("d2: " + d2.toString());

    // Interface polymorphism with value types
    print("--- Interface polymorphism ---");
    Measurable light = new Weight(0.001);
    Measurable heavy = new Weight(1000.0);
    print("Light: " + light.measure() + light.unit());
    print("Heavy: " + heavy.measure() + heavy.unit());

    print("=== Combo 07 Complete ===");
}

main();

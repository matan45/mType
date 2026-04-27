// Combo 01: Value Class + Generics + Stream API + Lambda
// Tests: value class in generic container, streamed with filter/map/reduce

import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

value class Temperature {
    private int degrees;
    private string scale;

    public constructor(int degrees, string scale) {
        this.degrees = degrees;
        this.scale = scale;
    }

    public function getDegrees(): int {
        return this.degrees;
    }

    public function getScale(): string {
        return this.scale;
    }

    public function toString(): string {
        return this.degrees + "°" + this.scale;
    }

    public function toFahrenheit(): Temperature {
        if (this.scale == "C") {
            return new Temperature(this.degrees * 9 / 5 + 32, "F");
        }
        return new Temperature(this.degrees, this.scale);
    }
}

// Generic wrapper that works with value classes
class Measurement<T> {
    private T value;
    private string label;

    public constructor(T value, string label) {
        this.value = value;
        this.label = label;
    }

    public function getValue(): T {
        return this.value;
    }

    public function getLabel(): string {
        return this.label;
    }

    public function toString(): string {
        return this.label + ": " + this.value.toString();
    }
}

function main(): void {
    print("=== Combo 01: Value Class + Generics + Stream ===");

    // Create value class array for streaming
    Temperature[] temps = [
        new Temperature(0, "C"),
        new Temperature(100, "C"),
        new Temperature(25, "C"),
        new Temperature(-10, "C"),
        new Temperature(37, "C")
    ];

    // Stream filter: only positive temperatures
    print("--- Positive temperatures ---");
    Stream<Temperature> positiveStream = Streams::of(temps)
        .filter(t -> t.getDegrees() > 0);
    Temperature[] positiveTemps = positiveStream.toArray();

    for (Temperature t : positiveTemps) {
        print(t.toString());
    }

    // Stream map: convert to Fahrenheit
    print("--- Converted to Fahrenheit ---");
    Stream<Temperature> fahrenheitStream = Streams::of(temps)
        .filter(t -> t.getDegrees() >= 0)
        .map(t -> t.toFahrenheit());
    Temperature[] fahrenheitTemps = fahrenheitStream.toArray();

    for (Temperature t : fahrenheitTemps) {
        print(t.toString());
    }

    // Generic Measurement with value class
    print("--- Generic measurements ---");
    ArrayList<Measurement<Temperature>> measurements = new ArrayList<Measurement<Temperature>>();
    measurements.add(new Measurement<Temperature>(new Temperature(20, "C"), "Room"));
    measurements.add(new Measurement<Temperature>(new Temperature(37, "C"), "Body"));
    measurements.add(new Measurement<Temperature>(new Temperature(100, "C"), "Boiling"));

    for (int i = 0; i < measurements.size(); i++) {
        Measurement<Temperature> m = measurements.get(i);
        print(m.toString());
    }

    // Stream count
    print("--- Counts ---");
    int positiveCount = Streams::of(temps)
        .filter(t -> t.getDegrees() > 0)
        .count();
    print("Positive temps: " + positiveCount);

    int freezingCount = Streams::of(temps)
        .filter(t -> t.getDegrees() <= 0)
        .count();
    print("Freezing temps: " + freezingCount);

    print("=== Combo 01 Complete ===");
}

main();

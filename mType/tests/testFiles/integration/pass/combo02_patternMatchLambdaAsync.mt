// Combo 02: Pattern Matching + Lambda + Async
// Tests: async function using match on polymorphic types, dispatching lambdas

import * from "../../lib/primitives/String.mt";

abstract class Event {
    private string timestamp;

    public constructor(string ts) {
        this.timestamp = ts;
    }

    public function getTimestamp(): string {
        return this.timestamp;
    }

    abstract function describe(): string;
}

class ClickEvent extends Event {
    private int x;
    private int y;

    public constructor(string ts, int x, int y) : super(ts) {
        this.x = x;
        this.y = y;
    }

    public function getX(): int { return this.x; }
    public function getY(): int { return this.y; }

    public function describe(): string {
        return "Click at (" + this.x + ", " + this.y + ")";
    }
}

class KeyEvent extends Event {
    private string key;

    public constructor(string ts, string key) : super(ts) {
        this.key = key;
    }

    public function getKey(): string { return this.key; }

    public function describe(): string {
        return "Key pressed: " + this.key;
    }
}

class ScrollEvent extends Event {
    private int delta;

    public constructor(string ts, int delta) : super(ts) {
        this.delta = delta;
    }

    public function getDelta(): int { return this.delta; }

    public function describe(): string {
        return "Scroll delta: " + this.delta;
    }
}

interface EventHandler {
    function handle(string info): string;
}

// Async event processor using pattern matching + lambda
function async processEvent(Event event): Promise<String> {
    string result = "";

    match (event) {
        case ClickEvent click -> {
            EventHandler handler = info -> "CLICK_HANDLED[" + info + "]";
            result = handler.handle(click.getX() + "," + click.getY());
        }
        case KeyEvent key -> {
            EventHandler handler = info -> "KEY_HANDLED[" + info + "]";
            result = handler.handle(key.getKey());
        }
        case ScrollEvent scroll -> {
            EventHandler handler = info -> "SCROLL_HANDLED[" + info + "]";
            result = handler.handle("" + scroll.getDelta());
        }
        default -> {
            result = "UNKNOWN";
        }
    }

    return new String(result);
}

function async processAllEvents(Event[] events): Promise<void> {
    for (int i = 0; i < events.length; i++) {
        Event e = events[i];
        String resultObj = await processEvent(e);
        print(e.getTimestamp() + " -> " + resultObj.getValue());
    }
}

function async main(): Promise<void> {
    print("=== Combo 02: Pattern Match + Lambda + Async ===");

    Event[] events = [
        new ClickEvent("T1", 100, 200),
        new KeyEvent("T2", "Enter"),
        new ScrollEvent("T3", -50),
        new ClickEvent("T4", 0, 0),
        new KeyEvent("T5", "Escape")
    ];

    await processAllEvents(events);

    print("--- Single event ---");
    String single = await processEvent(new ScrollEvent("T6", 999));
    print("Result: " + single.getValue());

    print("=== Combo 02 Complete ===");
}

main();

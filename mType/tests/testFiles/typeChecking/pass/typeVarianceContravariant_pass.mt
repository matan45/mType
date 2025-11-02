// Test: Contravariant parameter types (function parameter flexibility)
// Expected: Pass - testing parameter type flexibility in inheritance

class Event {
    protected string eventType;

    public constructor(string type) {
        this.eventType = type;
    }

    public string getType() {
        return this.eventType;
    }

    public void describe() {
        print("Event: " + this.eventType);
    }
}

class MouseEvent extends Event {
    private int x;
    private int y;

    public constructor(int x, int y) : super("MouseEvent") {
        this.x = x;
        this.y = y;
    }

    public void describe() {
        print("MouseEvent at (" + this.x + ", " + this.y + ")");
    }

    public int getX() {
        return this.x;
    }

    public int getY() {
        return this.y;
    }
}

class ClickEvent extends MouseEvent {
    private int button;

    public constructor(int x, int y, int button) : super(x, y) {
        this.button = button;
    }

    public void describe() {
        print("ClickEvent at (" + this.getX() + ", " + this.getY() + ") button: " + this.button);
    }

    public int getButton() {
        return this.button;
    }
}

// Handler hierarchy demonstrating contravariance in parameter acceptance
class EventHandler {
    public void handle(Event e) {
        print("Handling generic event");
        e.describe();
    }

    public void process(Event e) {
        print("Processing event of type: " + e.getType());
        this.handle(e);
    }
}

class MouseEventHandler extends EventHandler {
    public void handle(Event e) {
        print("Handling as mouse event");
        e.describe();
    }
}

class ClickEventHandler extends MouseEventHandler {
    public void handle(Event e) {
        print("Handling as click event");
        e.describe();
    }
}

// Test 1: Basic handler hierarchy
print("Test 1: Event handler hierarchy");
EventHandler handler1 = new EventHandler();
Event event1 = new Event("Generic");
handler1.process(event1);

print("\nTest 2: Mouse event handler with upcast");
MouseEventHandler handler2 = new MouseEventHandler();
MouseEvent event2 = new MouseEvent(10, 20);
handler2.process(event2);  // MouseEvent upcast to Event parameter

print("\nTest 3: Click event handler");
ClickEventHandler handler3 = new ClickEventHandler();
ClickEvent event3 = new ClickEvent(30, 40, 1);
handler3.process(event3);  // ClickEvent upcast to Event parameter

// Test 4: Polymorphic handler array
print("\nTest 4: Polymorphic handler array");
EventHandler[] handlers = new EventHandler[3];
handlers[0] = new EventHandler();
handlers[1] = new MouseEventHandler();
handlers[2] = new ClickEventHandler();

Event testEvent = new ClickEvent(50, 60, 2);
int i = 0;
while (i < 3) {
    print("\nHandler " + i + ":");
    handlers[i].process(testEvent);
    i = i + 1;
}

// Test 5: Function accepting broader type (contravariance simulation)
function dispatchEvent(EventHandler h, Event e): void {
    print("Dispatching event");
    h.handle(e);
}

print("\nTest 5: Event dispatch function");
ClickEventHandler specificHandler = new ClickEventHandler();
Event broadEvent = new MouseEvent(70, 80);
dispatchEvent(specificHandler, broadEvent);

// Test 6: Parameter type flexibility
class Processor {
    public void processItem(Event item) {
        print("Processing: " + item.getType());
    }

    public void batchProcess(Event[] items, int count) {
        int idx = 0;
        while (idx < count) {
            this.processItem(items[idx]);
            idx = idx + 1;
        }
    }
}

print("\nTest 6: Batch processing with type flexibility");
Processor processor = new Processor();
Event[] events = new Event[3];
events[0] = new Event("Custom");
events[1] = new MouseEvent(100, 200);
events[2] = new ClickEvent(300, 400, 3);
processor.batchProcess(events, 3);

print("\nContravariant type tests completed");

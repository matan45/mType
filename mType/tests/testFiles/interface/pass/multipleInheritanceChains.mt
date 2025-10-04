// Test multiple independent inheritance chains to stress the validation cache

// Chain 1: Data processing interfaces
interface DataSource {
    function getData(): string;
}

interface DataProcessor extends DataSource {
    function processData(string data): string;
}

interface DataValidator extends DataProcessor {
    function validateData(string data): bool;
}

interface DataPersister extends DataValidator {
    function persistData(string data): void;
}

// Chain 2: Event handling interfaces
interface EventListener {
    function onEvent(string eventType): void;
}

interface EventHandler extends EventListener {
    function handleEvent(string eventType, string payload): void;
}

interface EventDispatcher extends EventHandler {
    function dispatchEvent(string eventType, string payload): void;
}

interface EventAggregator extends EventDispatcher {
    function aggregateEvents(): void;
}

// Chain 3: UI component interfaces
interface Renderable {
    function render(): void;
}

interface Interactive extends Renderable {
    function onClick(): void;
}

interface Focusable extends Interactive {
    function onFocus(): void;
    function onBlur(): void;
}

interface Draggable extends Focusable {
    function onDragStart(): void;
    function onDragEnd(): void;
}

// Implementation that uses all chains
class ComplexComponent implements DataPersister, EventAggregator, Draggable {
    // DataSource methods
    public function getData(): string { return "test data"; }

    // DataProcessor methods
    public function processData(string data): string { return "processed: " + data; }

    // DataValidator methods
    public function validateData(string data): bool { return true; }

    // DataPersister methods
    public function persistData(string data): void { print("Persisting: " + data); }

    // EventListener methods
    public function onEvent(string eventType): void { print("Event: " + eventType); }

    // EventHandler methods
    public function handleEvent(string eventType, string payload): void {
        print("Handling: " + eventType + " with " + payload);
    }

    // EventDispatcher methods
    public function dispatchEvent(string eventType, string payload): void {
        print("Dispatching: " + eventType);
    }

    // EventAggregator methods
    public function aggregateEvents(): void { print("Aggregating events"); }

    // Renderable methods
    public function render(): void { print("Rendering component"); }

    // Interactive methods
    public function onClick(): void { print("Click handled"); }

    // Focusable methods
    public function onFocus(): void { print("Focus gained"); }
    public function onBlur(): void { print("Focus lost"); }

    // Draggable methods
    public function onDragStart(): void { print("Drag started"); }
    public function onDragEnd(): void { print("Drag ended"); }
}

function main(): void {
    print("Testing multiple interface inheritance chains...");

    ComplexComponent component = new ComplexComponent();

    // Test data processing chain
    string data = component.getData();
    string processed = component.processData(data);
    bool valid = component.validateData(processed);
    if (valid) {
        component.persistData(processed);
    }

    // Test event handling chain
    component.onEvent("test");
    component.handleEvent("click", "button1");
    component.dispatchEvent("custom", "payload");
    component.aggregateEvents();

    // Test UI component chain
    component.render();
    component.onClick();
    component.onFocus();
    component.onDragStart();
    component.onDragEnd();
    component.onBlur();

    print("Multiple inheritance chains test completed!");
}

main();
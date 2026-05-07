// Combo 15: Grand Feature Showcase

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Annotation.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/exceptions/Exception.mt";

// --- Annotations ---
@Retention(RUNTIME)
@Target([CLASS])
annotation Component {
    string name = "";
}

// --- Interfaces ---
interface Renderable {
    function render(): string;
}

interface Clickable {
    function onClick(): void;
}

interface Formatter {
    function format(string value): string;
}

// --- Abstract class with generics ---
abstract class Widget<T> implements Renderable {
    protected T data;
    protected string id;

    public constructor(string id, T data) {
        this.id = id;
        this.data = data;
    }

    public function getId(): string { return this.id; }
    public function getData(): T { return this.data; }

    public abstract function render(): string;
}

// --- Final class ---
@Component(name = "Button")
final class Button extends Widget<String> implements Clickable {
    private int clickCount;

    public constructor(string id, String label) : super(id, label) {
        this.clickCount = 0;
    }

    public function render(): string {
        return $"<button id='{this.id}'>{this.data.getValue()}</button>";
    }

    public function onClick(): void {
        this.clickCount = this.clickCount + 1;
    }

    public function getClickCount(): int {
        return this.clickCount;
    }
}

class Label extends Widget<Int> {
    public constructor(string id, Int value) : super(id, value) {}

    public function render(): string {
        return $"<label id='{this.id}'>{this.data.getValue()}</label>";
    }
}

class Panel extends Widget<String> {
    private ArrayList<Renderable> children;

    public constructor(string id) : super(id, new String("panel")) {
        this.children = new ArrayList<Renderable>();
    }

    public function addChild(Renderable child): void {
        this.children.add(child);
    }

    public function render(): string {
        string result = $"<panel id='{this.id}'>";
        for (Renderable child : this.children) {
            result = result + "\n  " + child.render();
        }
        result = result + "\n</panel>";
        return result;
    }
}

class RenderException extends Exception {
    public constructor(string msg) : super(msg) {}
}

// --- Static overloaded factory ---
class WidgetFactory {
    public static function create(string type, string id): Renderable {
        Renderable result = new Panel(id);
        if (type == "button") {
            result = new Button(id, new String("Click Me"));
        } else if (type == "label") {
            result = new Label(id, new Int(0));
        }
        return result;
    }

    public static function create(string type, string id, string label): Renderable {
        Renderable result = new Panel(id);
        if (type == "button") {
            result = new Button(id, new String(label));
        }
        return result;
    }
}

function async renderWidget(Renderable widget): Promise<String> {
    try {
        string html = widget.render();
        return new String(html);
    } catch (Exception e) {
        throw new RenderException("Render failed: " + e.getMessage());
    }
}

function describeWidget(Renderable widget): string {
    match (widget) {
        case Button b -> {
            return $"Button '{b.getId()}' clicked {b.getClickCount()} times";
        }
        case Label l -> {
            return $"Label '{l.getId()}' value={l.getData().getValue()}";
        }
        case Panel p -> {
            return $"Panel '{p.getId()}'";
        }
        default -> {
            return "Unknown widget";
        }
    }
}

function async main(): Promise<void> {
    print("=== Combo 15: Grand Feature Showcase ===");

    print("--- Factory creation ---");
    Renderable btn1 = WidgetFactory::create("button", "btn1", "Submit");
    Renderable btn2 = WidgetFactory::create("button", "btn2");
    Renderable lbl = WidgetFactory::create("label", "lbl1");

    print("--- Async render ---");
    String html1 = await renderWidget(btn1);
    print(html1.getValue());
    String html2 = await renderWidget(lbl);
    print(html2.getValue());

    print("--- Pattern matching ---");
    Renderable[] widgets = [btn1, btn2, lbl];
    for (int i = 0; i < widgets.length; i++) {
        print(describeWidget(widgets[i]));
    }

    print("--- Casting + Clickable ---");
    if (btn1 isClassOf Button) {
        Button b = (Button)btn1;
        b.onClick();
        b.onClick();
        b.onClick();
        print($"Button clicked {b.getClickCount()} times");
    }

    print("--- Panel composition ---");
    Panel panel = new Panel("mainPanel");
    panel.addChild(btn1);
    panel.addChild(lbl);
    panel.addChild(WidgetFactory::create("button", "inner", "OK"));
    String panelHtml = await renderWidget(panel);
    print(panelHtml.getValue());

    print("--- Reflection ---");
    Class btnClass = Class::forName("Button");
    print("Class: " + btnClass.getName());
    if (btnClass.hasAnnotation("Component")) {
        Annotation? ann = btnClass.getAnnotation("Component");
        if (ann != null) {
            print("Component name: " + ann.getString("name"));
        }
    }

    print("--- Lambda + interpolation ---");
    final string tag = "widget";
    Formatter f = value -> $"<{tag}>{value}</{tag}>";
    string[] names = ["alpha", "beta", "gamma"];
    for (string name : names) {
        print(f.format(name));
    }

    print("--- Error handling ---");
    try {
        final int divisor = 0;
        if (divisor == 0) {
            throw new RenderException("Cannot divide by zero");
        }
    } catch (RenderException e) {
        print("Caught: " + e.getMessage());
    } finally {
        print("Finally executed");
    }

    print("=== Combo 15 Complete ===");
}

main();

// Test: Box<T> over a user-defined class (not a primitive wrapper).
// Confirms the reflection machinery doesn't hardcode primitive names —
// any registered class can be a type argument.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/primitives/Box.mt";

class Widget {
    public int id;
    public string label;

    public constructor(int id, string label) {
        this.id = id;
        this.label = label;
    }
}

Class boxWidget = Class::forName("Box<Widget>");

print("getName: " + boxWidget.getName());
print("getRawName: " + boxWidget.getRawName());
print("isClosed: " + boxWidget.isClosed());

Class[] args = boxWidget.getTypeArguments();
print("Arg count: " + args.length);
print("Arg getName: " + args[0].getName());
print("Arg isGeneric: " + args[0].isGeneric());

// The inner Class for Widget should be reflection-equivalent to what
// Class::forName("Widget") returns — same handle.
Class widgetDirect = Class::forName("Widget");
print("Inner handle matches Widget: " +
      (args[0].getNativeHandle() == widgetDirect.getNativeHandle()));

print("Test passed");

// Reflect on a class that implements an interface — pins that the
// implementor's getMethods/getDeclaredMethods correctly surface interface
// methods, and that the implementor's getInterfaces lists every interface.
//
// interfaceReflection.mt only counts/names the interfaces, never enumerates
// the implementor's methods to verify interface-required ones are present.
// reflectInheritedMethods.mt only covers a single concrete-class chain
// (no interfaces).
//
// Note: Class::forName does not resolve interface type names today
// (MT-E5005 "Class not found: <Interface>"), so this test reflects via the
// implementing class.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

interface Drawable {
    function draw(): void;
    function getName(): string;
}

interface Resizable {
    function resize(int width, int height): void;
}

class Widget implements Drawable, Resizable {
    public int width;
    public int height;

    public constructor() {
        this.width = 0;
        this.height = 0;
    }

    public function draw(): void { print("drew " + this.width + "x" + this.height); }
    public function getName(): string { return "Widget"; }
    public function resize(int w, int h): void { this.width = w; this.height = h; }
    public function own(): int { return 7; }
}

Class widgetClass = Class::forName("Widget");
print("Widget.isInterface=" + widgetClass.isInterface());

string[] ifs = widgetClass.getInterfaces();
print("interface count=" + ifs.length);
bool hasDrawable = false;
bool hasResizable = false;
for (int i = 0; i < ifs.length; i = i + 1) {
    if (ifs[i] == "Drawable") { hasDrawable = true; }
    if (ifs[i] == "Resizable") { hasResizable = true; }
}
print("has Drawable=" + hasDrawable);
print("has Resizable=" + hasResizable);

Method[] declared = widgetClass.getDeclaredMethods();
print("declared count=" + declared.length);

bool sawDraw = false;
bool sawGetName = false;
bool sawResize = false;
bool sawOwn = false;
for (Method m : declared) {
    if (m.getName() == "draw") { sawDraw = true; }
    if (m.getName() == "getName") { sawGetName = true; }
    if (m.getName() == "resize") { sawResize = true; }
    if (m.getName() == "own") { sawOwn = true; }
}
print("declared has draw=" + sawDraw);
print("declared has getName=" + sawGetName);
print("declared has resize=" + sawResize);
print("declared has own=" + sawOwn);

Method drawMethod = widgetClass.getDeclaredMethod("draw", 0);
print("draw param count=" + drawMethod.getParameterCount());
print("draw return type=" + drawMethod.getReturnType());

Method resizeMethod = widgetClass.getDeclaredMethod("resize", 2);
print("resize param count=" + resizeMethod.getParameterCount());

print("Test passed");

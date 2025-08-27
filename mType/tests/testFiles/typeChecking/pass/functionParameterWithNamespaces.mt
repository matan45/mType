namespace utils {
    class Helper {
        int value;
        constructor(int v) {
            value = v;
        }
    }
    
    function processHelper(Helper h): int {
        return h.value * 2;
    }
}

namespace app {
    class Widget {
        string name;
        constructor(string n) {
            name = n;
        }
    }
    
    function processWidget(Widget w, utils::Helper h): string {
        return w.name + ": " + toString(h.value);
    }
}

// Test with namespace-qualified types
utils::Helper helper = new utils::Helper(5);
app::Widget widget = new app::Widget("TestWidget");

int result1 = utils::processHelper(helper);
string result2 = app::processWidget(widget, helper);

print(toString(result1));
print(result2);
class Widget {
    int id;
    constructor(int i) {
        id = i;
    }
}

function process(Widget w, string name, int count): void {
    print(name + ": " + toString(w.id) + " x " + toString(count));
}

Widget widget = new Widget(101);

// Wrong order: string, int, Widget instead of Widget, string, int
process("TestWidget", 5, widget);
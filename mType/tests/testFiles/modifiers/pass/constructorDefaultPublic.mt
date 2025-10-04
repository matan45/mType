// Test: Constructors default to public
class Widget {
    public int id;

    constructor(int wid) {  // No modifier = public by default
        id = wid;
    }
}

// Constructor accessible externally (default public)
Widget w = new Widget(10);
print(w.id);  // Expected: 10

class Widget {
    int value;
    constructor(int v) {
        value = v;
    }
}

Widget? w1 = new Widget(42);
Widget? w2;

// Null assignments should work
w2 = null;
w1 = null;

// Assignment from null should work
Widget? w3 = new Widget(100);
w2 = w3;  // w2 was null, now gets w3
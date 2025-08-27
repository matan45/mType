class Widget {
    int id;
    static int nextId = 1;
    
    constructor() {
        id = nextId;
        nextId = nextId + 1;
    }
    
    function getId(): int {
        return id;
    }
}

Widget w1 = new Widget();
print(w1.getId()); // 1

Widget w2 = new Widget();
print(w2.getId()); // 2

Widget w3 = new Widget();
print(w3.getId()); // 3

// Create in loop
for (int i = 0; i < 3; i++) {
    Widget w = new Widget();
    print(w.getId()); // 4, 5, 6
}
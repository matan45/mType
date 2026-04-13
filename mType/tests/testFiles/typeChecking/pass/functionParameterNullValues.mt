class Data {
    public int value;

    constructor() {
        value = 0;
    }
}

function processData(Data? d): void {
    if (d != null) {
        print("Data value: " + d.value);
    } else {
        print("Data is null");
    }
}

Data data1 = new Data();
data1.value = 42;
processData(data1);  // Should work

Data? data2 = null;
processData(data2);  // Should work - null is valid for object types
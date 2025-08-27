class Data {
    int value;
    string label;
    
    constructor() {
        value = 0;
        label = "default";
    }
}

Data d1 = new Data();
Data d2 = new Data();

d1.value = 42;
d1.label = "answer";

d2.value = 100;
d2.label = "century";

print(d1.value); // 42
print(d1.label); // answer
print(d2.value); // 100
print(d2.label); // century

// Assign one object to another
d2 = d1;
print(d2.value); // 42
print(d2.label); // answer
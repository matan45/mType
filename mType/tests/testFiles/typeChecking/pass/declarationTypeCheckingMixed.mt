class Phone {
    string model;
    constructor(string m) {
        model = m;
    }
}

class Computer {
    string brand;
    constructor(string b) {
        brand = b;
    }
}

Phone phone1 = new Phone("iPhone");     // Valid
Phone phone2 = new Phone("Samsung");    // Valid
phone1 = phone2;                        // Valid assignment

Computer comp = new Computer("Dell");   // Valid
print("Mixed scenarios successful");
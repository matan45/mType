// Basic function overloading - different parameter counts
function print2(int x): void {
    print("int: " + x);
}

function print2(int x, int y): void {
    print("int, int: " + x + ", " + y);
}

function print2(string s): void {
    print("String: " + s);
}


function main(): void {
    print2(42);           // Should call print(int)
    print2(10, 20);       // Should call print(int, int)
    print2("Hello");      // Should call print(String)
}

main();

// Consumer interface lambda test
interface Consumer {
    function accept(value: int) : void;
}

print("=== Consumer Interface Test ===");

Consumer printer = value -> print("Consumed: " + value);
Consumer doubleprinter = x -> print("Double consumed: " + (x * 2));

printer.accept(10);
doubleprinter.accept(7);
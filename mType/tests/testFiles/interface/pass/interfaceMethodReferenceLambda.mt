// Test method reference to interface method (if supported)

import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/String.mt";

interface Printer {
    function printMessage(string message): void;
}

class ConsolePrinter implements Printer {
    private string prefix;

    public constructor(string prefix) {
        this.prefix = prefix;
    }

    public function printMessage(string message): void {
        print(this.prefix + message);
    }
}

class MessageProcessor {
    public function process(ArrayList<String> messages, Printer printer): void {
        int i = 0;
        while (i < messages.size()) {
            printer.printMessage(messages.get(i));
            i = i + 1;
        }
    }
}

ArrayList<String> messages = new ArrayList<String>();
messages.add(new String("Hello"));
messages.add(new String("World"));
messages.add(new String("Test"));

MessageProcessor processor = new MessageProcessor();
ConsolePrinter consolePrinter = new ConsolePrinter("[INFO] ");

// Pass interface implementation
processor.process(messages, consolePrinter);

// Alternative with different printer
ConsolePrinter errorPrinter = new ConsolePrinter("[ERROR] ");
processor.process(messages, errorPrinter);

// Wrong parameter count in interface implementation
interface Processor {
    function process(string input, int mode): string;
}

class BadProcessor implements Processor {
    function process(string input): string {  // Missing 'mode' parameter
        return input;
    }
}

print("This should not print");
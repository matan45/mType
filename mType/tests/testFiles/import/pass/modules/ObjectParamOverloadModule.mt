public class Wrapper {
    public int value;

    public function init(int v): void {
        value = v;
    }
}

public class Processor {
    public static function process(int x): string {
        return "int:" + x;
    }

    public static function process(string s): string {
        return "string:" + s;
    }

    public static function process(Wrapper w): string {
        return "wrapper:" + w.value;
    }
}

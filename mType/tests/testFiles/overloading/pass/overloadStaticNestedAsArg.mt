// Test: An overloaded static call's result is passed as an argument to
// another overloaded static call. Type inference must resolve the inner
// overload first so the outer overload-resolver sees the correct argument
// class for its scoring.

class Wrapper {
    public string content;
    public constructor(string c) { this.content = c; }
}

class Source {
    public static function get(string key): Wrapper {
        return new Wrapper("value-" + key);
    }
    public static function get(string key, string fallback): Wrapper {
        return new Wrapper(key + "/" + fallback);
    }
}

class Sink {
    public static function consume(Wrapper w): void {
        print("one: " + w.content);
    }
    public static function consume(Wrapper w, string tag): void {
        print(tag + ": " + w.content);
    }
}

Sink::consume(Source::get("a"));
Sink::consume(Source::get("a", "b"));
Sink::consume(Source::get("c"), "tagged");
Sink::consume(Source::get("c", "d"), "both");

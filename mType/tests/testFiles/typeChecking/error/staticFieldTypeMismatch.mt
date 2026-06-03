// MYT-377: now that `Class::FIELD` infers the field's real declared type,
// passing a string static field into an int parameter must still be rejected
// — on the correct ground ("expects int but got string"), not masked as void.

class Cfg {
    public static final string HOST = "localhost";
}

function takeInt(int x): void { print(x); }

takeInt(Cfg::HOST);

// Tier A: returning a ternary whose two branches have different nullable
// shapes. The return path should agree with the assignment path on the
// inferred nullable status of the whole expression.
// Targets: TypeInferenceEngine.cpp:inferExpressionNullable; FunctionCompiler.cpp:306.

class Box {
    private string text;

    public constructor(string t) {
        this.text = t;
    }

    public function getText(): string {
        return this.text;
    }
}

function makeBox(): Box {
    return new Box("hello");
}

// Nullable return type — both arms (concrete Box and null literal) must be
// accepted. Mirrors the assignment `Box? local = cond ? makeBox() : null;`.
function pickBoxOrNull(bool useReal): Box? {
    if (useReal) {
        return makeBox();
    }
    return null;
}

Box? a = pickBoxOrNull(true);
Box? b = pickBoxOrNull(false);

if (a != null) {
    print("a: " + a.getText());
}
if (b == null) {
    print("b: null");
}

// Tier A: function declared with a non-nullable object return type returns
// a value the inference proves nullable. The assignment-statement path
// rejects nullable->non-nullable; the return path's pre-check at
// FunctionCompiler.cpp:306 must reject it too.
// Targets: FunctionCompiler.cpp:303-313.

class Box {
    private string text;

    public constructor(string t) {
        this.text = t;
    }

    public function getText(): string {
        return this.text;
    }
}

function maybeBox(): Box? {
    return null;
}

// Declared non-nullable Box, but returns a Box? expression directly.
function bad(): Box {
    return maybeBox();
}

Box b = bad();
print(b.getText());

// Tier C: dereferencing a nullable object reference without a null check
// or null-safe operator should error. Negative counterpart of
// typeNullPropagation_pass / nullableChainSafe_pass.
// Targets: null-safety check in TypeInferenceEngine / TypeValidator.

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

Box? b = maybeBox();

// Direct method call on a nullable value without `?.` or null guard.
print(b.getText());

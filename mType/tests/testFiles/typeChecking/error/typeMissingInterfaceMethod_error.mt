// Tier C: a class declared to implement an interface but missing one of
// the required methods must be rejected. Negative counterpart of
// typeInterfaceImplementation_pass.
// Targets: ClassRegistrar / interface conformance check at class registration.

interface Drawable {
    function draw(): void;
    function getDescription(): string;
}

// Missing draw() — only getDescription is implemented.
class BadShape implements Drawable {
    public constructor() {}

    public function getDescription(): string {
        return "incomplete";
    }
}

BadShape s = new BadShape();
print(s.getDescription());

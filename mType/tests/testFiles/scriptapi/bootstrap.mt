// MYT-42 — shared bootstrap for the ScriptApiNativeTestSuite.
//
// Declares classes and helper functions only. Callbacks obtain their
// subjects via ScriptAPI::callFunction, which matches the pattern used
// by existing SCRIPT_INTEROP tests and avoids any dependency on
// top-level variable persistence.

import * from "../lib/reflect/Class.mt";
import * from "../lib/primitives/Int.mt";
import * from "../lib/primitives/Box.mt";

class Animal {
    public constructor() {}
}

function makeBox(): Box<Int> {
    return new Box<Int>(42);
}

function makeAnimal(): Animal {
    return new Animal();
}

// Captures the native handle of Class.forName("Box<Int>") from the
// language side, so C++ tests can assert cross-surface handle identity.
function langForNameHandle(): int {
    return Class::forName("Box<Int>").getNativeHandle();
}

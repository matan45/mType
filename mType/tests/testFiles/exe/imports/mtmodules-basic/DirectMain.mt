// MYT-310 — top-level script invoked via `mType.exe DirectMain.mt`.
// The ambient .mtproj sibling triggers tryLoadAmbientProject, which
// scans mt_modules/ and registers @greetlib. Output must match
// DirectMain.expected. Lives outside src/ so the package build glob
// (<Include>src/**/*.mt</Include>) doesn't pick it up.
import { Greet } from "@greetlib/Greet.mt";

print(Greet::hello("MYT-304"));

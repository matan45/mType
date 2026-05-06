// Edge: absolute path that obviously points outside any reasonable project
// root. Either enforceWithinProjectRoot rejects it (when allowedRoots is
// configured) or the file simply does not exist — either is acceptable as
// long as the compiler errors cleanly rather than crashing.
import { Anything } from "C:/Windows/system32/cmd.mt";

print("should not reach here");

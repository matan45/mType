// Edge: a transitively imported module has a syntax error. The compiler must
// surface that error (potentially via Wrapper.mt importing Bad.mt) rather
// than silently swallowing it as "missing symbol" in this top-level file.
import { Wrapper } from "./transitiveBroken/Wrapper.mt";

print("should not reach here");

// Edge: backslash separators in an import path. On Windows fs::path may
// accept them as separators; on POSIX they're literal characters. Either way
// the target file does not exist, so the test passes as long as ANY error is
// raised (no segfault, no silent accept).
import { Foo } from ".\nonexistent\Foo.mt";

print("should not reach here");

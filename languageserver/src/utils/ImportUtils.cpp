#include "ImportUtils.hpp"

namespace mtype::lsp::utils
{
    TextEdit buildImportInsertEdit(
        int insertLine,
        const std::string& identifier,
        const std::string& spelling)
    {
        // mType supports both `import * from "...mt";` (wildcard) and
        // `import { Foo } from "...mt";` (selective). The selective form
        // is preferred for auto-generated imports because it surfaces
        // exactly what the new code needs and avoids dragging in
        // unrelated symbols. Generic classes use the base name only —
        // `<T>` lives at the use site (e.g. `new Box<Int>(...)`), not
        // in the import.
        TextEdit edit;
        edit.range.start.line = insertLine;
        edit.range.start.character = 0;
        edit.range.end = edit.range.start;
        edit.newText = "import { " + identifier + " } from \"" + spelling + "\";\n";
        return edit;
    }
}

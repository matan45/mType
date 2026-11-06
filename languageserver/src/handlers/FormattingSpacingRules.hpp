#pragma once

#include <string>
#include <string_view>
#include "../utils/SourceSegmenter.hpp"

namespace mtype::lsp {

/**
 * Token-spacing rules applied by the document formatter: `x=1` -> `x = 1`,
 * `a,b` -> `a, b`, `x:int` -> `x: int`, `if(` -> `if (`.
 *
 * Rules only ever touch Code segments. String literals and comments are
 * masked out before the rules run and restored afterwards, so their text is
 * never rewritten (MYT-443). Every rule reaches its fixed point in one pass,
 * which keeps formatting idempotent.
 */
class FormattingSpacingRules {
public:
    static std::string apply(std::string_view line, const utils::LineScan& scan);

private:
    static std::string maskNonCode(std::string_view line, const utils::LineScan& scan);
    static std::string spaceCode(std::string text);
    static std::string restoreNonCode(const std::string& spaced, std::string_view line,
                                      const utils::LineScan& scan);
};

} // namespace mtype::lsp

#include "FormattingSpacingRules.hpp"

#include <algorithm>
#include <regex>
#include <vector>

namespace mtype::lsp {

namespace {

// Stands in for one whole string literal or comment while the rules run. Like
// the `"` or `/` it replaces, it is neither a word character nor whitespace,
// so `foo(a,"b")` still becomes `foo(a, "b")`.
constexpr char kMaskByte = '\x02';
const std::string kScopeMask = "\x01SCOPE\x01";

// Keywords that get a space before `(` or `{` (`if(` -> `if (`). They match
// only as whole words directly followed by the bracket, so identifiers that
// merely start with a keyword (forEach, finally, newValue, className) are
// never split. `foreach` is not an mType keyword and `from` is also used as a
// variable name, so neither is listed.
const char* const kSpacedKeywordsPattern =
    "\\b(class|interface|function|if|else|while|for|return|new|import|"
    "public|private|protected|static|final|abstract|async|await)(?=[({])";

bool hasCode(const utils::LineScan& scan) {
    return std::any_of(scan.segments.begin(), scan.segments.end(),
        [](const utils::Segment& segment) { return segment.kind == utils::SegmentKind::Code; });
}

} // namespace

std::string FormattingSpacingRules::apply(std::string_view line, const utils::LineScan& scan) {
    // The sentinel bytes never occur in valid mType source; if one does, leave
    // the line alone rather than risk restoring the wrong text.
    if (!hasCode(scan) || line.find(kMaskByte) != std::string_view::npos
        || line.find('\x01') != std::string_view::npos) {
        return std::string(line);
    }
    return restoreNonCode(spaceCode(maskNonCode(line, scan)), line, scan);
}

std::string FormattingSpacingRules::maskNonCode(std::string_view line, const utils::LineScan& scan) {
    std::string masked;
    masked.reserve(line.size());
    for (const auto& segment : scan.segments) {
        if (segment.kind == utils::SegmentKind::Code) {
            masked.append(line.substr(segment.start, segment.length));
        } else {
            masked += kMaskByte;
        }
    }
    return masked;
}

std::string FormattingSpacingRules::spaceCode(std::string text) {
    // Compiled once; lookaheads leave the next character unconsumed, so
    // `a=b=c` and `a,,b` are fully spaced in a single pass.
    static const std::regex assignment(R"((\w)=(?=\w))");
    static const std::regex comma(R"(,(?=\S))");
    static const std::regex scopeOperator("::");
    static const std::regex scopeMask(kScopeMask);
    static const std::regex colon(R"(:(?=\w))");
    static const std::regex spaceAfterScope(R"(::[ \t]+)");
    static const std::regex keyword(kSpacedKeywordsPattern);

    // Skip a rule when its trigger character is absent: std::regex is slow,
    // particularly in Debug builds.
    if (text.find('=') != std::string::npos) {
        text = std::regex_replace(text, assignment, "$1 = ");
    }
    if (text.find(',') != std::string::npos) {
        text = std::regex_replace(text, comma, ", ");
    }
    if (text.find(':') != std::string::npos) {
        // Space after a type-annotation colon, but never inside "::", and
        // drop stray whitespace after "::" (`Audio:: play` -> `Audio::play`).
        text = std::regex_replace(text, scopeOperator, kScopeMask);
        text = std::regex_replace(text, colon, ": ");
        text = std::regex_replace(text, scopeMask, "::");
        text = std::regex_replace(text, spaceAfterScope, "::");
    }
    if (text.find_first_of("({") != std::string::npos) {
        text = std::regex_replace(text, keyword, "$1 ");
    }
    return text;
}

std::string FormattingSpacingRules::restoreNonCode(const std::string& spaced, std::string_view line,
                                                   const utils::LineScan& scan) {
    std::vector<std::string_view> spans;
    for (const auto& segment : scan.segments) {
        if (segment.kind != utils::SegmentKind::Code) {
            spans.push_back(line.substr(segment.start, segment.length));
        }
    }

    std::string restored;
    restored.reserve(spaced.size() + line.size());
    std::size_t next = 0;
    for (char c : spaced) {
        if (c != kMaskByte) {
            restored += c;
        } else if (next < spans.size()) {
            restored.append(spans[next++]);
        } else {
            return std::string(line);  // rules disturbed a mask: keep the line as written
        }
    }
    return next == spans.size() ? restored : std::string(line);
}

} // namespace mtype::lsp

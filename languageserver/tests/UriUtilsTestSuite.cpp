#include "UriUtilsTestSuite.hpp"
#include "../src/utils/UriUtils.hpp"

namespace mtype::lsp::test {

void UriUtilsTestSuite::registerTests(LspTestHarness& harness) {

    harness.addTest("urlDecode: %20 becomes space", []() {
        require(UriUtils::urlDecode("hello%20world") == "hello world",
            "expected 'hello world'");
    });

    harness.addTest("urlDecode: %3A becomes colon", []() {
        require(UriUtils::urlDecode("C%3A/foo") == "C:/foo",
            "expected 'C:/foo'");
    });

    harness.addTest("urlDecode: + becomes space", []() {
        require(UriUtils::urlDecode("hello+world") == "hello world",
            "expected 'hello world'");
    });

    harness.addTest("urlDecode: passthrough for normal characters", []() {
        require(UriUtils::urlDecode("normal") == "normal",
            "expected passthrough");
    });

    harness.addTest("uriToFilePath: Windows drive letter", []() {
        std::string result = UriUtils::uriToFilePath("file:///C:/Users/foo/bar.mt");
        require(result == "C:/Users/foo/bar.mt",
            "expected 'C:/Users/foo/bar.mt', got '" + result + "'");
    });

    harness.addTest("uriToFilePath: percent-encoded path", []() {
        std::string result = UriUtils::uriToFilePath("file:///C:/my%20project/test.mt");
        require(result == "C:/my project/test.mt",
            "expected decoded path, got '" + result + "'");
    });

    harness.addTest("filePathToUri: Windows path", []() {
        std::string result = UriUtils::filePathToUri("C:/Users/foo/bar.mt");
        require(result == "file:///C:/Users/foo/bar.mt",
            "expected 'file:///C:/Users/foo/bar.mt', got '" + result + "'");
    });

    harness.addTest("filePathToUri: encodes space and special chars", []() {
        std::string result = UriUtils::filePathToUri("C:/my project/test#1.mt");
        require(result.find("%20") != std::string::npos, "space should be encoded");
        require(result.find("%23") != std::string::npos, "# should be encoded");
    });

    harness.addTest("filePathToUri: normalizes backslashes", []() {
        std::string result = UriUtils::filePathToUri("C:\\Users\\foo\\bar.mt");
        require(result.find('\\') == std::string::npos, "backslashes should be normalized");
        require(result == "file:///C:/Users/foo/bar.mt",
            "expected normalized URI, got '" + result + "'");
    });

    harness.addTest("filePathToUri: empty path returns empty", []() {
        require(UriUtils::filePathToUri("").empty(), "empty path should return empty URI");
    });
}

} // namespace mtype::lsp::test

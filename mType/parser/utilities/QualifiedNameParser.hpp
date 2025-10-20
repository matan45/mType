#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace parser
{
    /// @brief Specialized utility for parsing and building qualified names (e.g., Class::method, namespace::Class)
    /// Handles qualified identifier chains and name construction
    /// Follows Single Responsibility Principle - only handles qualified name operations
    class QualifiedNameParser
    {
    public:
        /// @brief Efficiently build qualified names from parts (e.g., "Class::method")
        /// Optimized O(n) string concatenation instead of O(n²)
        /// @param parts Vector of name parts to join with "::"
        /// @return Qualified name string, empty if parts is empty
        [[nodiscard]] static std::string buildQualifiedName(const std::vector<std::string>& parts) noexcept;

        /// @brief Parse qualified identifier chain (e.g., namespace::Class::method)
        /// Eliminates code duplication in qualified name parsing
        /// @param stream Token stream to parse from
        /// @param initialName First part of qualified name
        /// @return Complete qualified name parts vector
        [[nodiscard]] static std::vector<std::string> parseQualifiedIdentifierChain(
            class TokenStream& stream,
            std::string_view initialName);

    private:
        // Utility class - no instances allowed
        QualifiedNameParser() = delete;
        ~QualifiedNameParser() = delete;
        QualifiedNameParser(const QualifiedNameParser&) = delete;
        QualifiedNameParser& operator=(const QualifiedNameParser&) = delete;
    };
}

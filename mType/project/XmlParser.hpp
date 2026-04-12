#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace project
{
    struct XmlElement
    {
        std::string tagName;
        std::unordered_map<std::string, std::string> attributes;
        std::string content;
        std::vector<XmlElement> children;
    };

    class XmlParser
    {
    public:
        XmlParser() = default;

        XmlElement parse(const std::string& xml);

    private:
        void skipWhitespace(const std::string& xml, size_t& pos);

        std::string parseTagName(const std::string& xml, size_t& pos);

        std::unordered_map<std::string, std::string> parseAttributes(const std::string& xml, size_t& pos);

        std::vector<XmlElement> parseChildren(const std::string& xml, size_t& pos, const std::string& parentTag);
    };
}

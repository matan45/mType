#include "XmlParser.hpp"
#include <stdexcept>

namespace project
{
    void XmlParser::skipWhitespace(const std::string& xml, size_t& pos)
    {
        while (pos < xml.size() && std::isspace(static_cast<unsigned char>(xml[pos])))
        {
            ++pos;
        }
    }

    std::string XmlParser::parseTagName(const std::string& xml, size_t& pos)
    {
        std::string name;
        while (pos < xml.size() && !std::isspace(static_cast<unsigned char>(xml[pos])) &&
               xml[pos] != '>' && xml[pos] != '/')
        {
            name += xml[pos++];
        }
        return name;
    }

    std::unordered_map<std::string, std::string> XmlParser::parseAttributes(const std::string& xml,
                                                                             size_t& pos)
    {
        std::unordered_map<std::string, std::string> attrs;

        while (pos < xml.size())
        {
            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] == '>' || xml[pos] == '/')
            {
                break;
            }

            std::string attrName;
            while (pos < xml.size() && xml[pos] != '=' && !std::isspace(static_cast<unsigned char>(xml[pos])))
            {
                attrName += xml[pos++];
            }

            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] != '=')
            {
                continue;
            }
            ++pos;

            skipWhitespace(xml, pos);

            if (pos >= xml.size() || xml[pos] != '"')
            {
                continue;
            }
            ++pos;

            std::string attrValue;
            while (pos < xml.size() && xml[pos] != '"')
            {
                attrValue += xml[pos++];
            }

            if (pos < xml.size())
            {
                ++pos;
            }

            attrs[attrName] = attrValue;
        }

        return attrs;
    }

    XmlElement XmlParser::parse(const std::string& xml)
    {
        size_t pos = 0;
        skipWhitespace(xml, pos);

        if (xml.substr(pos, 5) == "<?xml")
        {
            pos = xml.find("?>", pos);
            if (pos != std::string::npos)
            {
                pos += 2;
            }
            skipWhitespace(xml, pos);
        }

        if (pos >= xml.size() || xml[pos] != '<')
        {
            throw std::runtime_error("Invalid XML: expected '<'");
        }
        ++pos;

        XmlElement element;
        element.tagName = parseTagName(xml, pos);
        element.attributes = parseAttributes(xml, pos);

        skipWhitespace(xml, pos);

        if (pos < xml.size() && xml[pos] == '/')
        {
            ++pos;
            if (pos < xml.size() && xml[pos] == '>')
            {
                ++pos;
            }
            return element;
        }

        if (pos < xml.size() && xml[pos] == '>')
        {
            ++pos;
        }

        element.children = parseChildren(xml, pos, element.tagName);

        return element;
    }

    std::vector<XmlElement> XmlParser::parseChildren(const std::string& xml,
                                                      size_t& pos,
                                                      const std::string& parentTag)
    {
        std::vector<XmlElement> children;

        while (pos < xml.size())
        {
            if (xml[pos] == '<')
            {
                if (pos + 1 < xml.size() && xml[pos + 1] == '/')
                {
                    pos += 2;
                    std::string closingTag = parseTagName(xml, pos);

                    while (pos < xml.size() && xml[pos] != '>')
                    {
                        ++pos;
                    }
                    if (pos < xml.size())
                    {
                        ++pos;
                    }

                    break;
                }

                ++pos;
                XmlElement child;
                child.tagName = parseTagName(xml, pos);
                child.attributes = parseAttributes(xml, pos);

                skipWhitespace(xml, pos);

                if (pos < xml.size() && xml[pos] == '/')
                {
                    ++pos;
                    if (pos < xml.size() && xml[pos] == '>')
                    {
                        ++pos;
                    }
                    children.push_back(child);
                }
                else if (pos < xml.size() && xml[pos] == '>')
                {
                    ++pos;

                    size_t contentStart = pos;
                    size_t depth = 1;
                    size_t childContentEnd = pos;

                    while (pos < xml.size() && depth > 0)
                    {
                        if (xml[pos] == '<')
                        {
                            if (pos + 1 < xml.size() && xml[pos + 1] == '/')
                            {
                                --depth;
                                if (depth == 0)
                                {
                                    childContentEnd = pos;
                                }
                            }
                            else if (pos + 1 < xml.size() && xml[pos + 1] != '!')
                            {
                                ++depth;
                            }
                        }
                        ++pos;
                    }

                    std::string innerContent = xml.substr(contentStart, childContentEnd - contentStart);

                    size_t innerPos = 0;
                    bool hasChildElements = false;
                    while (innerPos < innerContent.size())
                    {
                        if (innerContent[innerPos] == '<' && innerPos + 1 < innerContent.size() &&
                            innerContent[innerPos + 1] != '/')
                        {
                            hasChildElements = true;
                            break;
                        }
                        ++innerPos;
                    }

                    if (hasChildElements)
                    {
                        size_t parsePos = contentStart;
                        child.children = parseChildren(xml, parsePos, child.tagName);
                    }
                    else
                    {
                        size_t start = 0;
                        size_t end = innerContent.size();
                        while (start < end && std::isspace(static_cast<unsigned char>(innerContent[start])))
                        {
                            ++start;
                        }
                        while (end > start && std::isspace(static_cast<unsigned char>(innerContent[end - 1])))
                        {
                            --end;
                        }
                        child.content = innerContent.substr(start, end - start);
                    }

                    while (pos < xml.size() && xml[pos] != '>')
                    {
                        ++pos;
                    }
                    if (pos < xml.size())
                    {
                        ++pos;
                    }

                    children.push_back(child);
                }
            }
            else
            {
                ++pos;
            }
        }

        return children;
    }
}

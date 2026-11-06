#include "SourceSegmenter.hpp"

namespace mtype::lsp::utils
{
    namespace
    {
        bool startsWith(std::string_view line, std::size_t i, char first, char second)
        {
            return i + 1 < line.size() && line[i] == first && line[i + 1] == second;
        }

        bool isBlank(char c)
        {
            return c == ' ' || c == '\t' || c == '\r' || c == '\f' || c == '\v';
        }

        void appendSegment(std::vector<Segment>& segments, SegmentKind kind,
                           std::size_t start, std::size_t length)
        {
            if (!segments.empty() && segments.back().kind == kind)
            {
                segments.back().length += length;
                return;
            }
            segments.push_back(Segment{kind, start, length});
        }
    }

    LineScan SourceSegmenter::scanLine(std::string_view line)
    {
        LineScan scan;
        scan.startsIn = region();

        std::size_t i = 0;
        while (i < line.size())
        {
            const std::size_t start = i;
            SegmentKind kind = SegmentKind::Code;
            if (stack_.empty())
            {
                i = stepCode(line, i, kind);
            }
            else
            {
                // A character's kind is decided before the step, so a closing
                // quote or `*/` belongs to the literal or comment it ends.
                kind = insideKind();
                i = stepInside(line, i);
            }
            appendSegment(scan.segments, kind, start, i - start);
        }

        scan.endsIn = region();
        return scan;
    }

    Region SourceSegmenter::region() const
    {
        if (stack_.empty())
        {
            return Region::Code;
        }
        return stack_.front().frame == Frame::BlockComment ? Region::BlockComment : Region::String;
    }

    SegmentKind SourceSegmenter::insideKind() const
    {
        return region() == Region::BlockComment ? SegmentKind::Comment : SegmentKind::String;
    }

    std::size_t SourceSegmenter::stepCode(std::string_view line, std::size_t i, SegmentKind& kind)
    {
        if (startsWith(line, i, '/', '/'))
        {
            kind = SegmentKind::Comment;
            return line.size();
        }
        if (startsWith(line, i, '/', '*'))
        {
            kind = SegmentKind::Comment;
            stack_.push_back(Entry{Frame::BlockComment});
            return i + 2;
        }
        if (startsWith(line, i, '$', '"'))
        {
            kind = SegmentKind::String;
            stack_.push_back(Entry{Frame::InterpText});
            return i + 2;
        }
        if (line[i] == '"')
        {
            kind = SegmentKind::String;
            stack_.push_back(Entry{Frame::String});
            return i + 1;
        }
        kind = SegmentKind::Code;
        return i + 1;
    }

    std::size_t SourceSegmenter::stepInside(std::string_view line, std::size_t i)
    {
        switch (stack_.back().frame)
        {
        case Frame::BlockComment:
            if (startsWith(line, i, '*', '/'))
            {
                stack_.pop_back();
                return i + 2;
            }
            return i + 1;
        case Frame::String:
        case Frame::InterpText:
            return stepString(line, i);
        case Frame::InterpExpr:
            return stepHole(line, i);
        }
        return i + 1;
    }

    std::size_t SourceSegmenter::stepString(std::string_view line, std::size_t i)
    {
        const char c = line[i];
        if (c == '\\')
        {
            // A backslash at the end of a line escapes the line break; the
            // literal continues on the next line either way.
            return i + 2 < line.size() ? i + 2 : line.size();
        }
        if (c == '"')
        {
            stack_.pop_back();
        }
        else if (c == '{' && stack_.back().frame == Frame::InterpText)
        {
            stack_.push_back(Entry{Frame::InterpExpr});
        }
        return i + 1;
    }

    std::size_t SourceSegmenter::stepHole(std::string_view line, std::size_t i)
    {
        // Inside `{...}` of an interpolated string the lexer reads normal code,
        // so nested literals and comments must be tracked to find the real end.
        if (startsWith(line, i, '/', '/'))
        {
            return line.size();
        }
        if (startsWith(line, i, '/', '*'))
        {
            stack_.push_back(Entry{Frame::BlockComment});
            return i + 2;
        }
        if (startsWith(line, i, '$', '"'))
        {
            stack_.push_back(Entry{Frame::InterpText});
            return i + 2;
        }

        Entry& hole = stack_.back();
        const char c = line[i];
        if (c == '"')
        {
            stack_.push_back(Entry{Frame::String});
        }
        else if (c == '{')
        {
            ++hole.braceDepth;
        }
        else if (c == '}')
        {
            if (hole.braceDepth == 0)
            {
                stack_.pop_back();
            }
            else
            {
                --hole.braceDepth;
            }
        }
        return i + 1;
    }

    char firstCodeChar(std::string_view line, const LineScan& scan)
    {
        for (const Segment& segment : scan.segments)
        {
            if (segment.kind != SegmentKind::Code)
            {
                continue;
            }
            for (std::size_t i = segment.start; i < segment.start + segment.length; ++i)
            {
                if (!isBlank(line[i]))
                {
                    return line[i];
                }
            }
        }
        return '\0';
    }

    char lastCodeChar(std::string_view line, const LineScan& scan)
    {
        for (auto it = scan.segments.rbegin(); it != scan.segments.rend(); ++it)
        {
            if (it->kind != SegmentKind::Code)
            {
                continue;
            }
            for (std::size_t i = it->start + it->length; i > it->start; --i)
            {
                if (!isBlank(line[i - 1]))
                {
                    return line[i - 1];
                }
            }
        }
        return '\0';
    }
}

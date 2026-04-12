#include "SemVer.hpp"
#include <sstream>
#include <algorithm>

namespace packagemanager
{
    SemVer SemVer::parse(const std::string& str)
    {
        SemVer ver;
        size_t pos = 0;
        size_t dotPos;

        dotPos = str.find('.', pos);
        if (dotPos == std::string::npos)
        {
            throw std::runtime_error("Invalid semver: " + str);
        }
        ver.major = std::stoi(str.substr(pos, dotPos - pos));
        pos = dotPos + 1;

        dotPos = str.find('.', pos);
        if (dotPos == std::string::npos)
        {
            throw std::runtime_error("Invalid semver: " + str);
        }
        ver.minor = std::stoi(str.substr(pos, dotPos - pos));
        pos = dotPos + 1;

        ver.patch = std::stoi(str.substr(pos));

        return ver;
    }

    std::string SemVer::toString() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    bool SemVer::operator==(const SemVer& other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    bool SemVer::operator!=(const SemVer& other) const
    {
        return !(*this == other);
    }

    bool SemVer::operator<(const SemVer& other) const
    {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    bool SemVer::operator<=(const SemVer& other) const
    {
        return *this == other || *this < other;
    }

    bool SemVer::operator>(const SemVer& other) const
    {
        return other < *this;
    }

    bool SemVer::operator>=(const SemVer& other) const
    {
        return *this == other || *this > other;
    }

    SemVerRange SemVerRange::parse(const std::string& rangeStr)
    {
        SemVerRange range;
        std::string str = rangeStr;

        // Trim whitespace
        while (!str.empty() && str[0] == ' ') str.erase(0, 1);
        while (!str.empty() && str.back() == ' ') str.pop_back();

        if (str.empty())
        {
            throw std::runtime_error("Empty version range");
        }

        // ^1.2.3 — compatible (same major)
        if (str[0] == '^')
        {
            SemVer base = SemVer::parse(str.substr(1));
            range.minVersion = base;
            range.minInclusive = true;
            range.maxVersion = SemVer(base.major + 1, 0, 0);
            range.maxInclusive = false;
            return range;
        }

        // ~1.2.3 — patch-level (same minor)
        if (str[0] == '~')
        {
            SemVer base = SemVer::parse(str.substr(1));
            range.minVersion = base;
            range.minInclusive = true;
            range.maxVersion = SemVer(base.major, base.minor + 1, 0);
            range.maxInclusive = false;
            return range;
        }

        // >=1.0.0 <2.0.0 — explicit range
        if (str.substr(0, 2) == ">=" || str.substr(0, 1) == ">")
        {
            bool minInc = (str[1] == '=');
            size_t start = minInc ? 2 : 1;

            // Find the space separator
            size_t spacePos = str.find(' ', start);
            if (spacePos == std::string::npos)
            {
                // Just a lower bound: >=1.0.0
                range.minVersion = SemVer::parse(str.substr(start));
                range.minInclusive = minInc;
                range.maxVersion = SemVer(999999, 999999, 999999);
                range.maxInclusive = true;
                return range;
            }

            range.minVersion = SemVer::parse(str.substr(start, spacePos - start));
            range.minInclusive = minInc;

            std::string upper = str.substr(spacePos + 1);
            while (!upper.empty() && upper[0] == ' ') upper.erase(0, 1);

            if (upper.substr(0, 2) == "<=")
            {
                range.maxVersion = SemVer::parse(upper.substr(2));
                range.maxInclusive = true;
            }
            else if (upper[0] == '<')
            {
                range.maxVersion = SemVer::parse(upper.substr(1));
                range.maxInclusive = false;
            }
            else
            {
                throw std::runtime_error("Invalid range upper bound: " + upper);
            }

            return range;
        }

        // Exact version: 1.2.3
        SemVer exact = SemVer::parse(str);
        range.minVersion = exact;
        range.maxVersion = exact;
        range.minInclusive = true;
        range.maxInclusive = true;
        return range;
    }

    bool SemVerRange::satisfiedBy(const SemVer& version) const
    {
        bool aboveMin = minInclusive ? (version >= minVersion) : (version > minVersion);
        bool belowMax = maxInclusive ? (version <= maxVersion) : (version < maxVersion);
        return aboveMin && belowMax;
    }

    SemVer findBestMatch(const std::vector<SemVer>& versions, const SemVerRange& range)
    {
        SemVer best;
        bool found = false;

        for (const auto& ver : versions)
        {
            if (range.satisfiedBy(ver))
            {
                if (!found || ver > best)
                {
                    best = ver;
                    found = true;
                }
            }
        }

        if (!found)
        {
            throw std::runtime_error("No version satisfies range: " +
                range.minVersion.toString() + " - " + range.maxVersion.toString());
        }

        return best;
    }
}

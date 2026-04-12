#pragma once
#include <string>
#include <vector>
#include <stdexcept>

namespace packagemanager
{
    struct SemVer
    {
        int major = 0;
        int minor = 0;
        int patch = 0;

        SemVer() = default;
        SemVer(int maj, int min, int pat) : major(maj), minor(min), patch(pat) {}

        static SemVer parse(const std::string& str);

        std::string toString() const;

        bool operator==(const SemVer& other) const;
        bool operator!=(const SemVer& other) const;
        bool operator<(const SemVer& other) const;
        bool operator<=(const SemVer& other) const;
        bool operator>(const SemVer& other) const;
        bool operator>=(const SemVer& other) const;
    };

    struct SemVerRange
    {
        SemVer minVersion;
        SemVer maxVersion;
        bool minInclusive = true;
        bool maxInclusive = false;

        static SemVerRange parse(const std::string& rangeStr);

        bool satisfiedBy(const SemVer& version) const;
    };

    // Find the highest version in a list that satisfies the range
    SemVer findBestMatch(const std::vector<SemVer>& versions, const SemVerRange& range);
}

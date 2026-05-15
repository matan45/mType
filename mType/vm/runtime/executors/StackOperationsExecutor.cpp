// MYT-319: all handler bodies live in the header so MSVC v145 can inline
// them through the dispatch switch. This translation unit is intentionally
// empty — the header carries the full implementation. The .cpp is kept so
// premake5.lua's wildcard pickup remains stable.
#include "StackOperationsExecutor.hpp"

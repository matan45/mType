#include "ErrorCodeRegistry.hpp"

namespace diagnostics::codes
{
#define MTYPE_ERROR_CODES_DEFINE(name, id, sev, title) \
    const ::diagnostics::ErrorCode name = { id, sev, title };
    MTYPE_ERROR_CODES(MTYPE_ERROR_CODES_DEFINE)
#undef MTYPE_ERROR_CODES_DEFINE
}

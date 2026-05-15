// MYT-319: handleLambda body lives in the header. See LambdaExecutor.hpp.
// MYT-B1: handleLambdaInvoke was removed — the LAMBDA_INVOKE opcode was
// never emitted by the compiler. Lambda invocation flows through
// CALL_METHOD with the lambda value as the receiver, routed through
// ObjectExecutor.
#include "LambdaExecutor.hpp"

#include "DebuggerTestSuite.hpp"

#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugExpressionEvaluator.hpp"
#include "../../debugger/DebugProtocol.hpp"
#include "../../debugger/VMVariableInspector.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../services/ScriptInterpreter.hpp"
#include "../../value/ValueType.hpp"
#include "../../value/ValueShim.hpp"

#include <optional>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;

    namespace
    {
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }

        std::string trimTrailingNewline(std::string text)
        {
            while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            {
                text.pop_back();
            }
            return text;
        }

        void resetDebugContext()
        {
            debugger::DebugContext::shutdown();
            debugger::DebugContext::initialize();
            auto& ctx = debugger::DebugContext::getInstance();
            ctx.clearAllBreakpoints();
            ctx.clearExceptionBreakpoints();
            ctx.setEventCallback(nullptr);
            ctx.setConditionEvaluator(nullptr);
            ctx.continueExecution();
        }

        debugger::DebugExpressionEvaluator::Resolver mapResolver(
            const std::unordered_map<std::string, value::Value>& values)
        {
            return [&values](const std::string& name) -> std::optional<value::Value> {
                auto it = values.find(name);
                if (it == values.end())
                {
                    return std::nullopt;
                }
                return it->second;
            };
        }

        bool hasVariableNamed(const std::vector<debugger::DebugVariable>& variables,
                              const std::string& name)
        {
            for (const auto& variable : variables)
            {
                if (variable.name == name)
                {
                    return true;
                }
            }
            return false;
        }
    }

    void DebuggerTestSuite::setupTests()
    {
        addCallbackTest("protocol: quoted values round-trip control characters", "",
            [](ScriptAPI&) {
                debugger::DebugProtocol::Message msg("OUTPUT");
                const std::string text = "alpha:beta=gamma \"quoted\"\nnext\tcolumn\\tail";
                msg.addParameter("text", text);
                msg.addParameter("category", "stdout");

                auto parsed = debugger::DebugProtocol::parse(msg.serialize());
                require(parsed.command == "OUTPUT", "expected OUTPUT command");
                require(parsed.getParameter("text") == text,
                    "escaped protocol text did not round-trip");
                require(parsed.getParameter("category") == "stdout",
                    "category did not round-trip");
            });

        addCallbackTest("protocol: variables include structured fields", "",
            [](ScriptAPI&) {
                std::ostringstream out;
                debugger::DebugProtocol::setProtocolStream(&out);

                std::vector<std::tuple<std::string, std::string, std::string, int64_t>> vars;
                vars.emplace_back("weird:name", "a=b:c \"q\"\nline", "String:Like", 1234);
                debugger::DebugProtocol::sendVariables(vars);
                debugger::DebugProtocol::setProtocolStream(nullptr);

                const std::string line = trimTrailingNewline(out.str());
                require(line.find('\n') == std::string::npos,
                    "protocol payload must not contain raw embedded newlines");

                auto parsed = debugger::DebugProtocol::parse(line);
                require(parsed.command == "VARIABLES", "expected VARIABLES command");
                require(parsed.getParameter("count") == "1", "expected count=1");
                require(parsed.getParameter("var0_name") == "weird:name",
                    "structured variable name lost punctuation");
                require(parsed.getParameter("var0_value") == "a=b:c \"q\"\nline",
                    "structured variable value lost punctuation");
                require(parsed.getParameter("var0_type") == "String:Like",
                    "structured variable type lost punctuation");
                require(parsed.getParameter("var0_ref") == "1234",
                    "structured variable ref mismatch");
            });

        addCallbackTest("protocol: exception stopped event carries message", "",
            [](ScriptAPI&) {
                std::ostringstream out;
                debugger::DebugProtocol::setProtocolStream(&out);
                debugger::DebugProtocol::sendStoppedEvent(
                    "exception",
                    errors::SourceLocation("file.mt", 7, 3),
                    "MyError: bad\nmessage");
                debugger::DebugProtocol::setProtocolStream(nullptr);

                auto parsed = debugger::DebugProtocol::parse(trimTrailingNewline(out.str()));
                require(parsed.command == "STOPPED", "expected STOPPED command");
                require(parsed.getParameter("reason") == "exception",
                    "expected exception reason");
                require(parsed.getParameter("message") == "MyError: bad\nmessage",
                    "exception message did not round-trip");
            });

        addCallbackTest("debug expression: evaluates read-only operators", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("x", value::Value(7));
                values.emplace("y", value::Value(2));
                values.emplace("flag", value::Value(true));

                auto result = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "x * 3 + y == 23 && flag",
                    mapResolver(values));

                require(result.success, "expression should evaluate successfully: " + result.error);
                require(value::isBool(result.value), "expression result should be Bool");
                require(value::asBool(result.value), "expression result should be true");
            });

        addCallbackTest("debug expression: resolves static-style names", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("Thing::count", value::Value(5));
                values.emplace("offset", value::Value(4));

                auto result = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "Thing::count + offset",
                    mapResolver(values));

                require(result.success, "static expression should evaluate successfully: " + result.error);
                require(value::isInt(result.value), "static expression result should be Int");
                require(value::asInt(result.value) == 9, "static expression result mismatch");
            });

        addCallbackTest("debug expression: compares large integers exactly", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("a", value::Value(INT64_C(9007199254740993)));
                values.emplace("b", value::Value(INT64_C(9007199254740994)));

                auto equal = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "a == b",
                    mapResolver(values));
                require(equal.success, "large int equality should evaluate: " + equal.error);
                require(value::isBool(equal.value), "large int equality should produce Bool");
                require(!value::asBool(equal.value), "distinct large ints should not compare equal");

                auto less = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "a < b",
                    mapResolver(values));
                require(less.success, "large int comparison should evaluate: " + less.error);
                require(value::isBool(less.value), "large int comparison should produce Bool");
                require(value::asBool(less.value), "large int comparison should preserve ordering");
            });

        addCallbackTest("debug expression: rejects integer division overflow", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("min", value::Value(INT64_MIN));
                values.emplace("minusOne", value::Value(INT64_C(-1)));

                auto result = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "min / minusOne",
                    mapResolver(values));
                require(!result.success, "INT64_MIN / -1 should be rejected");
                require(result.error.find("Integer overflow in division") != std::string::npos,
                    "division overflow rejection should be clear");
            });

        addCallbackTest("debug expression: rejects calls and assignments", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("x", value::Value(1));

                auto assignment = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "x = 2",
                    mapResolver(values));
                require(!assignment.success, "assignment should be rejected");
                require(assignment.error.find("Assignments") != std::string::npos,
                    "assignment rejection should be clear");

                auto call = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "x.toString()",
                    mapResolver(values));
                require(!call.success, "method call should be rejected");
                require(call.error.find("calls are not supported") != std::string::npos,
                    "call rejection should be clear");
            });

        addCallbackTest("context: breakpoint pauses and emits event", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                bool eventSeen = false;
                ctx.setEventCallback([&](const debugger::DebugEvent& event) {
                    if (event.type == debugger::DebugEvent::Type::BREAKPOINT_HIT &&
                        event.location.getLine() == 10)
                    {
                        eventSeen = true;
                    }
                });

                ctx.addBreakpoint("debug_test.mt", 10);
                const bool paused = ctx.shouldPauseAt(
                    errors::SourceLocation("debug_test.mt", 10, 1));

                require(paused, "expected breakpoint to pause");
                require(ctx.isPaused(), "context should be paused after breakpoint");
                require(eventSeen, "breakpoint event was not emitted");

                ctx.resume();
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: conditional breakpoint skips when false", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.setConditionEvaluator([](const std::string& condition) {
                    return condition == "trueCondition";
                });

                ctx.addBreakpoint("debug_condition.mt", 4, "falseCondition");
                const bool paused = ctx.shouldPauseAt(
                    errors::SourceLocation("debug_condition.mt", 4, 1));

                require(!paused, "false conditional breakpoint should not pause");
                require(!ctx.isPaused(), "context should keep running");
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: step out waits for shallower frame", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.pushCallFrame("outer", errors::SourceLocation("step.mt", 1, 1));
                ctx.pushCallFrame("inner", errors::SourceLocation("step.mt", 2, 1));

                ctx.stepOut();
                bool paused = ctx.shouldPauseAt(errors::SourceLocation("step.mt", 3, 1));
                require(!paused, "step out should not pause at same depth");

                ctx.popCallFrame();
                paused = ctx.shouldPauseAt(errors::SourceLocation("step.mt", 4, 1));
                require(paused, "step out should pause after returning to shallower depth");

                ctx.resume();
                ctx.popCallFrame();
                debugger::DebugContext::shutdown();
            });

        // VK-1378: end-to-end regression. A method invoked through the interop
        // path (ScriptInterpreter::callMethod -> vm->invokeMethod) must honour
        // breakpoints. Before the fix the invokeMethod mini-loop bypassed the
        // debug hook that lived only in interpretLoop, so engine-invoked
        // lifecycle methods (onUpdate etc.) never paused.
        const std::string interopFixture =
            "mType/tests/testFiles/debugger/interopBreakpoint.mt";

        addInterpreterCallbackTest(
            "interop: breakpoint pauses inside invokeMethod (VK-1378)",
            interopFixture,
            [interopFixture](services::ScriptInterpreter& interp) {
                // Debugging is an interpreter-mode feature; JIT and the debugger
                // are mutually exclusive. The --tests runner arms JIT for every
                // suite, so force it off here for a deterministic interpreter run.
                interp.getVM()->setJitEnabled(false);

                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                // line 15 is `total = total + delta;` inside step().
                ctx.addBreakpoint(interopFixture, 15);

                bool hit = false;
                ctx.setEventCallback([&](const debugger::DebugEvent& event) {
                    if (event.type == debugger::DebugEvent::Type::BREAKPOINT_HIT &&
                        event.location.getLine() == 15)
                    {
                        hit = true;
                    }
                    // Release the paused VM thread immediately so the test
                    // can't deadlock on waitForResume(); also models the
                    // debug client's CONTINUE.
                    ctx.continueExecution();
                });

                // Arm the executing VM exactly as ScriptDebugServer does.
                interp.enableDebugging();

                value::Value target = interp.createObject("InteropBreakTarget");
                require(value::isObject(target),
                    "fixture object should construct");

                // Routes through vm->invokeMethod — the path that lacked the hook.
                std::vector<value::Value> args{ value::Value(int64_t{5}) };
                value::Value result = interp.callMethod(target, "step", args);

                ctx.setEventCallback(nullptr);
                require(hit,
                    "breakpoint inside invokeMethod did not pause (VK-1378 regression)");
                require(value::isInt(result) && value::asInt(result) == 5,
                    "step() should still return its value after the debug pause");

                debugger::DebugContext::shutdown();
            });

        const std::string methodLocalsFixture =
            "mType/tests/testFiles/debugger/methodFrameLocalNames.mt";

        addInterpreterCallbackTest(
            "interop: method frame locals use source names (MYT-380)",
            methodLocalsFixture,
            [methodLocalsFixture](services::ScriptInterpreter& interp) {
                interp.getVM()->setJitEnabled(false);

                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                // line 15 is `return mag;`, after dx/dy/mag have all been initialized.
                ctx.addBreakpoint(methodLocalsFixture, 15);

                bool hit = false;
                std::vector<debugger::DebugVariable> localsAtPause;
                ctx.setEventCallback([&](const debugger::DebugEvent& event) {
                    if (event.type == debugger::DebugEvent::Type::BREAKPOINT_HIT &&
                        event.location.getLine() == 15)
                    {
                        hit = true;
                        debugger::VMVariableInspector inspector;
                        localsAtPause = inspector.getLocalVariables(interp.getVM());
                    }
                    ctx.continueExecution();
                });

                interp.enableDebugging();

                value::Value target = interp.createObject("MethodFrameLocalNamesTarget");
                require(value::isObject(target),
                    "fixture object should construct");

                std::vector<value::Value> args{ value::Value(int64_t{5}) };
                value::Value result = interp.callMethod(target, "step", args);

                ctx.setEventCallback(nullptr);
                require(hit, "breakpoint inside method did not pause");
                require(value::isInt(result) && value::asInt(result) == 13,
                    "step() should still return its computed value");

                require(hasVariableNamed(localsAtPause, "this"),
                    "method locals should include this");
                require(hasVariableNamed(localsAtPause, "delta"),
                    "method locals should include parameter delta");
                require(hasVariableNamed(localsAtPause, "dx"),
                    "method locals should include local dx");
                require(hasVariableNamed(localsAtPause, "dy"),
                    "method locals should include local dy");
                require(hasVariableNamed(localsAtPause, "mag"),
                    "method locals should include local mag");
                require(!hasVariableNamed(localsAtPause, "local_2") &&
                        !hasVariableNamed(localsAtPause, "local_3") &&
                        !hasVariableNamed(localsAtPause, "local_4"),
                    "method locals should not fall back to local_N names");

                debugger::DebugContext::shutdown();
            });

        // =============================================
        // PROTOCOL ROBUSTNESS
        // =============================================

        addCallbackTest("protocol: malformed and empty lines do not crash the parser", "",
            [](ScriptAPI&) {
                // The host loop reads lines from a socket/stdin — a garbage or
                // empty line must never take the session down. Either a parse
                // exception or a benign message object is acceptable; a crash
                // is not.
                try { debugger::DebugProtocol::parse("???- not a command =="); }
                catch (const std::exception&) {}
                try { debugger::DebugProtocol::parse(""); }
                catch (const std::exception&) {}
                try { debugger::DebugProtocol::parse("KEYONLY ="); }
                catch (const std::exception&) {}
            });

        addCallbackTest("protocol: SETBREAKPOINT with condition and logMessage round-trips", "",
            [](ScriptAPI&) {
                debugger::DebugProtocol::Message msg("SETBREAKPOINT");
                msg.addParameter("file", "C:\\proj\\src file\\main.mt");
                msg.addParameter("line", "42");
                msg.addParameter("condition", "x > 3 && name == \"hero\"");
                msg.addParameter("logMessage", "hit with {x}\n");

                auto parsed = debugger::DebugProtocol::parse(msg.serialize());
                require(parsed.command == "SETBREAKPOINT", "command mismatch");
                require(parsed.getParameter("file") == "C:\\proj\\src file\\main.mt",
                    "windows path with spaces did not round-trip");
                require(parsed.getParameter("line") == "42", "line mismatch");
                require(parsed.getParameter("condition") == "x > 3 && name == \"hero\"",
                    "condition with quotes did not round-trip");
                require(parsed.getParameter("logMessage") == "hit with {x}\n",
                    "logMessage with newline did not round-trip");
            });

        // =============================================
        // BREAKPOINT MANAGEMENT
        // =============================================

        addCallbackTest("context: removeBreakpoint stops the pause", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                ctx.addBreakpoint("bpmgmt.mt", 5);
                require(ctx.shouldPauseAt(errors::SourceLocation("bpmgmt.mt", 5, 1)),
                    "breakpoint should pause before removal");
                ctx.resume();

                ctx.removeBreakpoint("bpmgmt.mt", 5);
                require(!ctx.hasBreakpoint("bpmgmt.mt", 5),
                    "breakpoint should be gone after removeBreakpoint");
                require(!ctx.shouldPauseAt(errors::SourceLocation("bpmgmt.mt", 5, 1)),
                    "removed breakpoint must not pause");
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: clearBreakpoints(file) clears only that file", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                ctx.addBreakpoint("a.mt", 3);
                ctx.addBreakpoint("b.mt", 3);
                ctx.clearBreakpoints("a.mt");

                require(!ctx.hasBreakpoint("a.mt", 3), "a.mt breakpoint should be cleared");
                require(ctx.hasBreakpoint("b.mt", 3), "b.mt breakpoint must survive");
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: re-adding the same line overwrites condition", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.setConditionEvaluator([](const std::string& condition) {
                    return condition == "trueCondition";
                });

                ctx.addBreakpoint("overwrite.mt", 9);  // unconditional
                ctx.addBreakpoint("overwrite.mt", 9, "falseCondition");

                debugger::BreakpointInfo info;
                require(ctx.getBreakpointInfo("overwrite.mt", 9, info),
                    "breakpoint info should exist");
                require(info.condition == "falseCondition",
                    "second add must overwrite the condition");
                require(!ctx.shouldPauseAt(errors::SourceLocation("overwrite.mt", 9, 1)),
                    "overwritten (now false-conditional) breakpoint must not pause");
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: conditional breakpoint pauses when condition is true", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.setConditionEvaluator([](const std::string& condition) {
                    return condition == "trueCondition";
                });

                ctx.addBreakpoint("cond.mt", 4, "trueCondition");
                require(ctx.shouldPauseAt(errors::SourceLocation("cond.mt", 4, 1)),
                    "true conditional breakpoint should pause");
                ctx.resume();
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: log point stores its message and reports isLogPoint", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                ctx.addBreakpoint("log.mt", 12, "", "value is {x}");

                debugger::BreakpointInfo info;
                require(ctx.getBreakpointInfo("log.mt", 12, info),
                    "log point info should exist");
                require(info.isLogPoint(), "breakpoint with logMessage is a log point");
                require(!info.hasCondition(), "log point without condition");
                require(info.logMessage == "value is {x}", "logMessage stored verbatim");
                debugger::DebugContext::shutdown();
            });

        // =============================================
        // STEPPING
        // =============================================

        addCallbackTest("context: step over skips deeper frames, pauses at same depth", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.pushCallFrame("outer", errors::SourceLocation("stepover.mt", 1, 1));

                ctx.stepOver();
                ctx.pushCallFrame("inner", errors::SourceLocation("stepover.mt", 2, 1));
                require(!ctx.shouldPauseAt(errors::SourceLocation("stepover.mt", 3, 1)),
                    "step over must not pause inside a deeper frame");

                ctx.popCallFrame();
                require(ctx.shouldPauseAt(errors::SourceLocation("stepover.mt", 4, 1)),
                    "step over should pause back at the starting depth");

                ctx.resume();
                ctx.popCallFrame();
                debugger::DebugContext::shutdown();
            });

        addCallbackTest("context: step into pauses at the next location regardless of depth", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();
                ctx.pushCallFrame("outer", errors::SourceLocation("stepinto.mt", 1, 1));

                ctx.stepInto();
                ctx.pushCallFrame("inner", errors::SourceLocation("stepinto.mt", 2, 1));
                require(ctx.shouldPauseAt(errors::SourceLocation("stepinto.mt", 3, 1)),
                    "step into should pause inside the deeper frame");

                ctx.resume();
                ctx.popCallFrame();
                ctx.popCallFrame();
                debugger::DebugContext::shutdown();
            });

        // =============================================
        // EXCEPTION FILTERS
        // =============================================

        addCallbackTest("context: exception filters all vs uncaught", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                ctx.setExceptionBreakpoints({"all"});
                require(ctx.shouldBreakOnException(false),
                    "'all' filter should break on caught exceptions");
                require(ctx.shouldBreakOnException(true),
                    "'all' filter should break on uncaught exceptions");

                ctx.setExceptionBreakpoints({"uncaught"});
                require(!ctx.shouldBreakOnException(false),
                    "'uncaught' filter should ignore caught exceptions");
                require(ctx.shouldBreakOnException(true),
                    "'uncaught' filter should break on uncaught exceptions");

                ctx.clearExceptionBreakpoints();
                require(!ctx.shouldBreakOnException(false) &&
                        !ctx.shouldBreakOnException(true),
                    "cleared filters should never break");
                debugger::DebugContext::shutdown();
            });

        // =============================================
        // CALL STACK DEPTH
        // =============================================

        addCallbackTest("context: 150-frame call stack reports every frame", "",
            [](ScriptAPI&) {
                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                for (int i = 0; i < 150; ++i)
                {
                    ctx.pushCallFrame("fn" + std::to_string(i),
                                      errors::SourceLocation("deep.mt", i + 1, 1));
                }
                auto stack = ctx.getCallStack();
                require(stack.size() == 150,
                    "expected 150 frames, got " + std::to_string(stack.size()));
                require(ctx.getCurrentDepth() == 150, "depth should be 150");
                require(stack.front().functionName == "fn0" ||
                        stack.back().functionName == "fn0",
                    "fn0 should sit at one end of the reported stack");

                for (int i = 0; i < 150; ++i) ctx.popCallFrame();
                require(ctx.getCurrentDepth() == 0, "depth should drain to 0");
                debugger::DebugContext::shutdown();
            });

        // =============================================
        // EXPRESSION EVALUATOR ERROR PATHS
        // =============================================

        addCallbackTest("debug expression: division by zero fails cleanly", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                values.emplace("x", value::Value(INT64_C(10)));
                values.emplace("zero", value::Value(INT64_C(0)));

                auto result = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "x / zero", mapResolver(values));
                require(!result.success, "division by zero must not succeed");
                require(!result.error.empty(), "division by zero needs an error message");
            });

        addCallbackTest("debug expression: unknown variable fails cleanly", "",
            [](ScriptAPI&) {
                std::unordered_map<std::string, value::Value> values;
                auto result = debugger::DebugExpressionEvaluator::evaluateWithResolver(
                    "ghost + 1", mapResolver(values));
                require(!result.success, "unknown variable must not evaluate");
                require(!result.error.empty(), "unknown variable needs an error message");
            });

        // =============================================
        // VARIABLE INSPECTION (EXPANDVARIABLE PATH)
        // =============================================

        const std::string arrayLocalsFixture =
            "mType/tests/testFiles/debugger/arrayLocalsExpansion.mt";

        addInterpreterCallbackTest(
            "inspector: array local expands to its elements",
            arrayLocalsFixture,
            [arrayLocalsFixture](services::ScriptInterpreter& interp) {
                interp.getVM()->setJitEnabled(false);

                resetDebugContext();
                auto& ctx = debugger::DebugContext::getInstance();

                // line 18 is `return sum;` — arr and sum are both live.
                ctx.addBreakpoint(arrayLocalsFixture, 18);

                bool hit = false;
                std::vector<debugger::DebugVariable> locals;
                std::vector<debugger::DebugVariable> children;
                debugger::VMVariableInspector inspector;
                ctx.setEventCallback([&](const debugger::DebugEvent& event) {
                    if (event.type == debugger::DebugEvent::Type::BREAKPOINT_HIT &&
                        event.location.getLine() == 18)
                    {
                        hit = true;
                        locals = inspector.getLocalVariables(interp.getVM());
                        for (const auto& v : locals)
                        {
                            if (v.name == "arr" && v.isExpandable && v.referenceId != 0)
                            {
                                children = inspector.getVariableChildren(
                                    interp.getVM(), v.referenceId);
                            }
                        }
                    }
                    ctx.continueExecution();
                });

                interp.enableDebugging();

                value::Value target = interp.createObject("ArrayLocalsTarget");
                require(value::isObject(target), "fixture object should construct");

                std::vector<value::Value> args{ value::Value(int64_t{5}) };
                value::Value result = interp.callMethod(target, "step", args);

                ctx.setEventCallback(nullptr);
                require(hit, "breakpoint at line 18 did not pause");
                require(value::isInt(result) && value::asInt(result) == 29,
                    "step() should return 7+8+9+5 = 29");

                require(hasVariableNamed(locals, "arr"), "locals should include arr");
                require(hasVariableNamed(locals, "sum"), "locals should include sum");
                require(children.size() == 3,
                    "expanding arr should yield 3 children, got "
                    + std::to_string(children.size()));
                require(children[0].value == "7" && children[1].value == "8" &&
                        children[2].value == "9",
                    "expanded array children should be 7, 8, 9 in order");

                debugger::DebugContext::shutdown();
            });
    }
}

#include "DebuggerTestSuite.hpp"

#include "../../debugger/DebugContext.hpp"
#include "../../debugger/DebugExpressionEvaluator.hpp"
#include "../../debugger/DebugProtocol.hpp"
#include "../../errors/SourceLocation.hpp"
#include "../../value/ValueType.hpp"

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
    }
}

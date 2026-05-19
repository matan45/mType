#include "BytecodeProgram.hpp"
#include <stdexcept>

namespace vm::bytecode
{
    BytecodeProgram::ConstantPool& BytecodeProgram::getConstantPool() {
        return constantPool;
    }

    const BytecodeProgram::ConstantPool& BytecodeProgram::getConstantPool() const {
        return constantPool;
    }

    void BytecodeProgram::registerFunction(const std::string& name, const FunctionMetadata& metadata) {
        functions[name] = metadata;

        // allPrimitiveParams is computed once at registration: the listed types
        // never need convertLambdaArgumentsToInterfaces or auto-boxing at call sites.
        {
            auto& stored = functions[name];
            bool allPrim = true;
            for (const auto& t : stored.parameterTypes)
            {
                if (t != "int" && t != "float" && t != "bool" && t != "string"
                    && t != "void")
                {
                    allPrim = false;
                    break;
                }
            }
            stored.allPrimitiveParams = allPrim;
        }

        if (functionNameToIndex.find(name) == functionNameToIndex.end()) {
            size_t idx = functionIndexToName.size();
            functionIndexToName.push_back(name);
            functionNameToIndex[name] = idx;
        }

        // MYT-197: intern the name into the frame-name interner and backfill
        // its metadata pointer. std::unordered_map does not invalidate
        // pointers to values on rehash, so &functions[name] stays stable
        // for the program's lifetime.
        FunctionNameHandle handle = internFrameName(name);
        frameNameToMeta[handle.id] = &functions[name];
    }

    FunctionNameHandle BytecodeProgram::internFrameName(std::string_view name) const {
        auto it = frameNameToId.find(name);
        if (it != frameNameToId.end()) {
            return FunctionNameHandle{ it->second };
        }
        const uint32_t id = static_cast<uint32_t>(frameNameById.size());
        if (id == INVALID_FN_HANDLE.id) {
            throw std::runtime_error("BytecodeProgram: frame-name interner exhausted");
        }
        frameNameById.emplace_back(name);
        frameNameToMeta.push_back(nullptr);
        // Key the map on a string_view into the deque entry. deque::emplace_back
        // does not invalidate references to existing elements, so views from
        // earlier inserts remain valid.
        frameNameToId.emplace(std::string_view(frameNameById.back()), id);
        return FunctionNameHandle{ id };
    }

    const std::string& BytecodeProgram::getFrameName(FunctionNameHandle handle) const {
        // MYT-197: degrade gracefully on stale/cross-program handles so stack
        // traces continue rather than throw mid-format.
        static const std::string kEmpty;
        if (handle.id >= frameNameById.size()) {
            return kEmpty;
        }
        return frameNameById[handle.id];
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunctionMeta(FunctionNameHandle handle) const {
        if (handle.id >= frameNameToMeta.size()) {
            return nullptr;
        }
        return frameNameToMeta[handle.id];
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunction(const std::string& name) const {
        auto it = functions.find(name);
        return it != functions.end() ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, BytecodeProgram::FunctionMetadata>& BytecodeProgram::getFunctions() const {
        return functions;
    }

    size_t BytecodeProgram::getFunctionIndex(const std::string& name) const {
        auto it = functionNameToIndex.find(name);
        return it != functionNameToIndex.end() ? it->second : SIZE_MAX;
    }

    const BytecodeProgram::FunctionMetadata* BytecodeProgram::getFunctionByIndex(size_t index) const {
        if (index >= functionIndexToName.size()) return nullptr;
        return getFunction(functionIndexToName[index]);
    }

    size_t BytecodeProgram::getFunctionCount() const {
        return functionIndexToName.size();
    }

    void BytecodeProgram::registerGlobalVariable(const GlobalVariableMetadata& metadata) {
        globalVariables.push_back(metadata);
    }

    const std::vector<BytecodeProgram::GlobalVariableMetadata>& BytecodeProgram::getGlobalVariables() const {
        return globalVariables;
    }

    ExceptionTable& BytecodeProgram::getGlobalExceptionTable() {
        return globalExceptionTable;
    }

    const ExceptionTable& BytecodeProgram::getGlobalExceptionTable() const {
        return globalExceptionTable;
    }

    void BytecodeProgram::addSourceLocation(size_t instructionOffset, uint32_t line, uint32_t column, const std::string& filename) {
        sourceLocations[instructionOffset] = {line, column, filename};
    }

    const BytecodeProgram::SourceLocation* BytecodeProgram::getSourceLocation(size_t instructionOffset) const {
        auto it = sourceLocations.find(instructionOffset);
        return it != sourceLocations.end() ? &it->second : nullptr;
    }

    void BytecodeProgram::setEntryPoint(size_t offset) {
        entryPoint = offset;
    }

    size_t BytecodeProgram::getEntryPoint() const {
        return entryPoint;
    }

    void BytecodeProgram::setSourceFilePath(const std::string& path) {
        sourceFilePath = path;
    }

    const std::string& BytecodeProgram::getSourceFilePath() const {
        return sourceFilePath;
    }

    void BytecodeProgram::registerClass(const ClassMetadata& classMeta) {
        classes.push_back(classMeta);
    }

    const std::vector<BytecodeProgram::ClassMetadata>& BytecodeProgram::getClasses() const {
        return classes;
    }

    void BytecodeProgram::addInterface(const InterfaceMetadata& interfaceMeta) {
        interfaces.push_back(interfaceMeta);
    }

    const std::vector<BytecodeProgram::InterfaceMetadata>& BytecodeProgram::getInterfaces() const {
        return interfaces;
    }

    bool BytecodeProgram::hasAsyncFunctions() const {
        // Async-flagged across global functions, instance methods, static
        // methods, and constructors — all registered in `functions` by their
        // fully-qualified names.
        for (const auto& pair : functions) {
            if (pair.second.isAsync) {
                return true;
            }
        }
        return false;
    }

    bool BytecodeProgram::hasAwaitInstructions() const {
        // True iff the program actually uses await (a function may be marked
        // async without ever awaiting).
        for (const auto& instr : instructions) {
            if (instr.opcode == OpCode::AWAIT) {
                return true;
            }
        }
        return false;
    }

    void BytecodeProgram::addAnnotationDeclaration(const AnnotationDeclData& declData) {
        annotationDeclarations.push_back(declData);
    }

    const std::vector<BytecodeProgram::AnnotationDeclData>&
        BytecodeProgram::getAnnotationDeclarations() const {
        return annotationDeclarations;
    }

    void BytecodeProgram::addStaticInitializerFunction(const std::string& functionName) {
        staticInitializerFunctions.push_back(functionName);
    }

    const std::vector<std::string>& BytecodeProgram::getStaticInitializerFunctions() const {
        return staticInitializerFunctions;
    }
}

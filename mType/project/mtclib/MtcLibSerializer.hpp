#pragma once
#include "MtcLibFormat.hpp"
#include <iostream>

namespace project::mtclib
{
    /**
     * Serializer/deserializer for .mtcLib binary format.
     * Produces a single self-contained binary file with embedded bytecode.
     */
    class MtcLibSerializer
    {
    public:
        static void serialize(const MtcLibProgram& lib, std::ostream& out);
        static MtcLibProgram deserialize(std::istream& in);

    private:
        // Section writers
        static void writeHeader(std::ostream& out, const MtcLibHeader& header);
        static void writeMetadata(std::ostream& out, const MtcLibMetadata& metadata);
        static void writeDependencies(std::ostream& out, const std::vector<MtcLibDependency>& deps);
        static void writeExports(std::ostream& out, const std::vector<MtcLibExport>& exports);
        static void writeImports(std::ostream& out, const std::vector<MtcLibImport>& imports);
        static void writeBytecodePayload(std::ostream& out, const vm::bytecode::BytecodeProgram& program);

        // Section readers
        static MtcLibHeader readHeader(std::istream& in);
        static MtcLibMetadata readMetadata(std::istream& in);
        static std::vector<MtcLibDependency> readDependencies(std::istream& in);
        static std::vector<MtcLibExport> readExports(std::istream& in);
        static std::vector<MtcLibImport> readImports(std::istream& in);
        static vm::bytecode::BytecodeProgram readBytecodePayload(std::istream& in);
    };
}

#include "MtcLibSerializer.hpp"
#include "../../vm/bytecode/BytecodeIOHelper.hpp"
#include "../../constants/SecurityConstants.hpp"
#include <sstream>
#include <stdexcept>
#include <cstring>

namespace project::mtclib
{
    using BytecodeIOHelper = vm::bytecode::BytecodeIOHelper;

    // === Serialize ===

    void MtcLibSerializer::serialize(const MtcLibProgram& lib, std::ostream& out)
    {
        writeHeader(out, lib.header);
        writeMetadata(out, lib.metadata);
        writeDependencies(out, lib.dependencies);
        writeExports(out, lib.exports);
        writeImports(out, lib.imports);
        writeBytecodePayload(out, lib.bytecodeProgram);
    }

    void MtcLibSerializer::writeHeader(std::ostream& out, const MtcLibHeader& header)
    {
        out.write(header.magic, 6);
        out.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));
        out.write(reinterpret_cast<const char*>(&header.flags), sizeof(header.flags));
    }

    void MtcLibSerializer::writeMetadata(std::ostream& out, const MtcLibMetadata& metadata)
    {
        BytecodeIOHelper::writeString(out, metadata.name);
        BytecodeIOHelper::writeString(out, metadata.version);
        BytecodeIOHelper::writeString(out, metadata.mtypeVersion);
        out.write(reinterpret_cast<const char*>(&metadata.sourceHash), sizeof(metadata.sourceHash));
    }

    void MtcLibSerializer::writeDependencies(std::ostream& out, const std::vector<MtcLibDependency>& deps)
    {
        uint32_t count = static_cast<uint32_t>(deps.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& dep : deps) {
            BytecodeIOHelper::writeString(out, dep.name);
            BytecodeIOHelper::writeString(out, dep.versionConstraint);
            out.write(reinterpret_cast<const char*>(&dep.contentHash), sizeof(dep.contentHash));
        }
    }

    void MtcLibSerializer::writeExports(std::ostream& out, const std::vector<MtcLibExport>& exports)
    {
        uint32_t count = static_cast<uint32_t>(exports.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& exp : exports) {
            auto kind = static_cast<uint8_t>(exp.kind);
            out.write(reinterpret_cast<const char*>(&kind), sizeof(kind));
            BytecodeIOHelper::writeString(out, exp.name);
            BytecodeIOHelper::writeString(out, exp.signature);
            out.write(reinterpret_cast<const char*>(&exp.bytecodeOffset), sizeof(exp.bytecodeOffset));
            auto vis = static_cast<uint8_t>(exp.visibility);
            out.write(reinterpret_cast<const char*>(&vis), sizeof(vis));
        }
    }

    void MtcLibSerializer::writeImports(std::ostream& out, const std::vector<MtcLibImport>& imports)
    {
        uint32_t count = static_cast<uint32_t>(imports.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& imp : imports) {
            auto kind = static_cast<uint8_t>(imp.kind);
            out.write(reinterpret_cast<const char*>(&kind), sizeof(kind));
            BytecodeIOHelper::writeString(out, imp.name);
            BytecodeIOHelper::writeString(out, imp.signature);
            BytecodeIOHelper::writeString(out, imp.sourceLib);
        }
    }

    void MtcLibSerializer::writeBytecodePayload(std::ostream& out, const vm::bytecode::BytecodeProgram& program)
    {
        // Serialize BytecodeProgram to a temporary buffer to get the size
        std::ostringstream buffer;
        program.serialize(buffer);
        std::string payload = buffer.str();

        uint64_t payloadSize = payload.size();
        out.write(reinterpret_cast<const char*>(&payloadSize), sizeof(payloadSize));
        out.write(payload.data(), static_cast<std::streamsize>(payloadSize));
    }

    // === Deserialize ===

    MtcLibProgram MtcLibSerializer::deserialize(std::istream& in)
    {
        MtcLibProgram lib;
        lib.header = readHeader(in);
        lib.metadata = readMetadata(in);
        lib.dependencies = readDependencies(in);
        lib.exports = readExports(in);
        lib.imports = readImports(in);
        lib.bytecodeProgram = readBytecodePayload(in);
        return lib;
    }

    MtcLibHeader MtcLibSerializer::readHeader(std::istream& in)
    {
        MtcLibHeader header;
        in.read(header.magic, 6);

        if (std::memcmp(header.magic, "MTCLIB", 6) != 0) {
            throw std::runtime_error("Invalid .mtcLib file: bad magic number");
        }

        in.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
        if (header.version != MtcLibHeader::CURRENT_VERSION) {
            throw std::runtime_error(
                "Unsupported .mtcLib format version: " + std::to_string(header.version) +
                " (expected " + std::to_string(MtcLibHeader::CURRENT_VERSION) + ")");
        }

        in.read(reinterpret_cast<char*>(&header.flags), sizeof(header.flags));
        return header;
    }

    MtcLibMetadata MtcLibSerializer::readMetadata(std::istream& in)
    {
        MtcLibMetadata metadata;
        metadata.name = BytecodeIOHelper::readString(in);
        metadata.version = BytecodeIOHelper::readString(in);
        metadata.mtypeVersion = BytecodeIOHelper::readString(in);
        in.read(reinterpret_cast<char*>(&metadata.sourceHash), sizeof(metadata.sourceHash));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in metadata");
        return metadata;
    }

    std::vector<MtcLibDependency> MtcLibSerializer::readDependencies(std::istream& in)
    {
        uint32_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in dependencies");

        if (count > constants::security::MAX_MTCLIB_DEPENDENCIES) {
            throw std::runtime_error("Library dependency count (" + std::to_string(count) +
                ") exceeds security limit (" + std::to_string(constants::security::MAX_MTCLIB_DEPENDENCIES) + ")");
        }

        std::vector<MtcLibDependency> deps(count);
        for (auto& dep : deps) {
            dep.name = BytecodeIOHelper::readString(in);
            dep.versionConstraint = BytecodeIOHelper::readString(in);
            in.read(reinterpret_cast<char*>(&dep.contentHash), sizeof(dep.contentHash));
            if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in dependency entry");
        }
        return deps;
    }

    std::vector<MtcLibExport> MtcLibSerializer::readExports(std::istream& in)
    {
        uint32_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in exports");

        if (count > constants::security::MAX_MTCLIB_EXPORTS) {
            throw std::runtime_error("Library export count (" + std::to_string(count) +
                ") exceeds security limit (" + std::to_string(constants::security::MAX_MTCLIB_EXPORTS) + ")");
        }

        std::vector<MtcLibExport> exports(count);
        for (auto& exp : exports) {
            uint8_t kind;
            in.read(reinterpret_cast<char*>(&kind), sizeof(kind));
            if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in export entry");
            if (kind > static_cast<uint8_t>(SymbolKind::VARIABLE)) {
                throw std::runtime_error("Invalid SymbolKind in library: " + std::to_string(kind));
            }
            exp.kind = static_cast<SymbolKind>(kind);
            exp.name = BytecodeIOHelper::readString(in);
            exp.signature = BytecodeIOHelper::readString(in);
            in.read(reinterpret_cast<char*>(&exp.bytecodeOffset), sizeof(exp.bytecodeOffset));
            uint8_t vis;
            in.read(reinterpret_cast<char*>(&vis), sizeof(vis));
            if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in export entry");
            if (vis > static_cast<uint8_t>(SymbolVisibility::PROTECTED)) {
                throw std::runtime_error("Invalid SymbolVisibility in library: " + std::to_string(vis));
            }
            exp.visibility = static_cast<SymbolVisibility>(vis);
        }
        return exports;
    }

    std::vector<MtcLibImport> MtcLibSerializer::readImports(std::istream& in)
    {
        uint32_t count;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in imports");

        if (count > constants::security::MAX_MTCLIB_IMPORTS) {
            throw std::runtime_error("Library import count (" + std::to_string(count) +
                ") exceeds security limit (" + std::to_string(constants::security::MAX_MTCLIB_IMPORTS) + ")");
        }

        std::vector<MtcLibImport> imports(count);
        for (auto& imp : imports) {
            uint8_t kind;
            in.read(reinterpret_cast<char*>(&kind), sizeof(kind));
            if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in import entry");
            if (kind > static_cast<uint8_t>(SymbolKind::VARIABLE)) {
                throw std::runtime_error("Invalid SymbolKind in library import: " + std::to_string(kind));
            }
            imp.kind = static_cast<SymbolKind>(kind);
            imp.name = BytecodeIOHelper::readString(in);
            imp.signature = BytecodeIOHelper::readString(in);
            imp.sourceLib = BytecodeIOHelper::readString(in);
        }
        return imports;
    }

    vm::bytecode::BytecodeProgram MtcLibSerializer::readBytecodePayload(std::istream& in)
    {
        uint64_t payloadSize;
        in.read(reinterpret_cast<char*>(&payloadSize), sizeof(payloadSize));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream in bytecode payload header");

        if (payloadSize > constants::security::MAX_MTCLIB_PAYLOAD_SIZE) {
            throw std::runtime_error("Bytecode payload size (" + std::to_string(payloadSize) +
                ") exceeds security limit (" + std::to_string(constants::security::MAX_MTCLIB_PAYLOAD_SIZE) + ")");
        }

        // Read the raw payload bytes
        std::string payload(static_cast<size_t>(payloadSize), '\0');
        in.read(&payload[0], static_cast<std::streamsize>(payloadSize));
        if (!in) throw std::runtime_error("Unexpected end of .mtcLib stream: bytecode payload truncated");

        // Deserialize BytecodeProgram from the payload
        std::istringstream payloadStream(payload);
        return vm::bytecode::BytecodeProgram::deserialize(payloadStream);
    }
}

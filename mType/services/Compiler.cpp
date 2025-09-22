#include "Compiler.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include "../ast/serialization/ASTSerializer.hpp"
#include "ImportManager.hpp"
#include <filesystem>
#include <iostream>

namespace services
{
    bool Compiler::compile(const std::string& sourceFile, const std::string& outputFile)
    {
        try
        {
            // Parse the script with fresh, isolated components
            lexer::Lexer lexer(sourceFile);

            // Create fresh ImportManager - no shared state
            auto importManager = std::make_unique<ImportManager>();

            // Set base directory to the directory of the script file
            std::filesystem::path scriptPath(sourceFile);
            std::string baseDirectory = scriptPath.parent_path().string();
            importManager->setBaseDirectory(baseDirectory);

            // Create parser with fresh components
            parser::Parser parser(lexer, std::move(importManager));
            auto ast = parser.parseProgram();

            // Determine output path
            std::string outputFilePath = outputFile;
            if (outputFilePath.empty())
            {
                outputFilePath = sourceFile + "c"; // .mt -> .mtc
            }

            // Serialize with fresh serializer - no shared state
            ast::serialization::ASTSerializer serializer;
            return serializer.serializeWithImportResolution(ast.get(), outputFilePath, baseDirectory);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error compiling script: " << e.what() << std::endl;
            return false;
        }
    }
}
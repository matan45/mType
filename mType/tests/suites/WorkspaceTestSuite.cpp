#include "WorkspaceTestSuite.hpp"
#include "../../project/WorkspaceConfigParser.hpp"
#include "../../project/WorkspaceBuilder.hpp"
#include "../../project/ProjectDiscovery.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace tests::testSuite
{
    using namespace testFramework;

    void WorkspaceTestSuite::setupTests()
    {
        // =============================================
        // Workspace Parsing Tests
        // =============================================

        addCallbackTest("Parse basic workspace", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            if (config->name != "TestWorkspace")
                throw std::runtime_error("Expected name 'TestWorkspace', got '" + config->name + "'");
            if (config->version != "1.0.0")
                throw std::runtime_error("Expected version '1.0.0', got '" + config->version + "'");
            if (config->members.size() != 2)
                throw std::runtime_error("Expected 2 members, got " + std::to_string(config->members.size()));
            if (config->members[0].getName() != "core")
                throw std::runtime_error("Expected first member 'core', got '" + config->members[0].getName() + "'");
            if (config->members[1].getName() != "app")
                throw std::runtime_error("Expected second member 'app', got '" + config->members[1].getName() + "'");
        });

        addCallbackTest("Workspace members have parsed project configs", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            for (const auto& member : config->members)
            {
                if (!member.config)
                    throw std::runtime_error("Member '" + member.path + "' has no parsed config");
                if (member.config->name.empty())
                    throw std::runtime_error("Member at '" + member.path + "' has empty project name");
                if (member.config->resolvedSourceFiles.empty())
                    throw std::runtime_error("Member '" + member.getName() + "' has no resolved source files");
            }
        });

        addCallbackTest("Workspace output config parsed", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            if (config->output.directory != "build")
                throw std::runtime_error("Expected output directory 'build', got '" + config->output.directory + "'");
        });

        // =============================================
        // Name Override Tests
        // =============================================

        addCallbackTest("Member Name attribute overrides .mtproj name", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/nameOverride/NameOverride.mtworkspace");

            if (config->members.size() != 1)
                throw std::runtime_error("Expected 1 member, got " + std::to_string(config->members.size()));

            // The .mtproj has Name="mylib" but .mtworkspace overrides with Name="utils"
            if (config->members[0].getName() != "utils")
                throw std::runtime_error("Expected overridden name 'utils', got '" + config->members[0].getName() + "'");

            // The underlying config should still have the original project name
            if (config->members[0].config->name != "mylib")
                throw std::runtime_error("Expected underlying project name 'mylib', got '" + config->members[0].config->name + "'");
        });

        // =============================================
        // Error Handling Tests
        // =============================================

        addCallbackTest("Duplicate member names rejected", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;

            try
            {
                auto config = parser.parse("mType/tests/testFiles/workspace/duplicateNames/DuplicateNames.mtworkspace");
                throw std::runtime_error("Expected exception for duplicate names, but parsing succeeded");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("Duplicate") == std::string::npos && msg.find("duplicate") == std::string::npos)
                    throw std::runtime_error("Expected 'duplicate' in error message, got: " + msg);
            }
        });

        addCallbackTest("Missing member directory rejected", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;

            try
            {
                auto config = parser.parse("mType/tests/testFiles/workspace/missingMember/MissingMember.mtworkspace");
                throw std::runtime_error("Expected exception for missing member directory, but parsing succeeded");
            }
            catch (const std::runtime_error& e)
            {
                std::string msg = e.what();
                if (msg.find("not found") == std::string::npos && msg.find("Not found") == std::string::npos)
                    throw std::runtime_error("Expected 'not found' in error message, got: " + msg);
            }
        });

        addCallbackTest("Invalid root element rejected", "", [](services::ScriptAPI&) {
            // Create a temp file with wrong root element
            std::string tempPath = (std::filesystem::temp_directory_path() / "_test_bad_ws.mtworkspace").string();
            {
                std::ofstream out(tempPath);
                out << "<Project Name=\"Wrong\" Version=\"1.0.0\"><Source><Include>*.mt</Include></Source></Project>";
            }

            project::WorkspaceConfigParser parser;
            try
            {
                auto config = parser.parse(tempPath);
                std::filesystem::remove(tempPath);
                throw std::runtime_error("Expected exception for invalid root element, but parsing succeeded");
            }
            catch (const std::runtime_error& e)
            {
                std::filesystem::remove(tempPath);
                std::string msg = e.what();
                if (msg.find("Workspace") == std::string::npos)
                    throw std::runtime_error("Expected 'Workspace' in error message, got: " + msg);
            }
        });

        // =============================================
        // Workspace Discovery Tests
        // =============================================

        addCallbackTest("findWorkspace discovers .mtworkspace walking up", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;

            // Start from a subdirectory that should find the workspace above
            auto found = parser.findWorkspace("mType/tests/testFiles/workspace/basic/core/src");
            if (!found.has_value())
                throw std::runtime_error("Expected to find .mtworkspace, but got nullopt");

            std::string filename = std::filesystem::path(found.value()).filename().string();
            if (filename != "TestWorkspace.mtworkspace")
                throw std::runtime_error("Expected 'TestWorkspace.mtworkspace', got '" + filename + "'");
        });

        addCallbackTest("findProjectOrWorkspace prefers workspace over project", "", [](services::ScriptAPI&) {
            // The basic fixture has both .mtworkspace and .mtproj files
            // Workspace should be preferred
            auto result = project::findProjectOrWorkspace("mType/tests/testFiles/workspace/basic");
            if (!result.has_value())
                throw std::runtime_error("Expected discovery result, got nullopt");

            if (result->type != project::DiscoveryType::WORKSPACE)
                throw std::runtime_error("Expected WORKSPACE discovery type, got PROJECT");
        });

        addCallbackTest("findProjectOrWorkspace falls back to project", "", [](services::ScriptAPI&) {
            // The projectManifest directory only has .mtproj, no .mtworkspace
            auto result = project::findProjectOrWorkspace("mType/tests/testFiles/projectManifest");
            if (!result.has_value())
                throw std::runtime_error("Expected discovery result, got nullopt");

            if (result->type != project::DiscoveryType::PROJECT)
                throw std::runtime_error("Expected PROJECT discovery type, got WORKSPACE");
        });

        // =============================================
        // Build Order Tests
        // =============================================

        addCallbackTest("Workspace build produces result for all members", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            project::WorkspaceBuilder builder;
            auto result = builder.build(*config);

            if (!result.success)
            {
                std::string errors;
                for (const auto& e : result.errors) errors += e + "; ";
                throw std::runtime_error("Build failed: " + errors);
            }

            if (result.projectsBuilt != 2)
                throw std::runtime_error("Expected 2 projects built, got " + std::to_string(result.projectsBuilt));

            if (result.totalFilesCompiled < 2)
                throw std::runtime_error("Expected at least 2 files compiled, got " + std::to_string(result.totalFilesCompiled));
        });

        addCallbackTest("Cross-project @alias import resolves end-to-end", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/crossImport/CrossImport.mtworkspace");

            project::WorkspaceBuilder builder;
            auto result = builder.build(*config);

            if (!result.success)
            {
                std::string errors;
                for (const auto& e : result.errors) errors += e + "; ";
                throw std::runtime_error("Cross-project build failed: " + errors);
            }

            // The app project imports from @lib/ — if alias injection works,
            // both projects compile successfully
            if (result.projectsBuilt != 2)
                throw std::runtime_error("Expected 2 projects built, got " + std::to_string(result.projectsBuilt));

            // Clean up build artifacts
            builder.clean(*config);
        });

        addCallbackTest("Workspace clean removes build artifacts", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            // Build first to create artifacts
            project::WorkspaceBuilder builder;
            builder.build(*config);

            // Clean
            builder.clean(*config);

            // Verify build directories are gone
            for (const auto& member : config->members)
            {
                if (member.config)
                {
                    std::filesystem::path buildDir =
                        std::filesystem::path(member.config->projectRoot) / member.config->output.directory;
                    if (std::filesystem::exists(buildDir))
                        throw std::runtime_error("Build directory still exists after clean: " + buildDir.string());
                }
            }
        });

        // =============================================
        // Workspace Alias Tests
        // =============================================

        addCallbackTest("Workspace aliases use @projectName format", "", [](services::ScriptAPI&) {
            project::WorkspaceConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/workspace/basic/TestWorkspace.mtworkspace");

            // Simulate what WorkspaceBuilder::buildWorkspaceAliases does
            std::unordered_map<std::string, std::string> aliases;
            for (const auto& member : config->members)
            {
                if (member.config)
                {
                    std::string aliasName = "@" + member.getName();
                    aliases[aliasName] = member.config->projectRoot;
                }
            }

            if (aliases.find("@core") == aliases.end())
                throw std::runtime_error("Expected '@core' alias to be generated");
            if (aliases.find("@app") == aliases.end())
                throw std::runtime_error("Expected '@app' alias to be generated");
            if (aliases["@core"].empty())
                throw std::runtime_error("'@core' alias has empty path");
        });

        // =============================================
        // XmlParser Shared Utility Tests
        // =============================================

        addCallbackTest("XmlParser handles self-closing tags", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><Item Name=\"test\" /></Root>");

            if (element.tagName != "Root")
                throw std::runtime_error("Expected root tag 'Root', got '" + element.tagName + "'");
            if (element.children.size() != 1)
                throw std::runtime_error("Expected 1 child, got " + std::to_string(element.children.size()));
            if (element.children[0].tagName != "Item")
                throw std::runtime_error("Expected child tag 'Item', got '" + element.children[0].tagName + "'");

            auto nameIt = element.children[0].attributes.find("Name");
            if (nameIt == element.children[0].attributes.end())
                throw std::runtime_error("Expected 'Name' attribute on child");
            if (nameIt->second != "test")
                throw std::runtime_error("Expected Name='test', got '" + nameIt->second + "'");
        });

        addCallbackTest("XmlParser handles nested content", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><Child>content text</Child></Root>");

            if (element.children.size() != 1)
                throw std::runtime_error("Expected 1 child, got " + std::to_string(element.children.size()));
            if (element.children[0].content != "content text")
                throw std::runtime_error("Expected content 'content text', got '" + element.children[0].content + "'");
        });

        addCallbackTest("XmlParser handles xml declaration", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<?xml version=\"1.0\"?><Root Name=\"test\" />");

            if (element.tagName != "Root")
                throw std::runtime_error("Expected root tag 'Root', got '" + element.tagName + "'");
        });

        addCallbackTest("XmlParser drops comment between siblings", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><!-- foo --><Child/></Root>");

            if (element.children.size() != 1)
                throw std::runtime_error("Expected 1 child after dropping comment, got " +
                                         std::to_string(element.children.size()));
            if (element.children[0].tagName != "Child")
                throw std::runtime_error("Expected child tag 'Child', got '" + element.children[0].tagName + "'");
        });

        addCallbackTest("XmlParser handles comment with embedded gt", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><!-- a > b --><Child/></Root>");

            if (element.children.size() != 1)
                throw std::runtime_error("Embedded '>' in comment corrupted parsing; got " +
                                         std::to_string(element.children.size()) + " children");
            if (element.children[0].tagName != "Child")
                throw std::runtime_error("Expected child tag 'Child', got '" + element.children[0].tagName + "'");
        });

        addCallbackTest("XmlParser handles comment with embedded markup", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><!-- < </X> --><Child/></Root>");

            if (element.children.size() != 1)
                throw std::runtime_error("Embedded '</X>' in comment corrupted parsing; got " +
                                         std::to_string(element.children.size()) + " children");
            if (element.children[0].tagName != "Child")
                throw std::runtime_error("Expected child tag 'Child', got '" + element.children[0].tagName + "'");
        });

        addCallbackTest("XmlParser handles comments around root", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<!-- before --><Root Name=\"x\"/><!-- after -->");

            if (element.tagName != "Root")
                throw std::runtime_error("Expected root tag 'Root', got '" + element.tagName + "'");
            auto nameIt = element.attributes.find("Name");
            if (nameIt == element.attributes.end())
                throw std::runtime_error("Expected 'Name' attribute on root");
            if (nameIt->second != "x")
                throw std::runtime_error("Expected Name='x', got '" + nameIt->second + "'");
        });

        addCallbackTest("XmlParser preserves content with leading comment", "", [](services::ScriptAPI&) {
            project::XmlParser xmlParser;
            auto element = xmlParser.parse("<Root><Item><!-- inner -->value</Item></Root>");

            if (element.children.size() != 1)
                throw std::runtime_error("Expected 1 child, got " + std::to_string(element.children.size()));
            if (element.children[0].tagName != "Item")
                throw std::runtime_error("Expected child tag 'Item', got '" + element.children[0].tagName + "'");
            if (element.children[0].content != "value")
                throw std::runtime_error("Expected content 'value', got '" + element.children[0].content + "'");
        });

        // =============================================
        // ProjectConfigParser still works after refactor
        // =============================================

        addCallbackTest("ProjectConfigParser still parses .mtproj after XmlParser extraction", "", [](services::ScriptAPI&) {
            project::ProjectConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/projectManifest/TestProject.mtproj");

            if (config->name != "TestProject")
                throw std::runtime_error("Expected 'TestProject', got '" + config->name + "'");
            if (config->imports.searchPaths.empty())
                throw std::runtime_error("Expected search paths, got none");
            if (config->imports.aliases.empty())
                throw std::runtime_error("Expected aliases, got none");
            if (config->resolvedSourceFiles.empty())
                throw std::runtime_error("Expected resolved source files, got none");
        });

        addCallbackTest("ProjectConfigParser resolves bare relative .mtproj from cwd", "", [](services::ScriptAPI&) {
            struct CurrentPathGuard
            {
                std::filesystem::path originalPath;

                CurrentPathGuard() : originalPath(std::filesystem::current_path()) {}

                ~CurrentPathGuard()
                {
                    std::error_code ec;
                    std::filesystem::current_path(originalPath, ec);
                }
            } guard;

            auto fixtureRoot = std::filesystem::canonical("mType/tests/testFiles/projectManifest");
            std::filesystem::current_path(fixtureRoot);

            project::ProjectConfigParser parser;
            auto config = parser.parse("TestProject.mtproj");

            if (std::filesystem::path(config->projectRoot) != fixtureRoot)
            {
                throw std::runtime_error("Expected projectRoot '" + fixtureRoot.string() +
                                         "', got '" + config->projectRoot + "'");
            }
            if (config->resolvedSourceFiles.empty())
                throw std::runtime_error("Expected resolved source files for bare relative .mtproj, got none");
        });

        addCallbackTest("ProjectConfigParser parses .mtproj with comments", "", [](services::ScriptAPI&) {
            project::ProjectConfigParser parser;
            auto config = parser.parse("mType/tests/testFiles/projectManifest/TestProjectWithComments.mtproj");

            if (config->name != "TestProjectWithComments")
                throw std::runtime_error("Expected 'TestProjectWithComments', got '" + config->name + "'");
            if (config->imports.searchPaths.empty())
                throw std::runtime_error("Expected search paths after comment-laden manifest, got none");
            if (config->imports.aliases.empty())
                throw std::runtime_error("Expected aliases after comment-laden manifest, got none");
            if (config->resolvedSourceFiles.empty())
                throw std::runtime_error("Expected resolved source files, got none");
        });
    }
}

# mType 1.0.0 Release Procedure

This release is distributed through GitHub Releases from `v1.0.0` tags.

## Local Validation

1. Regenerate project files.
   - Windows: `runPremake.bat`
   - Linux/macOS: `premake5 gmake2`
2. Build Release x64.
   - Windows: `msbuild Interpreter.sln /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /m`
   - Linux/macOS: `make -j$(nproc) config=release_x64`
3. Run release-grade native checks.
   - `mType --tests`
   - `mType --tests --no-jit`
   - `mtype-language-server-tests`
   - `mType --version`
   - `mtpm --version`
   - `mtype-language-server --version`
   - `mType --help`
   - `mtpm --help`
4. Validate the VS Code extension.
   - `cd mtype-vscode-extension`
   - `npm ci`
   - `npm run compile`
   - `npx @vscode/vsce package`
   - Install the generated `.vsix`, open a `.mt` file, and confirm the language server starts from the configured, bundled, or PATH binary.

## Tag and Publish

1. Ensure `mType/version/Version.hpp`, root docs, and extension metadata all refer to `1.0.0`.
2. Commit the release changes.
3. Create and push the release tag: `git tag v1.0.0 && git push origin v1.0.0`.
4. Confirm the release workflow uploads:
   - `mType-1.0.0-windows-x64.zip`
   - `mType-1.0.0-linux-x64.tar.gz`
   - `mType-1.0.0-macos-x64.tar.gz`
   - `mtype-language-support-1.0.0.vsix`
5. Download each archive on a clean machine/path and run:
   - `mType --version`
   - `mtpm --version`
   - `mtype-language-server --version`
   - a simple `.mt` script
   - one `.mtproj` build

Marketplace, MSI, Homebrew, apt, winget, Chocolatey, and Scoop distribution are intentionally out of scope for 1.0.0.

@echo off
REM MYT-139: Orphan-obj cleanup.
REM
REM An older premake5.lua configuration emitted build artifacts directly
REM into mType\obj\x64\<Config>\ (no per-project subdirectory). The
REM current lua emits per-project (mType\obj\x64\<Config>\<ProjName>\).
REM Obj/pdb/tlog files left in the flat root from the old layout are
REM orphaned: no current vcxproj targets them, but they can leak into
REM the link if a .lib in bin/ was last built when the flat layout was
REM active. This sweeps them so a regen + incremental build starts clean.
REM
REM Sweep is idempotent and cheap (no-op when the root is already empty).
for %%C in (Debug Release) do (
    if exist "mType\obj\x64\%%C\*.obj" del /q "mType\obj\x64\%%C\*.obj"
    if exist "mType\obj\x64\%%C\*.pdb" del /q "mType\obj\x64\%%C\*.pdb"
    if exist "mType\obj\x64\%%C\*.tlog" del /q "mType\obj\x64\%%C\*.tlog"
    if exist "mType\obj\x64\%%C\*.lastbuildstate" del /q "mType\obj\x64\%%C\*.lastbuildstate"
)

premake5 vs2022
pause

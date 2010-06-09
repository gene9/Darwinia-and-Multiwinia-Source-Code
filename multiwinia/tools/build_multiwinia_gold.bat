@echo off
SETLOCAL

SET TOOLS=%~dp0
if not defined BUILD (set BUILD=build)

call build_multiwinia_data.bat

CD %TOOLS%\..

REM executables 
copy darwinia_gl.exe build\multiwinia.exe
copy contrib\systemIV\contrib\XCrashReport\dbghelp.dll build\
copy MultiwiniaSupport.ini build\
copy tools\multiwiniasupport\MultiwiniaSupport.exe build\
copy tools\autopatcher\autopatch.exe build\
copy tools\autopatcher\copy_assist.exe build\
copy tools\profilelocatorwin32\profilelocator.exe build\

REM manual
copy docs\multiwinia_manual.pdf build\manual.pdf

REM Cause problems with debugging tools
REM tools\upx --best build\MultiwiniaSupport.exe
REM tools\upx --best build\multiwinia.exe

:end
@pause

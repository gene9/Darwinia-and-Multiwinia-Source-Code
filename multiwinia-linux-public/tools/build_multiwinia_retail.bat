@echo off
SETLOCAL

SET TOOLS=%~dp0
if not defined BUILD (set BUILD=build)

SET LANGUAGE=lang%1
IF %LANGUAGE% == lang (
	SET LANGUAGE=langEnglish
)

call build_multiwinia_data.bat retail %LANGUAGE:~4%

CD %TOOLS%\..

REM executables 
copy darwinia_gl.exe build\multiwinia.exe
copy contrib\systemIV\contrib\XCrashReport\dbghelp.dll build\
copy MultiwiniaSupport.ini build\
copy tools\multiwiniasupport\MultiwiniaSupport.exe build\
copy tools\autopatcher\copy_assist.exe build\
copy tools\profilelocatorwin32\profilelocator.exe build\

IF /I %LANGUAGE%==langfrench ( copy tools\autopatcher\autopatch_fr.exe build\autopatch.exe )
IF /I %LANGUAGE%==langitalian ( copy tools\autopatcher\autopatch_it.exe build\autopatch.exe )
IF /I %LANGUAGE%==langgerman ( copy tools\autopatcher\autopatch_de.exe build\autopatch.exe )
IF /I %LANGUAGE%==langspanish ( copy tools\autopatcher\autopatch_es.exe build\autopatch.exe )
IF /I %LANGUAGE%==langenglish ( copy tools\autopatcher\autopatch.exe build\ )
IF /I %LANGUAGE%==langall ( copy tools\autopatcher\autopatch.exe build\ )
IF /I %LANGUAGE%==langefigs ( copy tools\autopatcher\autopatch.exe build\ )
REM Use english autopatcher for now
IF /I %LANGUAGE%==langrussian ( copy tools\autopatcher\autopatch.exe build\autopatch.exe )

REM manual
copy docs\multiwinia_manual.pdf build\manual.pdf

REM Cause problems with debugging tools
REM tools\upx --best build\MultiwiniaSupport.exe
REM tools\upx --best build\multiwinia.exe

:end
@pause

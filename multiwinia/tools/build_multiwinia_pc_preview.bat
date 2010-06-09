@echo off
SETLOCAL

SET TOOLS=%~dp0
if not defined BUILD (set BUILD=build)

call build_multiwinia_data.bat pcpreview %1
echo Data Building complete

CD %TOOLS%\..

REM executables 
copy darwinia_gl.exe build\multiwinia.exe
REM tools\upx --best build\multiwinia.exe

:end
@pause

REM @ECHO OFF
SETLOCAL
SET TOOLS=%~dp0
cd ..

call tools\build_language_dat platretail french build\french %TOOLS%
call tools\build_language_dat platretail italian build\italian %TOOLS%
call tools\build_language_dat platretail german build\german %TOOLS%
call tools\build_language_dat platretail spanish build\spanish %TOOLS%


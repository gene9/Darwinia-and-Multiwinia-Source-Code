@ECHO OFF
SETLOCAL

SET BUILD=%1
SET TOOLS=%2

REM date
FOR /F "TOKENS=1,2,3* DELIMS=/" %%A IN ('%TOOLS%\EnglishDate.exe') DO (
	SET dd=%%A
	SET mm=%%B
	SET yyyy=%%C
)

SET NOWDATE=%yyyy:~0,4%.%mm%.%dd%
FOR /F %%V IN ('TYPE %BUILD%\version') DO (SET VERSION=%%V)
SET SYMBOLSBASE=%BUILD%\..\symbols\%VERSION%
SET SYMBOLSTODAY=%SYMBOLSBASE%\%NOWDATE%
SET SYMBOLSLATEST=%SYMBOLSBASE%\latest

IF NOT EXIST %SYMBOLSTODAY% mkdir %SYMBOLSTODAY%
IF NOT EXIST %SYMBOLSLATEST% mkdir %SYMBOLSLATEST%

REM see what the next number is 
PUSHD %SYMBOLSTODAY%
SET /A FOLDER_NUM=1
FOR /D %%X IN ( "*" ) DO IF %FOLDER_NUM% LEQ %%X ( SET /A FOLDER_NUM = "%%X+1" ) 
POPD

REM Save symbols if no latest symbols recorded
IF NOT EXIST %SYMBOLSLATEST%\Darwinia.pdb ( GOTO :saveSymbols )

REM Check to see if we've already got the symbols
FOR /F %%a IN ("Darwinia.pdb") DO SET ts1=%%~ta %%~za
FOR /F %%a IN ("%SYMBOLSLATEST%\Darwinia.pdb") DO SET ts2=%%~ta %%~za
IF NOT "%ts1%"=="%ts2%" ( GOTO :saveSymbols )
ECHO Symbols already saved
EXIT /b

:saveSymbols
ECHO Saving Symbols 
IF NOT EXIST %SYMBOLSTODAY%\%FOLDER_NUM% MKDIR %SYMBOLSTODAY%\%FOLDER_NUM%
COPY Darwinia.pdb %SYMBOLSTODAY%\%FOLDER_NUM%
COPY darwinia_gl.exe %SYMBOLSTODAY%\%FOLDER_NUM%
COPY Darwinia.pdb %SYMBOLSLATEST%
COPY darwinia_gl.exe %SYMBOLSLATEST%
EXIT /b

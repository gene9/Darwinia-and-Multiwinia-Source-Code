@echo off
setlocal

REM Perl command path here!
set PERL=perl -w

set REPORT=%PERL% report.pl -a
set ZIP=7za u -tzip

rem %REPORT% french.txt

%REPORT% -n english-strings.txt french.txt
%ZIP% french.zip french-strings.txt french-report.txt english-strings.txt

%REPORT% italian.txt
%ZIP% italian.zip italian-strings.txt italian-report.txt english-strings.txt

%REPORT% german.txt
%ZIP% german.zip german-strings.txt german-report.txt english-strings.txt

%REPORT% spanish.txt
%ZIP% spanish.zip spanish-strings.txt spanish-report.txt english-strings.txt

rem %REPORT% dutch.txt
rem %ZIP% dutch.zip dutch-strings.txt dutch-report.txt english-strings.txt
rem %REPORT% russian.txt
rem %ZIP% russian.zip russian-strings.txt russian-report.txt english-strings.txt

del /q *-strings.txt *-report.txt

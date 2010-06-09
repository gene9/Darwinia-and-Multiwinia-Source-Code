@echo off
setlocal

REM Perl command path here!
set PERL=perl -w

set REPORT=%PERL% report.pl -a
set ZIP=7za u -tzip

%REPORT% -n english-strings.txt russian.txt
%ZIP% russian.zip russian-strings.txt russian-report.txt english-strings.txt

del /q *-strings.txt *-report.txt
pause
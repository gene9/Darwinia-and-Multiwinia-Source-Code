if %CD%==F:\Coding\Darwinia\tools goto label
if %CD%==f:\coding\darwinia\tools goto label
if %CD%==C:\Development\Darwinia\tools goto label
if %CD%==c:\development\darwinia\tools goto label
echo Must be ran from the Darwinia TOOLS directory
exit /b

:label
cd ..
del main.dat
del sounds.dat
cd data

dir *.bmp *.jpg *.shp *.txt /s/b >list
..\tools\sed_dos 's_.*data\\_data\\_' list | ..\tools\grep_dos -v \.svn > list2
..\tools\sort_dos -f list2 | ..\tools\uniq_dos -i > list3
cd ..
tools\rsx tools\rar32 a -m5 -s -idp main.dat @data\list3
del data\list data\list2 data\list3

cd data
dir *.ogg /s/b >list
..\tools\sed_dos 's_.*data\\_data\\_' list | ..\tools\grep_dos -v \.svn > list2
..\tools\sort_dos -f list2 | ..\tools\uniq_dos -i > list3
cd ..
tools\rsx tools\rar32 a -m2 -s -idp sounds.dat @data\list3
del data\list data\list2 data\list3

md build
del /q/s build

copy darwinia.exe build
copy main.dat build
copy sounds.dat build
copy userid.txt build
copy docs\soundsystem.txt build

tools\upx --best build\darwinia.exe

@pause

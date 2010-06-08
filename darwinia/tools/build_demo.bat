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
del /q/s build

md build
md build\data
md build\data\scripts
md build\data\levels
md build\data\terrain

perl tools\listfiles.pl data tools\demo_exclusions.txt > listAll

tools\grep_dos .ogg < listAll > listOgg
tools\sed_dos -e 's/sounds/data\/sounds/' < listOgg > listOgg2
tools\rsx tools\rar32 a -m2 -s -idp sounds.dat @listOgg2

tools\grep_dos -v sounds/ < listAll > listData
perl tools\copyfiles.pl data build\data < listData
perl tools\copystrings.pl data\english.txt > build\data\english.txt

copy data\scripts\mine_primary1.txt build\data\scripts\
copy data\scripts\mine_primary2.txt build\data\scripts\
copy data\scripts\mine_minehelp.txt build\data\scripts\
copy data\scripts\mine_demo_intro.txt build\data\scripts\

copy data\levels\map_mine.txt build\data\levels\
copy data\levels\mission_mine_demo.txt build\data\levels\

copy data\terrain\landscape_mine.bmp build\data\terrain\
copy data\terrain\waves_containmentfield.bmp build\data\terrain\
copy data\terrain\water_default.bmp build\data\terrain\

cd build
cd data

dir *.bmp *.jpg *.shp *.txt /s/b >list
..\..\tools\sed_dos 's_.*data\\_data\\_' list > list2
cd ..
..\tools\rsx ..\tools\rar32 a -m5 -s -idp main.dat @data\list2
del data\list data\list2

rmdir /q/s data
cd ..
copy darwinia.exe build
copy sounds.dat build

tools\upx --best build\darwinia.exe

del listAll listOgg listOgg2 listData

@pause

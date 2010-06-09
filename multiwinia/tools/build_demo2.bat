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
md build\data\language

perl tools\listfiles.pl data tools\demo2_exclusions.txt > listAll

tools\grep_dos .ogg < listAll > listOgg
tools\sed_dos -e 's/sounds/data\/sounds/' < listOgg > listOgg2
tools\rsx tools\rar32 a -m2 -s -idp sounds.dat @listOgg2

tools\grep_dos -v sounds/ < listAll > listData
perl tools\copyfiles.pl data build\data < listData
perl tools\copystrings.pl data\language\english.txt > build\data\language\english.txt
perl tools\copystrings.pl data\language\german.txt > build\data\language\german.txt

copy data\levels\map_launchpad.txt build\data\levels\
copy data\levels\mission_launchpad.txt build\data\levels\

copy data\purity\map_mine.txt build\data\levels\map_mine.txt
copy data\purity\map_mine.txt build\data\levels\map_garden.txt
copy data\purity\map_mine.txt build\data\levels\map_containment.txt
copy data\purity\map_mine.txt build\data\levels\map_generator.txt
copy data\purity\map_mine.txt build\data\levels\map_escort.txt
copy data\purity\map_mine.txt build\data\levels\map_biosphere.txt
copy data\purity\map_mine.txt build\data\levels\map_temple.txt

copy data\terrain\landscape_launchpad.bmp build\data\terrain\
copy data\terrain\waves_launchpad.bmp build\data\terrain\
copy data\terrain\water_launchpad.bmp build\data\terrain\

copy data\scripts\launchpad*.txt build\data\scripts\

copy data\shaders\* build\data\shaders\

cd build
cd data

dir *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h /s/b >list
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

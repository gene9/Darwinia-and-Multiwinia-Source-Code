
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

perl tools\copyfiles.pl data build\data < tools\language.list

cd build
cd data

dir *.bmp *.txt /s/b >list
..\..\tools\sed_dos 's_.*data\\_data\\_' list > list2
cd ..
..\tools\rsx ..\tools\rar32 a -m5 -s -idp language.dat @data\list2
del data\list data\list2

del /q/s data
cd ..

perl tools\listfiles.pl data tools\full_exclusions.txt > listAll

tools\grep_dos .ogg < listAll > listOgg
tools\sed_dos -e 's/sounds/data\/sounds/' < listOgg > listOgg2
tools\rsx tools\rar32 a -m2 -s -idp sounds.dat @listOgg2

tools\grep_dos -v sounds/ < listAll > listData
perl tools\copyfiles.pl data build\data < listData

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
copy junk\manual.pdf build

tools\upx --best build\darwinia.exe

del listAll listOgg listOgg2 listData

@pause

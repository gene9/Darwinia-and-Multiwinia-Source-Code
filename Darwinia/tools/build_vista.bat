
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
..\..\tools\sed "s_.*data\\_data\\_" list > list2
cd ..
..\tools\rar a -m5 -s -idp language.dat @data\list2
del data\list data\list2

del /q/s data
cd ..

perl tools\listfiles.pl data tools\vista_exclusions.txt > listAll

tools\grep .ogg < listAll > listOgg
tools\sed -e "s/sounds/data\/sounds/" < listOgg > listOgg2
tools\rar a -m2 -s -idp sounds.dat @listOgg2

tools\grep -v sounds/ < listAll > listData
perl tools\copyfiles.pl data build\data < listData

cd build
cd data

dir *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h /s/b >list
..\..\tools\sed "s_.*data\\_data\\_" list > list2
cd ..
..\tools\rar a -m5 -s -idp main.dat @data\list2
del data\list data\list2

rmdir /q/s data
cd ..
copy darwinia.exe build
copy sounds.dat build
copy junk\darwinia_vista_manual_web.pdf build\manual.pdf

REM Skip UPX for now (not sure how it will interact with Oberon's wrapper)
REM tools\upx --best build\darwinia.exe

del listAll listOgg listOgg2 listData

REM Replace the icon with a vista icon
tools\ReplaceVistaIcon build\darwinia.exe targets\vs2005\DarwiniaVista.ico

REM Authenticode sign Darwinia. You need to expand certificate.rar
REM (get password from John)

tools\signtool sign /f certificates\mycert.pfx /t http://timestamp.comodoca.com/authenticode build\darwinia.exe

REM Save the EXE and PDB together

md release
set archive=Darwinia Release %DATE:/=-% %TIME::=-%.rar
tools\rar a -m2 -s -idp "release/%archive%" darwinia.exe build\darwinia.exe Darwinia.pdb

@pause


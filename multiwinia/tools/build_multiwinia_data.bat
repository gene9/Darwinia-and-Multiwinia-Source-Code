@ECHO OFF
SETLOCAL

if not defined BUILD (set BUILD=build)
SET TOOLS=%~dp0
SET PLATFORM=plat%1
SET LANGUAGE=lang%2

IF %LANGUAGE% ==lang (
	REM 	No language specified, default to all language
	SET LANGUAGE=all
) ELSE IF %PLATFORM% ==platxbox (
	REM 	XBox has all languages
	SET LANGUAGE=all
) ELSE (
	SET LANGUAGE=%LANGUAGE:~4%
)

CD %TOOLS%\..


copy /b tools\sounds.txt+tools\menu_sounds.txt completeExclusions

IF %PLATFORM% ==platxbox (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_xbox.txt
	type tools\multiwinia_exclusions_xbox.txt >> completeExclusions
) ELSE IF %PLATFORM% ==platpchwcompat (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_hwcompat.txt
	type tools\multiwinia_exclusions_hwcompat.txt >> completeExclusions
) ELSE IF %PLATFORM% ==platpcbetatest (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_beta_test.txt
	type tools\multiwinia_exclusions_beta_test.txt >> completeExclusions
) ELSE IF %PLATFORM% ==platpcpreview (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_beta_test.txt for pc preview
	type tools\multiwinia_exclusions_beta_test.txt >> completeExclusions
) ELSE IF %PLATFORM% ==platpcassaultstress (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_hwcompat.txt
	type tools\multiwinia_exclusions_hwcompat.txt >> completeExclusions 
) ELSE IF %PLATFORM% == platambx (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions_hwcompat.txt
	type tools\multiwinia_exclusions_beta_test.txt >> completeExclusions 
) ELSE (
	echo Making exclusion list of sounds.txt and menu_sounds.txt and multiwinia_exclusions.txt
	type tools\multiwinia_exclusions.txt >> completeExclusions
)
IF EXIST main.dat ( del main.dat  )
IF EXIST sounds.dat ( del sounds.dat  )
IF EXIST sounds2.dat ( del sounds2.dat )
rmdir /q/s %BUILD%

mkdir %BUILD%

REM goto makeSounds2

call %TOOLS%\build_language_dat %PLATFORM% %LANGUAGE% %BUILD% %TOOLS%
call %TOOLS%\build_ambx_dat %BUILD% %TOOLS%

:makeInit
REM init.dat
echo Making init.dat
mkdir %BUILD%\data
perl tools\copyfiles.pl data %BUILD%\data < tools\init.list

if %PLATFORM% == platxbox (
	ECHO Converting Xbox Splash Screen to tex file.
	call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %BUILD%\data\textures\xboxlivearcade.bmp D3DFMT_DXT1 1
	call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %BUILD%\data\textures\ivlogo.bmp D3DFMT_DXT1 1
	del %BUILD%\data\textures\ivlogo.bmp
)

del %BUILD%\data\textures\xboxlivearcade.bmp

pushd %BUILD%\data
dir *.* /s/b/a-d >..\list
%TOOLS%\sed "s_.*data\\_data\\_" ..\list > list2
cd ..
%TOOLS%\rar a -m5 -s -idp init.dat @data\list2
del list data\list2
popd

rmdir /q/s %BUILD%\data

:makeMain
REM main.dat
echo Making main.dat
mkdir %BUILD%\data
perl tools\listfiles.pl data completeExclusions > listAll
tools\grep -v sounds/ < listAll > listData
perl tools\copyfiles.pl data %BUILD%\data < listData

IF %PLATFORM% ==platxbox (
	ECHO Converting MWHelp bitmaps to tex files
	FOR %%X IN ( %BUILD%\data\mwhelp\*.bmp ) do ( call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %%X D3DFMT_DXT1 1 )
	ECHO Converting MWHelp bitmaps that require an alpha channel
	FOR %%X IN ( %BUILD%\data\mwhelp\360_controller.bmp ) do ( call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %%X D3DFMT_DXT3 1 )
	ECHO Converting Thumbnails bitmaps to tex files
	FOR %%X IN ( %BUILD%\data\thumbnails\*.bmp ) do ( call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %%X D3DFMT_DXT1 1 )
	
	ECHO Deleting old bitmaps
	DEL %BUILD%\data\mwhelp\*.bmp 
	DEL %BUILD%\data\thumbnails\*.bmp
) ELSE IF %PLATFORM% ==platpchwcompat (
	perl tools\copyfiles.pl data %BUILD%\data < tools\multiwinia_inclusions_hwcompat.txt
) ELSE IF %PLATFORM% ==platpcassaultstress (
	perl tools\copyfiles.pl data %BUILD%\data < tools\multiwinia_inclusions_assaultstresstest.txt
) ELSE IF %PLATFORM% ==platpcbetatest (
	perl tools\copyfiles.pl data %BUILD%\data < tools\multiwinia_inclusions_beta_test_%2.txt
) ELSE IF %PLATFORM% ==platpcpreview (
	echo Adding PC Preview Inclusions
	perl tools\copyfiles.pl data %BUILD%\data < tools\multiwinia_inclusions_previewcopy.txt
)ELSE IF %PLATFORM% ==platambx (
	echo Adding ambx Inclusions
	perl tools\copyfiles.pl data %BUILD%\data < tools\multiwinia_inclusions_ambx.txt
)

pushd %BUILD%\data
IF %PLATFORM% ==platxbox (
	dir *.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim *.wmv /s/b >list
) ELSE (
	dir *.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim /s/b >list
)
%TOOLS%\sed "s_.*data\\_data\\_" list > list2
cd ..
%TOOLS%\rar a -m5 -s -idp main.dat @data\list2
del data\list data\list2
popd
rmdir /q/s %BUILD%\data

del listData

:makeMenuSounds

REM menusounds.dat
echo Making menusounds.dat
tools\sed -e "s/^mwsounds/data\/mwsounds/" < %TOOLS%\menu_sounds.txt > menuSoundsList
tools\sed -e "s/^sounds/data\/sounds/" < menuSoundsList > menuSoundsListFinal
tools\rar a -m2 -s -idp menusounds.dat @menuSoundsListFinal
move /Y menusounds.dat %BUILD%
del menuSoundsListFinal
del menuSoundsList

:makeSounds

REM sounds.dat
echo Making sounds.dat
tools\sed -e "s/^/data\//" < %TOOLS%\sounds.txt > soundsListFinal
tools\rar a -m2 -s -idp sounds.dat @soundsListFinal
move /Y sounds.dat %BUILD%
del soundsListFinal

:makeSounds2

REM sounds2.dat
echo Making sounds.dat
tools\grep .ogg < listAll > listOgg
tools\sed -e "s/^mwsounds/data\/mwsounds/" < listOgg > listMwSounds
tools\sed -e "s/^sounds/data\/sounds/" < listMwSounds > listOggAll
tools\rar a -m2 -s -idp sounds2.dat @listOggAll
del listOgg
del listMwSounds
del listOggAll

move /Y sounds2.dat %BUILD%

del listAll
del completeExclusions

REM Below is used to copy .pdbs and .exe's to a directory so we can keep track of shit.
IF %PLATFORM% ==platxbox (
	REM No stuff for xbox
) ELSE (
	REM version - should take it from the version string really
	PUSHD %BUILD%
	..\darwinia_gl.exe --write-version
	POPD

	call tools\save_symbols.cmd "%BUILD%" "tools\"
)
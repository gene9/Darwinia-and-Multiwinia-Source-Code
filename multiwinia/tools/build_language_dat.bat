rem @echo off

SET PLATFORM=%1
SET LANGUAGE=%2
SET BUILD=%3
SET TOOLS=%4

REM language.dat
echo Making language.dat (language: %LANGUAGE% platform: %PLATFORM%)
mkdir %BUILD%\data %TOOLS%

IF %PLATFORM% ==platxbox (
	ECHO Adding every language in the WORLD stuff to XBox build
	perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\language.list
	ECHO Converting font bitmaps to tex files. This will take a f**king long time due to asian bmps
	FOR %%X IN ( %BUILD%\data\textures\unicode\*.bmp ) do ( call %TOOLS%\convert_bmp_to_tex.cmd %TOOLS:~0,-6% %BUILD% %%X D3DFMT_DXT1 )
	ECHO Deleting old bitmaps
	DEL %BUILD%\data\textures\unicode\*.bmp
) ELSE IF %PLATFORM% ==platretail (
	IF /I %LANGUAGE% ==all (
		ECHO Adding all language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\language.list
	) ELSE IF /I %LANGUAGE% ==efigs (
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\englishlanguage.list
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\frenchlanguage.list
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\germanlanguage.list
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\italianlanguage.list
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\spanishlanguage.list	
	) ELSE IF /I %LANGUAGE% ==english (
		ECHO Only adding English language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\englishlanguage.list
	) ELSE IF /I %LANGUAGE% ==french (
		ECHO Only adding French language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\frenchlanguage.list
	) ELSE IF /I %LANGUAGE% ==german (
		ECHO Only adding German language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\germanlanguage.list
	) ELSE IF /I %LANGUAGE% ==korean (
		ECHO Only adding Korean language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\koreanlanguage.list
	) ELSE IF /I %LANGUAGE% ==russian (
		ECHO Only adding Russian language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\russianlanguage.list
	) ELSE IF /I %LANGUAGE% ==portuguese (
		ECHO Only adding Portuguese language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\portugueselanguage.list
	) ELSE IF /I %LANGUAGE% ==italian (
		ECHO Only adding Italian language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\italianlanguage.list
	) ELSE IF /I %LANGUAGE% ==japanese (
		ECHO Only adding Japanese language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\japaneselanguage.list
	) ELSE IF /I %LANGUAGE% ==spanish (
		ECHO Only adding Spanish language stuff to PC build
		perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\spanishlanguage.list
	)
) ELSE (
	ECHO Only adding English language stuff to PC build
	perl %TOOLS%\copyfiles.pl data %BUILD%\data < %TOOLS%\englishlanguage.list
)

pushd %BUILD%\data
dir *.bmp *.dds *.tex *.txt /s/b >list
%TOOLS%\sed "s_.*data\\_data\\_" list > list2
cd ..
%TOOLS%\rar a -m5 -s -idp language.dat @data\list2
del data\list data\list2
popd
rmdir /q/s %BUILD%\data

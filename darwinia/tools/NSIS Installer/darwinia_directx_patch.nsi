!include logiclib.nsh
!include "MUI.nsh"
!include "Sections.nsh"

Name "Darwinia Patch"
OutFile "darwinia_patch.exe"
InstallDir "$PROGRAMFILES\Darwinia\"

!define MUI_HEADERIMAGE
!define MUI_ABORTWARNING
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "darwinia.bmp"
!define MUI_COMPONENTSPAGE_NODESC

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "licence.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"

;Page license
;Page components
;Page directory CheckDefaultInstall
;Page instfiles

;LicenseData "licence.txt"

InstType /COMPONENTSONLYONCUSTOM
InstType "Express"

Section "Darwinia"
 SectionIn 1 RO
 SetOutPath $INSTDIR
 
 IfFileExists "$INSTDIR\main.dat" 0 missingDarwinia
 IfFileExists "$INSTDIR\sounds.dat" 0 missingDarwinia 
 IfFileExists "$WINDIR\system32\d3dx9_32.dll" 0 missingDirectX
 IfFileExists "$WINDIR\system32\xinput1_3.dll" 0 missingDirectX

 File "..\..\build\darwinia.exe"
 File "..\..\build\patch.dat"
 File "..\..\build\language.dat"

 GoTo done
 
 missingDirectX:
	MessageBox MB_YESNO "You need to update (or install) DirectX 9c to play Darwinia. Please update (or install) DirectX and run this installer again.\	
		 	     Would you like to download DirectX now?" IDNO no
		ExecShell "open" "http://www.microsoft.com/windows/directx/default.mspx"			
	no:
		MessageBox MB_OK "Please update (or install) DirectX and try installing again." 
		Abort 
 
 missingDarwinia:
 	MessageBox MB_OK "This is a patch, and requires an existing installation of Darwinia" 
	Abort
 
 done: 
SectionEnd
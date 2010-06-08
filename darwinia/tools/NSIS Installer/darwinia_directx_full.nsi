
!include logiclib.nsh
!include "MUI.nsh"
!include "Sections.nsh"

Name "Darwinia"
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
;UninstPage uninstConfirm
;UninstPage instfiles

;LicenseData "licence.txt"

InstType /COMPONENTSONLYONCUSTOM
InstType "Express"

Section "Darwinia"
 SectionIn 1 RO
 SetOutPath $INSTDIR
 strcpy $9 0
 IfFileExists "$WINDIR\system32\d3dx9_32.dll" dllfound
	strcpy $9 1
 dllfound:
 IfFileExists "$WINDIR\system32\xinput1_3.dll" dllfound2
	strcpy $9 1
 dllfound2:
 ${If} $9 == "1"
	MessageBox MB_YESNO "You do not have the required version of DirectX to play Darwinia. Please install DirectX and install again.\ 
		 	     Would you like to download DirectX now?" IDNO no
		ExecShell "open" "http://www.microsoft.com/windows/directx/default.mspx"
	no:
		Abort "Required version of DirectX missing. Please install it and try again."
 ${EndIf}
 File "..\..\build\darwinia.exe"
 File /NONFATAL "..\..\build\main.dat"
 File "..\..\build\patch.dat"
 File /NONFATAL "..\..\build\sounds.dat"
 File /NONFATAL "..\..\build\language.dat"
 File /NONFATAL "..\..\build\manual.pdf"
 WriteUninstaller "uninstall.exe" 
 SetOutPath "$INSTDIR"

SectionEnd

Section "un.Uninstaller"
 RMDir /r "$INSTDIR\*.*"
 RMDir "$INSTDIR"
SectionEnd

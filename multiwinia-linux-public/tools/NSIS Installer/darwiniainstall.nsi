
!addplugindir ".\"
!include logiclib.nsh
!include "MUI.nsh"
!include "Sections.nsh"

Name "Darwinia"
OutFile "darwiniavistasetup.exe"
InstallDir "$PROGRAMFILES\Introversion Software\Darwinia\"

!define MUI_HEADERIMAGE
!define MUI_ABORTWARNING
!define MUI_HEADERIMAGE_BITMAP "header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "darwinia.bmp"
!define MUI_COMPONENTSPAGE_NODESC

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "licence.txt"
!insertmacro MUI_PAGE_COMPONENTS
Page directory CheckDefaultInstall
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

var custom_install

InstType /COMPONENTSONLYONCUSTOM
InstType "Express"
RequestExecutionLevel admin

Function .onInit
 !insertmacro MUI_LANGDLL_DISPLAY
 StrCpy $custom_install 0
FunctionEnd

Function un.onInit
  !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

;Function RegisterFileAssociations

; WriteRegStr HKCR "DarwiniaSaveGame" "PreviewTitle" "prop:System.Game.RichSaveName;System.Game.RichApplicationName"
;WriteRegStr HKCR "DarwiniaSaveGame" "PreviewDetails" "prop:System.Game.RichLevel;System.DateChanged;System.Game.RichComment;System.ItemNameDisplay;System.ItemType"
; WriteRegStr HKCR "DarwiniaSaveGame\Shell\Open\Command" "" "$INSTDIR\darwinia.exe"
; WriteRegStr HKCR ".dsg" "" "DarwiniaSaveGame"
; WriteRegStr HKCR ".dsg\ShellEx\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}" "" "{4E5BFBF8-F59A-4e87-9805-1F9B42CC254A}"
; WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.dsg" "" "{ECDD6472-2B9B-4b4b-AE36-F316DF3C8D60}"
; WriteRegStr HKLM "Software\Classes\.dsg\ShellEx\{BB2E617C-0920-11d1-9A0B-00C04FC2D6C1}" "" "{4E5BFBF8-F59A-4e87-9805-1F9B42CC254A}"
; WriteRegStr HKLM "Software\Classes\DarwiniaSaveGame" "PreviewTitle" "prop:System.Game.RichSaveName;System.Game.RichApplicationName"
; WriteRegStr HKLM "Software\Classes\DarwiniaSaveGame" "PreviewDetails" "prop:System.Game.RichLevel;System.DateChanged;System.Game.RichComment;System.ItemNameDisplay;System.ItemType"
; WriteRegStr HKLM "Software\Classes\DarwiniaSaveGame\Shell\Open\Command" "" "$INSTDIR\darwinia.exe"

;FunctionEnd

Function InstallDX
 File "DXInstall\*.*"
 ExecWait '"DXSETUP.exe" /silent'
 Delete "$INSTDIR\*.*"
FunctionEnd

SectionGroup /e "!Darwinia"
Section "Darwinia"
 SectionIn 1 RO
 GESetup::CheckForVista
 SetOutPath $INSTDIR
 ${If} $0 == "0"
	Messagebox MB_OK|MB_ICONSTOP "Darwinia: Vista Edition requires Windows Vista or later to install"
	Abort "Darwinia: Vista Edition requires Windows Vista or later to install"
 ${EndIf}
 strcpy $9 0
 IfFileExists "$WINDIR\system32\d3dx9_32.dll" dllfound
	strcpy $9 1
 dllfound:
 IfFileExists "$WINDIR\system32\xinput1_3.dll" dllfound2
	strcpy $9 1
 dllfound2:
 ${If} $9 == "1"
 	Call InstallDX
 ${EndIf}
 File "..\..\build\darwinia.exe"
 File "..\..\build\main.dat"
 File "..\..\build\sounds.dat"
 File /NONFATAL "..\..\build\language.dat"
 File /NONFATAL "..\..\build\manual.pdf"
 SetOutPath "$INSTDIR\Mods\Perdition"
 File /r /x "*.svn*" "..\..\Mods\Perdition\*.*"
 WriteUninstaller "uninstall.exe" 
 GESetup::GetMediaCenterPath
 CreateDirectory "$0\Media Center Programs"
 SetOutPath "$0\Media Center Programs"
 File "..\..\darwinia.mcl"
 SetOutPath "$0\Media Center Programs\Darwinia"
 File "..\..\darwinia.png"
 GESetup::CreateMediaCenterShortcut "$INSTDIR\darwinia.exe"
 SetOutPath "$INSTDIR"

SectionEnd

Section "All Users" all_users_install
 SectionIn 1
 SetShellVarContext all
 GESetup::IsUserAdmin
 ${If} $0 == "0"
	MessageBox MB_OK|MB_ICONSTOP "Installing Darwinia: Vista Edition for All Users requires Administrator access.\nPlease log in with an Administrator account or install for the Current User only."
	Abort "Administrator access required for All Users install."
 ${EndIf}
 GESetup::SetupGameExplorerRequirements "$INSTDIR\darwinia.exe" "$INSTDIR" "1"
 ${If} $0 != "0"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "InstanceID" "$0"
 ${EndIf}
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "DisplayName" "Darwinia - Introversion Software"
 WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
		 "NoModify" 1
 WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
		 "NoRepair" 1
; Call RegisterFileAssociations
SectionEnd
SectionGroupEnd

Section # CurrentUser
 SectionGetFlags ${all_users_install} $0
 IntOp $0 $0 & ${SF_SELECTED}
 ${If} $0 != ${SF_SELECTED}
  GESetup::SetupGameExplorerRequirements "$INSTDIR\darwinia.exe" "$INSTDIR" "0"
  ${If} $0 != "0"
	WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "InstanceID" "$0"
  ${EndIf}
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "DisplayName" "Darwinia - Introversion Software"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
                 "UninstallString" "$INSTDIR\uninstall.exe"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
		 "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" \
		 "NoRepair" 1
; Call RegisterFileAssociations
 ${EndIf}
SectionEnd

Section "un.Uninstaller"
 ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" "InstanceID"
 ${If} $0 == ""
	 ReadRegStr $0 HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia" "InstanceID"
 ${EndIf}
 ${IF} $0 != ""
	 GESetup::RemoveFromGameExplorer "$0"
 ${EndIf}
 RMDir /r "$INSTDIR\*.*"
 RMDir "$INSTDIR"
 GESetup::GetMediaCenterPath
 RMDir /r "$0\Media Center Programs\Darwinia\*.*"
 RMDir "$0\Media Center Programs\Darwinia"
 Delete "$0\Media Center Programs\darwinia.mcl"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia"
 DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Darwinia"
 DeleteRegKey HKCR "DarwiniaSaveGame"
 DeleteRegKey HKCR ".dsg"
 DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\PropertySystem\PropertyHandlers\.dsg"
 DeleteRegKey HKLM "Software\Classes\.dsg"
 DeleteRegKey HKLM "Software\Classes\DarwiniaSaveGame"
 

SectionEnd

Function CheckDefaultInstall
 ${If} $custom_install == 0
	Abort
 ${EndIf}
FunctionEnd

Function .onSelChange
 StrCpy $custom_install 1
FunctionEnd

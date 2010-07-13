# Microsoft Developer Studio Project File - Name="ShapeExporter" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ShapeExporter - Win32 Hybrid
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ShapeExporter.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ShapeExporter.mak" CFG="ShapeExporter - Win32 Hybrid"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ShapeExporter - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShapeExporter - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ShapeExporter - Win32 Hybrid" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ShapeExporter - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /O2 /I "..\..\code" /I "..\..\contrib\nvtristrip" /I "..\..\contrib\unrar" /I "f:\3dsmax5\maxsdk\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "EXPORTER_BUILD" /YX /FD /LD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x35e90000" /subsystem:windows /dll /machine:I386 /out:"..\binaries\ShapeExporter.dle" /libpath:"F:\3dsmax5\maxsdk\lib" /release

!ELSEIF  "$(CFG)" == "ShapeExporter - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /ZI /Od /I "C:\program files\3dsmax5\maxsdk\include" /I "../../code" /I "../../contrib/nvtristrip" /I "../../contrib/alleggl/include" /I "../../contrib/allegro/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /FR /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib opengl32.lib /nologo /base:"0x35e90000" /subsystem:windows /dll /debug /machine:I386 /out:"../binaries/ShapeExporter.dle" /pdbtype:sept /libpath:"C:\program files\3dsmax5\maxsdk\lib"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ShapeExporter - Win32 Hybrid"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "ShapeExporter___Win32_Hybrid"
# PROP BASE Intermediate_Dir "ShapeExporter___Win32_Hybrid"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "ShapeExporter___Win32_Hybrid"
# PROP Intermediate_Dir "ShapeExporter___Win32_Hybrid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MD /W3 /Gm /ZI /Od /I "..\..\contrib\unrar" /I "..\..\contrib\alleggl\include" /I "..\..\contrib\allegro\include" /I "..\..\code" /I "..\..\contrib\nvtristrip" /I "f:\3dsmax5\maxsdk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "EXPORTER_BUILD" /FR /YX /FD /LD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib core.lib geom.lib gfx.lib mesh.lib maxutil.lib maxscrpt.lib paramblk2.lib /nologo /base:"0x35e90000" /subsystem:windows /dll /debug /machine:I386 /out:"f:\3dsmax\plugins\ShapeExporter.dle" /pdbtype:sept /libpath:"F:\3dsmax5\maxsdk\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ShapeExporter - Win32 Release"
# Name "ShapeExporter - Win32 Debug"
# Name "ShapeExporter - Win32 Hybrid"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\DllEntry.cpp
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\ShapeExporter.cpp
# End Source File
# Begin Source File

SOURCE=.\ShapeExporter.def
# End Source File
# Begin Source File

SOURCE=.\ShapeExporter.h
# End Source File
# Begin Source File

SOURCE=.\ShapeExporter.rc
# End Source File
# End Group
# Begin Group "Lib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\lib\debug_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\debug_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\matrix33.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\matrix33.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\matrix34.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\matrix34.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\nvtristrip\NvTriStrip.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\rgb_colour.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\rgb_colour.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\shape.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\shape.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_stream_readers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_stream_readers.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\texture_uv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\texture_uv.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\Tosser.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\universal_include.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\Vector3.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\Vector3.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\nvtristrip\NvTriStrip.lib
# End Source File
# End Group
# End Target
# End Project

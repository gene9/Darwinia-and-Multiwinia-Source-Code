# Microsoft Developer Studio Project File - Name="Unrar" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=Unrar - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Unrar.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Unrar.mak" CFG="Unrar - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Unrar - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "Unrar - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Unrar - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /G6 /W3 /GX- /O2 /I "lib" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"unrar.lib"

!ELSEIF  "$(CFG)" == "Unrar - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"unrar.lib"

!ENDIF 

# Begin Target

# Name "Unrar - Win32 Release"
# Name "Unrar - Win32 Debug"
# Begin Source File

SOURCE=.\archive.cpp
# End Source File
# Begin Source File

SOURCE=.\archive.h
# End Source File
# Begin Source File

SOURCE=.\arcread.cpp
# End Source File
# Begin Source File

SOURCE=.\array.h
# End Source File
# Begin Source File

SOURCE=.\cmddata.cpp
# End Source File
# Begin Source File

SOURCE=.\cmddata.h
# End Source File
# Begin Source File

SOURCE=.\coder.h
# End Source File
# Begin Source File

SOURCE=.\compress.h
# End Source File
# Begin Source File

SOURCE=.\consio.cpp
# End Source File
# Begin Source File

SOURCE=.\consio.h
# End Source File
# Begin Source File

SOURCE=.\crc.cpp
# End Source File
# Begin Source File

SOURCE=.\crc.h
# End Source File
# Begin Source File

SOURCE=.\crypt.cpp
# End Source File
# Begin Source File

SOURCE=.\crypt.h
# End Source File
# Begin Source File

SOURCE=.\encname.cpp
# End Source File
# Begin Source File

SOURCE=.\encname.h
# End Source File
# Begin Source File

SOURCE=.\errhnd.cpp
# End Source File
# Begin Source File

SOURCE=.\errhnd.h
# End Source File
# Begin Source File

SOURCE=.\extinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\extinfo.h
# End Source File
# Begin Source File

SOURCE=.\extract.cpp
# End Source File
# Begin Source File

SOURCE=.\extract.h
# End Source File
# Begin Source File

SOURCE=.\filcreat.cpp
# End Source File
# Begin Source File

SOURCE=.\filcreat.h
# End Source File
# Begin Source File

SOURCE=.\file.cpp
# End Source File
# Begin Source File

SOURCE=.\file.h
# End Source File
# Begin Source File

SOURCE=.\filefn.cpp
# End Source File
# Begin Source File

SOURCE=.\filefn.h
# End Source File
# Begin Source File

SOURCE=.\filestr.cpp
# End Source File
# Begin Source File

SOURCE=.\filestr.h
# End Source File
# Begin Source File

SOURCE=.\find.cpp
# End Source File
# Begin Source File

SOURCE=.\find.h
# End Source File
# Begin Source File

SOURCE=.\getbits.cpp
# End Source File
# Begin Source File

SOURCE=.\getbits.h
# End Source File
# Begin Source File

SOURCE=.\global.cpp
# End Source File
# Begin Source File

SOURCE=.\global.h
# End Source File
# Begin Source File

SOURCE=.\headers.h
# End Source File
# Begin Source File

SOURCE=.\int64.cpp
# End Source File
# Begin Source File

SOURCE=.\int64.h
# End Source File
# Begin Source File

SOURCE=.\isnt.cpp
# End Source File
# Begin Source File

SOURCE=.\isnt.h
# End Source File
# Begin Source File

SOURCE=.\list.cpp
# End Source File
# Begin Source File

SOURCE=.\list.h
# End Source File
# Begin Source File

SOURCE=.\loclang.h
# End Source File
# Begin Source File

SOURCE=.\log.h
# End Source File
# Begin Source File

SOURCE=.\match.cpp
# End Source File
# Begin Source File

SOURCE=.\match.h
# End Source File
# Begin Source File

SOURCE=.\memfree.h
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\os.h
# End Source File
# Begin Source File

SOURCE=.\pathfn.cpp
# End Source File
# Begin Source File

SOURCE=.\pathfn.h
# End Source File
# Begin Source File

SOURCE=.\rarbloat.cpp
# End Source File
# Begin Source File

SOURCE=.\rarbloat.h
# End Source File
# Begin Source File

SOURCE=.\rardefs.h
# End Source File
# Begin Source File

SOURCE=.\rarfn.h
# End Source File
# Begin Source File

SOURCE=.\rarlang.h
# End Source File
# Begin Source File

SOURCE=.\raros.h
# End Source File
# Begin Source File

SOURCE=.\rarresource.cpp
# End Source File
# Begin Source File

SOURCE=.\rarresource.h
# End Source File
# Begin Source File

SOURCE=.\rartypes.h
# End Source File
# Begin Source File

SOURCE=.\rarvm.cpp
# End Source File
# Begin Source File

SOURCE=.\rarvm.h
# End Source File
# Begin Source File

SOURCE=.\rarvmtbl.cpp
# End Source File
# Begin Source File

SOURCE=.\rawread.cpp
# End Source File
# Begin Source File

SOURCE=.\rawread.h
# End Source File
# Begin Source File

SOURCE=.\rdwrfn.cpp
# End Source File
# Begin Source File

SOURCE=.\rdwrfn.h
# End Source File
# Begin Source File

SOURCE=.\recvol.cpp
# End Source File
# Begin Source File

SOURCE=.\recvol.h
# End Source File
# Begin Source File

SOURCE=.\rijndael.cpp
# End Source File
# Begin Source File

SOURCE=.\rijndael.h
# End Source File
# Begin Source File

SOURCE=.\rs.cpp
# End Source File
# Begin Source File

SOURCE=.\rs.h
# End Source File
# Begin Source File

SOURCE=.\savepos.cpp
# End Source File
# Begin Source File

SOURCE=.\savepos.h
# End Source File
# Begin Source File

SOURCE=.\scantree.cpp
# End Source File
# Begin Source File

SOURCE=.\scantree.h
# End Source File
# Begin Source File

SOURCE=.\sha1.cpp
# End Source File
# Begin Source File

SOURCE=.\sha1.h
# End Source File
# Begin Source File

SOURCE=.\smallfn.h
# End Source File
# Begin Source File

SOURCE=.\strfn.cpp
# End Source File
# Begin Source File

SOURCE=.\strfn.h
# End Source File
# Begin Source File

SOURCE=.\strlist.cpp
# End Source File
# Begin Source File

SOURCE=.\strlist.h
# End Source File
# Begin Source File

SOURCE=.\suballoc.h
# End Source File
# Begin Source File

SOURCE=.\system.cpp
# End Source File
# Begin Source File

SOURCE=.\system.h
# End Source File
# Begin Source File

SOURCE=.\timefn.cpp
# End Source File
# Begin Source File

SOURCE=.\timefn.h
# End Source File
# Begin Source File

SOURCE=.\ulinks.cpp
# End Source File
# Begin Source File

SOURCE=.\ulinks.h
# End Source File
# Begin Source File

SOURCE=.\unicode.cpp
# End Source File
# Begin Source File

SOURCE=.\unicode.h
# End Source File
# Begin Source File

SOURCE=.\unpack.cpp
# End Source File
# Begin Source File

SOURCE=.\unpack.h
# End Source File
# Begin Source File

SOURCE=.\unrar.cpp
# End Source File
# Begin Source File

SOURCE=.\unrar.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# Begin Source File

SOURCE=.\volume.cpp
# End Source File
# Begin Source File

SOURCE=.\volume.h
# End Source File
# End Target
# End Project

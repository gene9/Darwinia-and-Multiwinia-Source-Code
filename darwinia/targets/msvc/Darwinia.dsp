# Microsoft Developer Studio Project File - Name="Darwinia" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Darwinia - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Darwinia.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Darwinia.mak" CFG="Darwinia - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Darwinia - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Darwinia - Win32 ReleaseSafe" (based on "Win32 (x86) Application")
!MESSAGE "Darwinia - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Darwinia - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /O2 /Ob2 /I "..\..\contrib\oggvorbis\include" /I "..\..\code" /I "..\..\contrib\unrar" /I ".\\" /I "..\..\contrib\netlib" /I "..\..\contrib\eclipse" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "TARGET_MSVC" /Fr /Yu"lib/universal_include.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 msvcrt.lib oleaut32.lib shell32.lib unrar.lib vorbis_static.lib ogg_static.lib vorbisfile_static.lib eclipse.lib winmm.lib uuid.lib dxguid.lib dxerr9.lib dsound.lib netclass.lib advapi32.lib oldnames.lib vfw32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib wsock32.lib ole32.lib /nologo /subsystem:windows /machine:I386 /nodefaultlib /out:"..\..\Darwinia.exe" /libpath:"../../contrib/oggvorbis/lib" /libpath:"../../contrib/unrar" /libpath:"../../contrib/eclipse" /libpath:"../../contrib/netclass" /OPT:REF
# SUBTRACT LINK32 /pdb:none /map /debug

!ELSEIF  "$(CFG)" == "Darwinia - Win32 ReleaseSafe"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseSafe"
# PROP Intermediate_Dir "ReleaseSafe"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /G6 /MT /W3 /Ot /Oa /Ow /Og /Oi /Os /Op /Ob2 /I "..\..\contrib\oggvorbis\include" /I "..\..\contrib\steam\\" /I "..\..\code" /I "..\..\contrib\unrar" /I ".\\" /I "..\..\contrib\netlib" /I "..\..\contrib\eclipse" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "TARGET_MSVC" /Fr /Yu"lib/universal_include.h" /FD /c
# SUBTRACT CPP /Ox
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 msvcrt.lib oleaut32.lib shell32.lib unrar.lib vorbis_static.lib ogg_static.lib vorbisfile_static.lib eclipse.lib winmm.lib uuid.lib dxguid.lib dxerr9.lib dsound.lib netclass.lib advapi32.lib oldnames.lib vfw32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib wsock32.lib ole32.lib /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib /out:"..\..\Darwinia.exe" /pdbtype:sept /libpath:"../../contrib/oggvorbis/lib" /libpath:"../../contrib/unrar" /libpath:"../../contrib/eclipse" /libpath:"../../contrib/netclass" /OPT:REF
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "Darwinia - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I "..\..\contrib\oggvorbis\include" /I "..\..\contrib\steam\\" /I "..\..\code" /I "..\..\contrib\unrar" /I ".\\" /I "..\..\contrib\netlib" /I "..\..\contrib\eclipse" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "TARGET_MSVC" /FR /Yu"lib/universal_include.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib winspool.lib comdlg32.lib oleaut32.lib shell32.lib unrar.lib vorbis_static.lib ogg_static.lib vorbisfile_static.lib eclipse.lib winmm.lib uuid.lib dxguid.lib dxerr9.lib dsound.lib netclass.lib advapi32.lib oldnames.lib vfw32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib wsock32.lib ole32.lib /nologo /subsystem:windows /map /debug /machine:I386 /nodefaultlib /out:"..\..\Darwinia.exe" /pdbtype:sept /libpath:"../../contrib/oggvorbis/lib" /libpath:"../../contrib/unrar" /libpath:"../../contrib/eclipse" /libpath:"../../contrib/netclass"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "Darwinia - Win32 Release"
# Name "Darwinia - Win32 ReleaseSafe"
# Name "Darwinia - Win32 Debug"
# Begin Group "interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\interface\buildings_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\buildings_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\camera_anim_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\camera_anim_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\camera_mount_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\camera_mount_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\cheat_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\cheat_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\darwinia_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\darwinia_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\debugmenu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\debugmenu.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\demoend_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\demoend_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\drop_down_menu.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\drop_down_menu.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\editor_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\editor_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\filedialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\filedialog.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\gesture_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\gesture_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\globalworldeditor_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\globalworldeditor_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\grabber_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\grabber_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\input_field.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\input_field.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\inputlogger_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\inputlogger_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\instant_unit_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\instant_unit_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\landscape_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\landscape_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\lights_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\lights_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\mainmenus.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\mainmenus.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\message_dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\message_dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\mods_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\mods_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\network_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\network_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\pokey_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\pokey_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_graphics_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_graphics_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_keybindings_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_keybindings_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_other_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_other_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_screen_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_screen_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_sound_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\prefs_sound_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\profilewindow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\profilewindow.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\reallyquit_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\reallyquit_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\save_on_quit_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\save_on_quit_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\scrollbar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\scrollbar.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\sound_profile_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\sound_profile_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundeditor_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundeditor_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundparameter_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundparameter_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundstats_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\soundstats_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\tree_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\tree_window.h
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\userprofile_window.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\interface\userprofile_window.h
# End Source File
# End Group
# Begin Group "lib"

# PROP Default_Filter ""
# Begin Group "container templates"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\lib\2d_array.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\2d_surface_map.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\bounded_array.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\btree.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\darray.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\fast_darray.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\hash_table.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\llist.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\presized_array.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\slice_darray.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\sorting_hash_table.h
# End Source File
# End Group
# Begin Group "math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\lib\invert_matrix.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\invert_matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\math_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\math_utils.h
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

SOURCE=..\..\code\lib\plane.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\plane.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\vector2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\vector2.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\vector3.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\vector3.h
# End Source File
# End Group
# Begin Group "render"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\lib\3d_sprite.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\3d_sprite.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\bitmap.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\bitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\debug_render.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\debug_render.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\ogl_extensions.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\ogl_extensions.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\persisting_debug_render.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\persisting_debug_render.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\render_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\render_utils.h
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

SOURCE=..\..\code\lib\sphere_renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\sphere_renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\texture_uv.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\texture_uv.h
# End Source File
# End Group
# Begin Group "file"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\lib\binary_stream_readers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\binary_stream_readers.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\file_writer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\file_writer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\filesys_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\filesys_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_stream_readers.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\text_stream_readers.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\code\lib\avi_generator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\avi_generator.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\avi_player.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\avi_player.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\debug_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\debug_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\hi_res_time.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\hi_res_time.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\input.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\input.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\language_table.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\language_table.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\poster_maker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\poster_maker.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\preferences.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\preferences.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\profiler.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\profiler.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\random.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\resource.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\string_utils.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\string_utils.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\system_info.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\system_info.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\universal_include.cpp
# ADD CPP /Yc"lib/universal_include.h"
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\universal_include.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\user_info.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\user_info.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\window_manager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\window_manager.h
# End Source File
# Begin Source File

SOURCE=..\..\code\lib\window_manager_win32.h
# End Source File
# End Group
# Begin Group "network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\network\bytestream.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ClientToServer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ClientToServer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\generic.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\generic.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\networkupdate.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\networkupdate.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\Server.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\Server.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ServerToClient.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ServerToClient.h
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ServerToClientLetter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\network\ServerToClientLetter.h
# End Source File
# End Group
# Begin Group "worldobject"

# PROP Default_Filter ""
# Begin Group "Buildings"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\worldobject\anthill.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\anthill.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\blueprintstore.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\blueprintstore.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\bridge.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\bridge.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\building.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\building.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\cave.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\cave.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\constructionyard.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\constructionyard.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\controltower.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\controltower.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\factory.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\factory.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\generator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\generator.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\generichub.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\generichub.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\goddish.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\goddish.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\gunturret.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\gunturret.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\incubator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\incubator.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\laserfence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\laserfence.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\library.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\library.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\mine.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\mine.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\powerstation.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\powerstation.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\radardish.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\radardish.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\researchitem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\researchitem.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\rocket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\rocket.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\safearea.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\safearea.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\scripttrigger.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\scripttrigger.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spam.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spam.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spawnpoint.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spawnpoint.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spiritreceiver.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spiritreceiver.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spiritstore.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spiritstore.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\staticshape.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\staticshape.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\switch.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\switch.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\teleport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\teleport.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\tree.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\tree.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\triffid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\triffid.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\trunkport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\trunkport.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\upgrade_port.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\upgrade_port.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\wall.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\wall.h
# End Source File
# End Group
# Begin Group "Entities"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\worldobject\ai.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\ai.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\airstrike.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\airstrike.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\armour.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\armour.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\armyant.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\armyant.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\centipede.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\centipede.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\darwinian.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\darwinian.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\egg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\egg.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\engineer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\engineer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\entity.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\entity.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\entity_leg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\entity_leg.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\insertion_squad.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\insertion_squad.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\lander.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\lander.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\lasertrooper.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\lasertrooper.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\officer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\officer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\souldestroyer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\souldestroyer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spider.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spider.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\sporegenerator.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\sporegenerator.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\tripod.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\tripod.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\virii.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\virii.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\code\worldobject\flag.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\flag.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\snow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\snow.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spirit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\spirit.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\weapons.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\weapons.h
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\worldobject.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\worldobject\worldobject.h
# End Source File
# End Group
# Begin Group "data"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\docs\changes.txt
# End Source File
# Begin Source File

SOURCE="..\..\docs\detailed schedule.txt"
# End Source File
# Begin Source File

SOURCE=..\..\data\effects.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\language\english.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\language\french.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\game.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\game_demo2.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\game_unlockall.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\language\german.txt
# End Source File
# Begin Source File

SOURCE=..\..\docs\multiwinia.txt
# End Source File
# Begin Source File

SOURCE=..\..\docs\newdemo.txt
# End Source File
# Begin Source File

SOURCE=..\..\preferences.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\sounds.txt
# End Source File
# Begin Source File

SOURCE=..\..\docs\soundsystem.txt
# End Source File
# Begin Source File

SOURCE=..\..\data\stats.txt
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\sound\pokey.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\pokey.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sample_cache.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sample_cache.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_filter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_filter.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_instance.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_instance.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_2d.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_2d.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d_dsound.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d_dsound.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d_software.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_library_3d_software.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_parameter.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_parameter.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_stream_decoder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\sound_stream_decoder.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\soundsystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sound\soundsystem.h
# End Source File
# End Group
# Begin Group "loaders"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\code\loaders\amiga_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\amiga_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\credits_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\credits_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\egames_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\egames_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\fodder_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\fodder_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\gameoflife_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\gameoflife_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\matrix_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\matrix_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\raytrace_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\raytrace_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\soul_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\soul_loader.h
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\speccy_loader.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\loaders\speccy_loader.h
# End Source File
# End Group
# Begin Group "NetLib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\contrib\netlib\net_lib.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_lib.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_lib_win32.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_mutex.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_mutex_win32.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_socket.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_socket.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_socket_listener.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_socket_listener.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_thread.h
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_thread_win32.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_udp_packet.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=..\..\contrib\netlib\net_udp_packet.h
# End Source File
# End Group
# Begin Group "steam"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\contrib\steam\Steam.lib
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\code\3d_sierpinski_gasket.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\3d_sierpinski_gasket.h
# End Source File
# Begin Source File

SOURCE=..\..\code\app.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\app.h
# End Source File
# Begin Source File

SOURCE=..\..\code\camera.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\camera.h
# End Source File
# Begin Source File

SOURCE=..\..\code\clouds.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\clouds.h
# End Source File
# Begin Source File

SOURCE=..\..\code\control_bindings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\control_bindings.h
# End Source File
# Begin Source File

SOURCE=.\darwinia.ico
# End Source File
# Begin Source File

SOURCE=..\..\code\demoendsequence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\demoendsequence.h
# End Source File
# Begin Source File

SOURCE=..\..\code\effect_processor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\effect_processor.h
# End Source File
# Begin Source File

SOURCE=..\..\code\entity_grid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\entity_grid.h
# End Source File
# Begin Source File

SOURCE=..\..\code\explosion.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\explosion.h
# End Source File
# Begin Source File

SOURCE=..\..\code\gamecursor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\gamecursor.h
# End Source File
# Begin Source File

SOURCE=..\..\code\gesture.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\gesture.h
# End Source File
# Begin Source File

SOURCE=..\..\code\gesture_demo.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\gesture_demo.h
# End Source File
# Begin Source File

SOURCE=..\..\code\gesture_demo_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\global_internet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\global_internet.h
# End Source File
# Begin Source File

SOURCE=..\..\code\global_world.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\global_world.h
# End Source File
# Begin Source File

SOURCE=..\..\code\globals.h
# End Source File
# Begin Source File

SOURCE=..\..\code\helpsystem.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\helpsystem.h
# End Source File
# Begin Source File

SOURCE=.\icon_resource.rc
# End Source File
# Begin Source File

SOURCE=..\..\code\keygen.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\keygen.h
# End Source File
# Begin Source File

SOURCE=..\..\code\landscape.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\landscape.h
# End Source File
# Begin Source File

SOURCE=..\..\code\landscape_renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\landscape_renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\level_file.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\level_file.h
# End Source File
# Begin Source File

SOURCE=..\..\code\location.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\location.h
# End Source File
# Begin Source File

SOURCE=..\..\code\location_editor.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\location_editor.h
# End Source File
# Begin Source File

SOURCE=..\..\code\location_input.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\location_input.h
# End Source File
# Begin Source File

SOURCE=..\..\code\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\main.h
# End Source File
# Begin Source File

SOURCE=..\..\code\obstruction_grid.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\obstruction_grid.h
# End Source File
# Begin Source File

SOURCE=..\..\code\particle_system.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\particle_system.h
# End Source File
# Begin Source File

SOURCE=..\..\code\renderer.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\renderer.h
# End Source File
# Begin Source File

SOURCE=..\..\code\routing_system.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\routing_system.h
# End Source File
# Begin Source File

SOURCE=..\..\code\script.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\script.h
# End Source File
# Begin Source File

SOURCE=..\..\code\sepulveda.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\sepulveda.h
# End Source File
# Begin Source File

SOURCE=..\..\code\startsequence.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\startsequence.h
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager.h
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface.h
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface_gestures.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface_gestures.h
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface_icons.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\taskmanager_interface_icons.h
# End Source File
# Begin Source File

SOURCE=..\..\code\team.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\team.h
# End Source File
# Begin Source File

SOURCE=..\..\code\testharness.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\testharness.h
# End Source File
# Begin Source File

SOURCE=..\..\code\tutorial.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\tutorial.h
# End Source File
# Begin Source File

SOURCE=..\..\code\tutorial_demo1.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\tutorial_demo2.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\unit.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\unit.h
# End Source File
# Begin Source File

SOURCE=..\..\code\user_input.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\user_input.h
# End Source File
# Begin Source File

SOURCE=..\..\code\water.cpp
# End Source File
# Begin Source File

SOURCE=..\..\code\water.h
# End Source File
# End Target
# End Project

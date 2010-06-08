Darwinia Development Distribution
=================================

* Visual Studio 2008

The solution file is located in targets\vs2005

BUILDING

There are four possible build configurations:

	Debug						produces darwinia_d_gl.exe
	DebugDirect3D				produces darwinia_d.exe
	Release						produces darwinia_gl.exe
	ReleaseDirect3D				produces darwinia.exe
	PC OpenGL (Release)			produces darwinia_gl.exe
	PC OpenGL (ReleaseDirect3D)	produces darwinia.exe
	
Debug and Release build Darwinia to use the OpenGL engine, and
DebugDirect3D and ReleaseDirect3D build Darwinia to use the DirectX 9.0 engine.

REQUIREMENTS

* DirectX SDK
* Windows Vista SDK (Optional)

You will need the DirectX SDK installed because Darwinia uses DirectSound on Windows
for the audio. Also, if you build TARGET_OS_VISTA (see lib\universal_include.h) then you
will need to download and install the Windows Vista SDK and configure Visual Studio 2008
to use it.

RUNNING

When running Darwinia from inside Visual Studio, you should set your working directory 
(Right click on Darwinia, Properties, Debugging, Working Directory) to "..\.."
otherwise Darwinia will fail to load because it can't find the data files.

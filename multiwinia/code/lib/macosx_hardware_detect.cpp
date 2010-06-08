#include "macosx_hardware_detect.h"

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/Graphics/IOFrameBufferShared.h>
#include <IOKit/IOKitLib.h>

static bool isMacOSXSlowCPU = false;
static bool isMacOSXGraphicsNoAcceleration = false;
static bool isMacOSXGraphicsLowMemory = false;

static bool isInitialised = false;

static void GetCPUInfo()
{
	long cpuType, cpuSpeed, cpuCount;
	int error;
	
	error = Gestalt(gestaltNativeCPUtype, &cpuType);
	error = Gestalt(gestaltProcClkSpeed, &cpuSpeed);
	cpuCount = MPProcessors();

	isMacOSXSlowCPU = cpuCount <= 1 || cpuSpeed < 1024*1024*1024;
}

static void GetGraphicsInfo()
{
	/* Detecting VRAM and capabilities of graphics card */

	long gfxCardVRAM;
	CGDirectDisplayID displayID = CGMainDisplayID(); /* Set the right display */
	io_service_t displayPort = CGDisplayIOServicePort(CGMainDisplayID());
	CFTypeRef typeCode = IORegistryEntryCreateCFProperty(
		displayPort,
		CFSTR(kIOFBMemorySizeKey), 
		kCFAllocatorDefault, 
		kNilOptions);

	if (typeCode && (CFGetTypeID(typeCode) == CFNumberGetTypeID()))	{
		CFNumberGetValue(typeCode, kCFNumberSInt32Type, &gfxCardVRAM);
		CFRelease(typeCode);
	}

	isMacOSXGraphicsNoAcceleration = !CGDisplayUsesOpenGLAcceleration(displayID);
	isMacOSXGraphicsLowMemory = gfxCardVRAM <= 32 * 1024 * 1024;
}

static void GetHardwareInfo()
{
	if (isInitialised)
		return;

	GetCPUInfo();
	GetGraphicsInfo();
	
	isInitialised = true;
}

bool MacOSXSlowCPU()
{
	GetHardwareInfo();
	return isMacOSXSlowCPU;
}

bool MacOSXGraphicsNoAcceleration()
{
	GetHardwareInfo();
	return isMacOSXGraphicsNoAcceleration;
}

bool MacOSXGraphicsLowMemory()
{
	GetHardwareInfo();
	return isMacOSXGraphicsLowMemory;
}



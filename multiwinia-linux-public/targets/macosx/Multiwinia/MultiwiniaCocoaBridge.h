/* MultiwiniaCocoaBridge.h
   This is the interface from the Cocoa code into the rest of the application. */

#ifndef MULTIWINIA_CORE_H
#define MULTIWINIA_CORE_H
#import "loading_screen.h" // for LoadingScreen::IsRendering()

int  WindowManagerSDLIsFullscreen();
void WindowManagerSDLPreventFullscreenStartup();
bool WindowManagerSDLGamePaused();
bool WindowManagerSDLToggleGamePaused();
void WindowManagerSDLAwakeFromSleep();

// defined in prefs_screen_window.cpp
extern void SetWindowed(bool _isWindowed, bool _isPermanent, bool &_isSwitchingToWindowed);
#endif
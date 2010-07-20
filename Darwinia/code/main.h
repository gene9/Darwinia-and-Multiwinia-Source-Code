#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

extern double   g_gameTime;					  // Updated from GetHighResTime every frame
extern double   g_startTime;
extern float    g_advanceTime;                // How long the last frame took
extern double   g_lastServerAdvance;          // Time of last server advance
extern float    g_predictionTime;             // Time between last server advance and start of render
extern float    g_targetFrameRate;
extern int      g_lastProcessedSequenceId;
extern int		g_sliceNum;					  // Most recently advanced slice
#ifdef TARGET_OS_VISTA
extern char     g_saveFile[128];       // The profile name extracted from the save file that was used to launch darwinia
extern bool		g_mediaCenter;			// Darwinia is being run from Media Center
#endif
extern bool     IsRunningVista();

void AppMain();

#endif

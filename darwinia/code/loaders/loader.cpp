#include "lib/universal_include.h"

#include <time.h>

#include "lib/debug_utils.h"
#include "lib/input/input.h"
#include "lib/window_manager.h"
#include "lib/hi_res_time.h"
#include "lib/preferences.h"

#include "loaders/loader.h"
#include "loaders/speccy_loader.h"
#include "loaders/matrix_loader.h"
#include "loaders/fodder_loader.h"
#include "loaders/raytrace_loader.h"
#include "loaders/soul_loader.h"
#include "loaders/gameoflife_loader.h"
#include "loaders/credits_loader.h"
#include "loaders/amiga_loader.h"

#include "app.h"
#include "main.h"
#include "renderer.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_2d.h"

Loader::Loader()
{
}


Loader::~Loader()
{
}


void Loader::Run()
{
}


int Loader::SetupFor2D( int _screenW )
{
    float screenRatio = (float) g_app->m_renderer->ScreenH() / (float) g_app->m_renderer->ScreenW();
    int screenH = _screenW * screenRatio;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, _screenW, screenH, 0);
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

    return screenH;
}


void Loader::FlipBuffers()
{
	g_inputManager->PollForEvents(); 
	g_inputManager->Advance();
	glFinish();
	g_windowManager->Flip();
    glClearColor(0,0,0,1);
	glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	Sleep(1);
}


void Loader::AdvanceSound()
{
    double realTime = GetHighResTime();
	g_advanceTime = float(realTime - g_gameTime);
	g_gameTime = realTime;

    g_app->m_soundSystem->Advance();
    g_soundLibrary2d->TopupBuffer();    
}


Loader *Loader::CreateLoader( int _type )
{
#ifndef DEMOBUILD

    switch( _type )
    {
        case TypeSpeccyLoader:          return new SpeccyLoader();
        case TypeMatrixLoader:          return new MatrixLoader();
        case TypeFodderLoader:          return new FodderLoader();
        case TypeRayTraceLoader:        return new RayTraceLoader();
        case TypeSoulLoader:            return new SoulLoader();
        case TypeGameOfLifeLoader:      return new GameOfLifeLoader(false);
        case TypeGameOfLifeLoaderGlow:  return new GameOfLifeLoader(true);
        case TypeCreditsLoader:         return new CreditsLoader();
		case TypeAmigaLoader:			return new AmigaLoader();

        default:
            DarwiniaReleaseAssert( false, "Unknown Loader Type" );
    }
#endif

    // Shut the compiler up
    return NULL;
}



char *Loader::GetLoaderName( int _index )
{
    if( _index < 0 ) return NULL;
    if( _index >= NumLoaders ) return NULL;

    // TODO : These strings should be encrypted

    static char *loaderNames[] = {  
                                    "Spectrum",
                                    "Matrix",
                                    "Fodder",
                                    "Raytrace",
                                    "Soul",
                                    "GameOfLife",
                                    "GameOfLife_Glow",
                                    "Credits",
									"Amiga"
                                };

    return loaderNames[_index];
}


int Loader::GetRandomLoaderIndex()
{
    darwiniaSeedRandom( (unsigned int) time(NULL) );
    int loaderChance[NumLoaders];
    memset( loaderChance, 0, NumLoaders*sizeof(int) );

    loaderChance[TypeSpeccyLoader]          = 10;
    loaderChance[TypeMatrixLoader]          = 10;
    loaderChance[TypeFodderLoader]          = 10;
    loaderChance[TypeRayTraceLoader]        = 10;
    loaderChance[TypeSoulLoader]            = 10;
    loaderChance[TypeGameOfLifeLoader]      = 10;
    loaderChance[TypeGameOfLifeLoaderGlow]  = 5;
	loaderChance[TypeAmigaLoader]			= 10;

    int totalChance = 0;
    for( int i = 0; i < NumLoaders; ++i )
	{
        totalChance += loaderChance[i];
	}

    int chosenChance = darwiniaRandom() % totalChance;

    totalChance = 0;
    for( int i = 0; i < NumLoaders; ++i )
    {
        int thisChance = loaderChance[i];
        if( chosenChance >= totalChance && 
            chosenChance < totalChance + thisChance )
        {
            return i;
        }
        totalChance += thisChance;
    }

    return -1;
}


int Loader::GetLoaderIndex( char *_name )
{    

    if( stricmp( _name, "random" ) == 0 )
    {
        return GetRandomLoaderIndex();
    }

    for( int i = 0; i < NumLoaders; ++i )
    {
        char *loaderName = GetLoaderName(i);
        if( stricmp( loaderName, _name ) == 0 )
        {
            return i;
        }
    }

    return -1;
}

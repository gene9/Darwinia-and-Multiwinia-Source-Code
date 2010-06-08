#include "lib/universal_include.h"
#include "lib/input/input.h"
#include "lib/window_manager.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "sound/soundsystem.h"

#include "loaders/fodder_loader.h"

#include "app.h"


FodderLoader::FodderLoader()
:   Loader()
{
}


FodderLoader::~FodderLoader()
{
}


void FodderLoader::Run()
{
    float startTime = GetHighResTime();

    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderFodder", SoundSourceBlueprint::TypeMusic );

    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        int screenH = SetupFor2D( 800 );

        float alpha = (GetHighResTime() - startTime) / 5.0f;
        alpha = max( 0.0f, alpha );
        alpha = min( 1.0f, alpha );

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );

        g_gameFont.DrawText2DCentre( 400, 215, 35, LANGUAGEPHRASE("bootloader_fodder_1") );
		g_gameFont.DrawText2DCentre( 400, 265, 35, LANGUAGEPHRASE("bootloader_fodder_2") );
        g_gameFont.DrawText2DCentre( 400, 320, 35, LANGUAGEPHRASE("bootloader_fodder_3") );

        FlipBuffers();
        AdvanceSound();

        if( !g_app->m_soundSystem->m_music ) break;
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderFodder" );
}


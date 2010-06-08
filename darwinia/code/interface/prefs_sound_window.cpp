#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/system_info.h"
#include "lib/language_table.h"

#include "interface/prefs_sound_window.h"
#include "interface/drop_down_menu.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_3d.h"
#include "sound/sample_cache.h"

#include "app.h"
#include "renderer.h"


#define SOUND_LIBRARY       "SoundLibrary"
#define SOUND_MIXFREQ       "SoundMixFreq"
#define SOUND_CHANNELS      "SoundChannels"
#define SOUND_HW3D          "SoundHW3D"
#define SOUND_SWAPSTEREO    "SoundSwapStereo"
#define SOUND_DSPEFFECTS    "SoundDSP"
#define SOUND_MEMORY        "SoundMemoryUsage"



class RestartSoundButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        PrefsSoundWindow *parent = (PrefsSoundWindow *) m_parent;

        int oldMemoryUsage = g_prefsManager->GetInt( SOUND_MEMORY );

        g_prefsManager->SetInt( SOUND_MIXFREQ, parent->m_mixFreq );
        g_prefsManager->SetInt( SOUND_CHANNELS, parent->m_numChannels );
        g_prefsManager->SetInt( SOUND_HW3D, parent->m_useHardware3D );
        g_prefsManager->SetInt( SOUND_SWAPSTEREO, parent->m_swapStereo );
        g_prefsManager->SetInt( SOUND_DSPEFFECTS, parent->m_dspEffects );
        g_prefsManager->SetInt( SOUND_MEMORY, parent->m_memoryUsage );

        if( parent->m_soundLib == 0 ) g_prefsManager->SetString( SOUND_LIBRARY, "software" );
        else                          g_prefsManager->SetString( SOUND_LIBRARY, "dsound" );

        g_app->m_soundSystem->RestartSoundLibrary();

        if( parent->m_memoryUsage != oldMemoryUsage )
        {
            //
            // Clear out the sample cache

            g_cachedSampleManager.EmptyCache();

            for( int i = 0; i < g_app->m_soundSystem->m_sounds.Size(); ++i )
            {
                if( g_app->m_soundSystem->m_sounds.ValidIndex(i) )
                {
                    SoundInstance *instance = g_app->m_soundSystem->m_sounds[i];
                    if( instance->m_cachedSampleHandle )
                    {
                        g_deletingCachedSampleHandle = true;
                        delete instance->m_cachedSampleHandle;
                        instance->m_cachedSampleHandle = NULL;
                        g_deletingCachedSampleHandle = false;
                        instance->OpenStream(false);
                    }
                }
            }

            if( g_app->m_soundSystem->m_music &&
                g_app->m_soundSystem->m_music->m_cachedSampleHandle )
            {
                g_deletingCachedSampleHandle = true;
                delete g_app->m_soundSystem->m_music->m_cachedSampleHandle;
                g_app->m_soundSystem->m_music->m_cachedSampleHandle = NULL;
                g_deletingCachedSampleHandle = false;
                g_app->m_soundSystem->m_music->OpenStream(false);
            }
        }


        if( parent->m_dspEffects )
        {
            g_app->m_soundSystem->PropagateBlueprints();
            // Causes all sounds to reload their DSP effects from the blueprints
        }
        else
        {
            g_app->m_soundSystem->StopAllDSPEffects();
        }

		g_prefsManager->Save();
    }
};



class HW3DDropDownMenu : public DropDownMenu
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        bool available = g_soundLibrary3d->Hardware3DSupport();
        if( available )
        {
            DropDownMenu::Render( realX, realY, highlighted, clicked );
        }
        else
        {
			DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
            g_editorFont.DrawText2D( realX+10, realY+9, parent->GetMenuSize(13), LANGUAGEPHRASE("dialog_unavailable") );
        }
    }

    void MouseUp()
    {
        bool available = g_soundLibrary3d->Hardware3DSupport();
        if( available )
        {
            DropDownMenu::MouseUp();
        }
    }
};


PrefsSoundWindow::PrefsSoundWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_soundoptions") )
{
    SetMenuSize( 532, 390 );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2,
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );

    m_mixFreq       = g_prefsManager->GetInt( SOUND_MIXFREQ, 22050 );
    m_numChannels   = g_prefsManager->GetInt( SOUND_CHANNELS, 16 );
    m_useHardware3D = g_prefsManager->GetInt( SOUND_HW3D, 0 );
    m_swapStereo    = g_prefsManager->GetInt( SOUND_SWAPSTEREO, 0 );
    m_dspEffects    = g_prefsManager->GetInt( SOUND_DSPEFFECTS, 1 );
    m_memoryUsage   = g_prefsManager->GetInt( SOUND_MEMORY, 1 );

    char *soundLib  = g_prefsManager->GetString( SOUND_LIBRARY );

    if( stricmp( soundLib, "dsound" ) == 0 ) m_soundLib = 1;
    else                                     m_soundLib = 0;
}


void PrefsSoundWindow::Create()
{
    DarwiniaWindow::Create();

    /*int x = GetMenuSize(150);
    int w = GetMenuSize(170);
    int y = GetMenuSize(30);
    int h = GetMenuSize(30);
	int fontSize = GetMenuSize(13);*/

	int y = GetClientRectY1() + GetMenuSize(30);
	int border = GetClientRectX1() + 10;
	int x = m_w/2;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w/2 - border * 2;
	int h = buttonH + border;
	int fontSize = GetMenuSize(13);

    InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y+=border, m_w - 20, GetClientRectY2() - h*3 - y );
    RegisterButton( box );

    DropDownMenu *soundLib = new DropDownMenu();
    soundLib->SetShortProperties( LANGUAGEPHRASE("dialog_soundlibrary"), x, y+=border, buttonW, buttonH );
#ifdef HAVE_DSOUND
    soundLib->AddOption( LANGUAGEPHRASE("dialog_directsound"), 1 );
#else
    soundLib->AddOption( LANGUAGEPHRASE("dialog_softwaresound"), 0 );
#endif
    soundLib->RegisterInt( &m_soundLib );
	soundLib->m_fontSize = fontSize;
    RegisterButton( soundLib );
	m_buttonOrder.PutData( soundLib );

    DropDownMenu *mixFreq = new DropDownMenu();
    mixFreq->SetShortProperties( LANGUAGEPHRASE("dialog_mixfrequency"), x, y+=h, buttonW, buttonH );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_11khz"), 11025 );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_22khz"), 22050 );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_44khz"), 44100 );
    mixFreq->RegisterInt( &m_mixFreq );
	mixFreq->m_fontSize = fontSize;
    RegisterButton( mixFreq );
	m_buttonOrder.PutData( mixFreq );

    DropDownMenu *numChannels = new DropDownMenu();
    numChannels->SetShortProperties( LANGUAGEPHRASE("dialog_numchannels"), x, y+=h, buttonW, buttonH );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_8channels"), 8 );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_16channels"), 16 );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_32channels"), 32 );
//#if !(defined (TARGET_OS_MACOSX) || defined(TARGET_OS_LINUX))
//	// Number of channels must be a power of 2 for the
//	// Software 3d library
//    numChannels->AddOption( "48 Channels", 48 );
//#endif
    numChannels->AddOption( LANGUAGEPHRASE("dialog_64channels"), 64 );
    numChannels->RegisterInt( &m_numChannels );
	numChannels->m_fontSize = fontSize;
    RegisterButton( numChannels );
	m_buttonOrder.PutData( numChannels );

    DropDownMenu *memoryUsage = new DropDownMenu();
    memoryUsage->SetShortProperties( LANGUAGEPHRASE("dialog_memoryusage"), x, y+=h, buttonW, buttonH );
    memoryUsage->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    memoryUsage->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    memoryUsage->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    memoryUsage->RegisterInt( &m_memoryUsage );
	memoryUsage->m_fontSize = fontSize;
    RegisterButton( memoryUsage );
	m_buttonOrder.PutData( memoryUsage );

    DropDownMenu *swapStereo = new DropDownMenu();
    swapStereo->SetShortProperties( LANGUAGEPHRASE("dialog_swapstereo"), x, y+=h, buttonW, buttonH );
    swapStereo->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    swapStereo->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    swapStereo->RegisterInt( &m_swapStereo );
	swapStereo->m_fontSize = fontSize;
    RegisterButton( swapStereo );
	m_buttonOrder.PutData( swapStereo );

    HW3DDropDownMenu *hw3d = new HW3DDropDownMenu();
    hw3d->SetShortProperties( LANGUAGEPHRASE("dialog_hw3dsound"), x, y+=h, buttonW, buttonH );
    hw3d->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    hw3d->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    hw3d->RegisterInt( &m_useHardware3D );
	hw3d->m_fontSize = fontSize;
    RegisterButton( hw3d );
	m_buttonOrder.PutData( hw3d );

    DropDownMenu *dspEffects = new DropDownMenu();
    dspEffects->SetShortProperties( LANGUAGEPHRASE("dialog_realtimeeffects"), x, y+=h, buttonW, buttonH );
    dspEffects->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    dspEffects->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    dspEffects->RegisterInt( &m_dspEffects );
    dspEffects->m_fontSize = fontSize;
    RegisterButton( dspEffects );
	m_buttonOrder.PutData( dspEffects );

	y = m_h - h;

    CloseButton *cancel = new CloseButton();
    cancel->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, y, buttonW, buttonH );
    cancel->m_fontSize = fontSize;
    cancel->m_centered = true;
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );

    RestartSoundButton *apply = new RestartSoundButton();
    apply->SetShortProperties( LANGUAGEPHRASE("dialog_apply"), m_w - buttonW - border, y, buttonW, buttonH );
    apply->m_fontSize = fontSize;
    apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );
}


void PrefsSoundWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

	unsigned int deviceId = g_systemInfo->m_audioInfo.m_preferredDevice;
    char const *hwDescription = g_systemInfo->m_audioInfo.m_deviceNames[deviceId];
    float fontSize = 1.2f * m_w / strlen(hwDescription);
    g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + GetMenuSize(30), fontSize, hwDescription );

	int border = GetClientRectX1() + 10;
    int x = m_x + 20;
    int y = m_y + GetClientRectY1() + border * 2 + GetMenuSize(30);
    int h = GetMenuSize(20) + border;
    int size = GetMenuSize(13);

    g_editorFont.DrawText2D( x, y+=border, size, LANGUAGEPHRASE("dialog_soundlibrary") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_mixfrequency") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_numchannels") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_memoryusage") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_swapstereo") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_hw3dsound") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_realtimeeffects") );


//    int numChannels = g_soundLibrary3d->m_numChannels;
//    g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - 70, 12, "%d channels allocated", numChannels );

#ifdef PROFILER_ENABLED
    ProfiledElement *element = g_app->m_profiler->m_rootElement->m_children.GetData( "Advance SoundSystem" );
    if( element->m_lastNumCalls > 0 )
    {
        float occup = element->m_lastTotalTime * 100;
        if( occup > 15 ) glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
        g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(50), GetMenuSize(17), "%s %d%%", LANGUAGEPHRASE("dialog_cpuusage"), int(occup) );
    }
    else
    {
        glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
        g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(50), size, LANGUAGEPHRASE("dialog_cpuusageunknown") );
    }
#endif

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    float memoryUsage = g_cachedSampleManager.GetMemoryUsage();
    memoryUsage /= 1024.0f;
    memoryUsage /= 1024.0f;
    g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(70), size, "%s %2.1f Mb", LANGUAGEPHRASE("dialog_memoryusage"), memoryUsage );
}


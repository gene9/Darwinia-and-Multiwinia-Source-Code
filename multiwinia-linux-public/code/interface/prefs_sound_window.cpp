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
#include "game_menu.h"


#define SOUND_LIBRARY       "SoundLibrary"
#define SOUND_MIXFREQ       "SoundMixFreq"
#define SOUND_CHANNELS      "SoundChannels"
#define SOUND_HW3D          "SoundHW3D"
#define SOUND_SWAPSTEREO    "SoundSwapStereo"
#define SOUND_DSPEFFECTS    "SoundDSP"
#define SOUND_MEMORY        "SoundMemoryUsage"



class RestartSoundButton : public GameMenuButton
{
public:

    RestartSoundButton()
    :   GameMenuButton( "dialog_apply" )
    {
    }
    void MouseUp()
    { 
        if( m_inactive ) return;

        PrefsSoundWindow *parent = (PrefsSoundWindow *) m_parent;

#ifdef USE_SOUND_MEMORY
        int oldMemoryUsage = g_prefsManager->GetInt( SOUND_MEMORY );
#endif

        g_prefsManager->SetInt( SOUND_MIXFREQ, parent->m_mixFreq );
        g_prefsManager->SetInt( SOUND_CHANNELS, parent->m_numChannels );
#ifdef USE_SOUND_HW3D
        g_prefsManager->SetInt( SOUND_HW3D, parent->m_useHardware3D );
#endif
        g_prefsManager->SetInt( SOUND_SWAPSTEREO, parent->m_swapStereo );
#ifdef USE_SOUND_DSPEFFECTS
        g_prefsManager->SetInt( SOUND_DSPEFFECTS, parent->m_dspEffects );
#endif
#ifdef USE_SOUND_MEMORY
        g_prefsManager->SetInt( SOUND_MEMORY, parent->m_memoryUsage );
#endif

        if( parent->m_soundLib == 0 ) g_prefsManager->SetString( SOUND_LIBRARY, "software" );
        else                          g_prefsManager->SetString( SOUND_LIBRARY, "dsound" );
        
        g_app->m_soundSystem->RestartSoundLibrary();

#ifdef USE_SOUND_MEMORY
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
                        // Taken care of in g_cachedSampleManager.EmptyCache();
                        //delete instance->m_cachedSampleHandle;
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
                // Taken care of in g_cachedSampleManager.EmptyCache();
                //delete g_app->m_soundSystem->m_music->m_cachedSampleHandle;
                g_app->m_soundSystem->m_music->m_cachedSampleHandle = NULL;
                g_deletingCachedSampleHandle = false;
                g_app->m_soundSystem->m_music->OpenStream(false);
            }
        }
#endif


#ifdef USE_SOUND_DSPEFFECTS
        if( parent->m_dspEffects )
#else
        if( g_prefsManager->GetInt( SOUND_DSPEFFECTS, 0 ) )
#endif
        {
            g_app->m_soundSystem->PropagateBlueprints();
            // Causes all sounds to reload their DSP effects from the blueprints
        }
        else
        {
            g_app->m_soundSystem->StopAllDSPEffects();
        }

		g_prefsManager->Save();

		parent->RefreshValues();
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        PrefsSoundWindow *parent = (PrefsSoundWindow *) m_parent;

        bool soundLibMatch = false;

        if( parent->m_soundLib == 0 )
        {
            const char *lib = g_prefsManager->GetString( SOUND_LIBRARY, NULL );
            if( !lib ||
                strcmp( lib, "software" ) == 0 )
            {
                soundLibMatch = true;
            }
        }
        else
        {
            const char *lib = g_prefsManager->GetString( SOUND_LIBRARY, NULL );
            if( !lib ||
                strlen(lib) == 0 ||
                strcmp( lib, "dsound" ) == 0 )
            {
                soundLibMatch = true;
            }
        }

        int mixFreq = g_prefsManager->GetInt( SOUND_MIXFREQ );
        if( parent->m_mixFreq == g_prefsManager->GetInt( SOUND_MIXFREQ, 22050 ) &&
            parent->m_numChannels == g_prefsManager->GetInt( SOUND_CHANNELS, 16 ) &&
#ifdef USE_SOUND_HW3D
            parent->m_useHardware3D == g_prefsManager->GetInt( SOUND_HW3D, 0 ) &&
#endif
            parent->m_swapStereo  == g_prefsManager->GetInt( SOUND_SWAPSTEREO, 0 ) &&
#ifdef USE_SOUND_DSPEFFECTS
            parent->m_dspEffects == g_prefsManager->GetInt( SOUND_DSPEFFECTS, 0 ) &&
#endif
#ifdef USE_SOUND_MEMORY
            parent->m_memoryUsage == g_prefsManager->GetInt( SOUND_MEMORY, 1 ) &&
#endif
            soundLibMatch )
        {
            m_inactive = true;
        }
        else
        {
            m_inactive = false;
        }

        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }
};


#ifdef USE_SOUND_HW3D
class HW3DDropDownMenu : public GameMenuCheckBox
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        bool available = g_app->m_soundSystem->IsInitialized() && g_soundLibrary3d->Hardware3DSupport();
        if( available )
        {
            GameMenuCheckBox::Render( realX, realY, highlighted, clicked );
        }
        else
        {
            float texY = realY + m_h/2 - m_fontSize/4;
            glColor4f( 1.0, 1.0, 1.0, 0.0 );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2D( realX+m_w*0.05, texY, m_fontSize, LANGUAGEPHRASE(m_name) );
            glColor4f( 0.3, 0.3, 0.3, 1.0 );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2D( realX+m_w*0.05, texY, m_fontSize, LANGUAGEPHRASE(m_name) );

			DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
            g_titleFont.DrawText2DRight( (realX+m_w) - m_w*0.05, texY, m_fontSize / 2.0, LANGUAGEPHRASE("dialog_unavailable") );
        }
    }

    void MouseUp()
    {
        bool available = g_app->m_soundSystem->IsInitialized() && g_soundLibrary3d->Hardware3DSupport();
        if( available )
        {
            GameMenuCheckBox::MouseUp();
        }
    }
};
#endif


PrefsSoundWindow::PrefsSoundWindow()
:   GameOptionsWindow( "dialog_soundoptions" ),
    m_menuSoundLib(NULL),
    m_menuMixFreq(NULL),
    m_menuNumChannels(NULL),
#ifdef USE_SOUND_MEMORY
    m_menuMemoryUsage(NULL),
#endif
#ifdef USE_SOUND_HW3D
    m_menuHw3d(NULL),
#endif
#ifdef USE_SOUND_DSPEFFECTS
    m_menuDspEffects(NULL),
#endif
    m_menuSwapStereo(NULL)
{

    /*SetMenuSize( 380, 390 );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );*/

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();
    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    ReadValues();
}


void PrefsSoundWindow::ReadValues()
{
    m_mixFreq       = g_prefsManager->GetInt( SOUND_MIXFREQ, 22050 );
    m_numChannels   = g_prefsManager->GetInt( SOUND_CHANNELS, 16 );
#ifdef USE_SOUND_HW3D
    m_useHardware3D = g_prefsManager->GetInt( SOUND_HW3D, 0 );
#endif
    m_swapStereo    = g_prefsManager->GetInt( SOUND_SWAPSTEREO, 0 );
#ifdef USE_SOUND_DSPEFFECTS
    m_dspEffects    = g_prefsManager->GetInt( SOUND_DSPEFFECTS, 0 );
#endif
#ifdef USE_SOUND_MEMORY
    m_memoryUsage   = g_prefsManager->GetInt( SOUND_MEMORY, 1 );
#endif

#ifdef TARGET_OS_MACOSX 
	const char *defaultSoundLib = "software";
#else
	const char *defaultSoundLib = "dsound";
#endif

    const char *soundLib  = g_prefsManager->GetString( SOUND_LIBRARY, defaultSoundLib );
    
    if( soundLib && stricmp( soundLib, "dsound" ) == 0 ) m_soundLib = 1;
    else                                     m_soundLib = 0;
}


void PrefsSoundWindow::Create()
{
//    DarwiniaWindow::Create();
    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0;
    float y = leftY;
    float fontSize = fontSmall;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    buttonH *= 0.65f;
    float border = gap + buttonH;
    
    /*InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y+=border, m_w - 20, GetClientRectY2() - h*3 - y );        
    RegisterButton( box );*/

    m_menuSoundLib = new GameMenuDropDown();
    m_menuSoundLib->SetShortProperties( "dialog_soundlibrary", x, y+=buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_soundlibrary") );    
#ifdef HAVE_DSOUND
    m_menuSoundLib->AddOption( "dialog_directsound", 1 );
#else
    m_menuSoundLib->AddOption( "dialog_softwaresound", 0 );
#endif
    m_menuSoundLib->RegisterInt( &m_soundLib );
	m_menuSoundLib->m_fontSize = fontSize;
    RegisterButton( m_menuSoundLib );
	m_buttonOrder.PutData( m_menuSoundLib );

    m_menuMixFreq = new GameMenuDropDown();
    m_menuMixFreq->SetShortProperties( "dialog_mixfrequency", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_mixfrequency") );
    m_menuMixFreq->AddOption( "dialog_11khz", 11025 );
    m_menuMixFreq->AddOption( "dialog_22khz", 22050 );
    m_menuMixFreq->AddOption( "dialog_44khz", 44100 );
    m_menuMixFreq->RegisterInt( &m_mixFreq );
	m_menuMixFreq->m_fontSize = fontSize;
    m_menuMixFreq->m_cycleMode = true;
    RegisterButton( m_menuMixFreq );
	m_buttonOrder.PutData( m_menuMixFreq );

	m_menuVolume = new VolumeControl();
	m_menuVolume->SetShortProperties( "volume", x, y+=border, buttonW, buttonH, "Master Volume" );
	m_menuVolume->CalcDimensions();
	m_menuVolume->m_fontSize = fontSize;
	m_menuVolume->m_centered = true;
	RegisterButton( m_menuVolume );
	m_buttonOrder.PutData( m_menuVolume );

    m_menuNumChannels = new GameMenuDropDown();
    m_menuNumChannels->SetShortProperties( "dialog_numchannels", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_numchannels") );
    m_menuNumChannels->AddOption( "dialog_8channels", 8 );
    m_menuNumChannels->AddOption( "dialog_16channels", 16 );
    m_menuNumChannels->AddOption( "dialog_32channels", 32 );
//#if !(defined (TARGET_OS_MACOSX) || defined(TARGET_OS_LINUX))
//	// Number of channels must be a power of 2 for the 
//	// Software 3d library
//    m_menuNumChannels->AddOption( "48 Channels", 48 );
//#endif
    m_menuNumChannels->AddOption( "dialog_64channels", 64 );
    m_menuNumChannels->RegisterInt( &m_numChannels );
	m_menuNumChannels->m_fontSize = fontSize;
    m_menuNumChannels->m_cycleMode = true;
    RegisterButton( m_menuNumChannels );
	m_buttonOrder.PutData( m_menuNumChannels );

#ifdef USE_SOUND_MEMORY
    m_menuMemoryUsage = new GameMenuDropDown();
    m_menuMemoryUsage->SetShortProperties( "dialog_memoryusage", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_memoryusage") );
    m_menuMemoryUsage->AddOption( "dialog_high", 1 );
    m_menuMemoryUsage->AddOption( "dialog_medium", 2 );
    m_menuMemoryUsage->AddOption( "dialog_low", 3 );
    m_menuMemoryUsage->RegisterInt( &m_memoryUsage );
	m_menuMemoryUsage->m_fontSize = fontSize;
    m_menuMemoryUsage->m_cycleMode = true;
    RegisterButton( m_menuMemoryUsage );
	m_buttonOrder.PutData( m_menuMemoryUsage );
#endif

    m_menuSwapStereo = new GameMenuCheckBox();
    m_menuSwapStereo->SetShortProperties( "dialog_swapstereo", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_swapstereo") );
    m_menuSwapStereo->RegisterInt( &m_swapStereo );
	m_menuSwapStereo->m_fontSize = fontSize;
    RegisterButton( m_menuSwapStereo );
	m_buttonOrder.PutData( m_menuSwapStereo );

#ifdef USE_SOUND_HW3D
    m_menuHw3d = new HW3DDropDownMenu();
    m_menuHw3d->SetShortProperties( "dialog_hw3dsound", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_hw3dsound") );
    m_menuHw3d->RegisterInt( &m_useHardware3D );
	m_menuHw3d->m_fontSize = fontSize;
    RegisterButton( m_menuHw3d );
	m_buttonOrder.PutData( m_menuHw3d );
#endif

#ifdef USE_SOUND_DSPEFFECTS
    m_menuDspEffects = new GameMenuCheckBox();
    m_menuDspEffects->SetShortProperties( "dialog_realtimeeffects", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_realtimeeffects") );
    m_menuDspEffects->RegisterInt( &m_dspEffects );
    m_menuDspEffects->m_fontSize = fontSize;	
    RegisterButton( m_menuDspEffects );
	m_buttonOrder.PutData( m_menuDspEffects );
#endif

    buttonH /= 0.65f;
    int buttonY = leftY + leftH - buttonH * 3;

    RestartSoundButton *apply = new RestartSoundButton();
    apply->SetShortProperties( "dialog_apply", x, buttonY, buttonW, buttonH, LANGUAGEPHRASE("dialog_apply") );
    apply->m_fontSize = fontMed;
    //apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );

    MenuCloseButton *cancel = new MenuCloseButton("dialog_close");
    cancel->SetShortProperties( "dialog_close", x, buttonY + buttonH + 5, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    cancel->m_fontSize = fontMed;
    //cancel->m_centered = true;    
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );
	m_backButton = cancel;
}


void PrefsSoundWindow::RefreshValues()
{
    ReadValues();

    if( m_menuSoundLib ) m_menuSoundLib->SelectOption( m_soundLib );
    if( m_menuMixFreq ) m_menuMixFreq->SelectOption( m_mixFreq );
    if( m_menuNumChannels ) m_menuNumChannels->SelectOption( m_numChannels );
#ifdef USE_SOUND_MEMORY
    if( m_menuMemoryUsage ) m_menuMemoryUsage->SelectOption( m_memoryUsage );
#endif
    if( m_menuSwapStereo ) m_menuSwapStereo->Set( m_swapStereo );
#ifdef USE_SOUND_HW3D
    if( m_menuHw3d ) m_menuHw3d->Set( m_useHardware3D );
#endif
#ifdef USE_SOUND_DSPEFFECTS
    if( m_menuDspEffects ) m_menuDspEffects->Set( m_dspEffects );
#endif
}


void PrefsSoundWindow::Render( bool _hasFocus )
{
    GameOptionsWindow::Render( _hasFocus );
    return;

    DarwiniaWindow::Render( _hasFocus );
    
	unsigned int deviceId = g_systemInfo->m_audioInfo.m_preferredDevice;
    char const *hwDescription = g_systemInfo->m_audioInfo.m_deviceNames[deviceId];
    float fontSize = 1.2 * m_w / strlen(hwDescription);
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

//    if( g_app->m_soundSystem->IsInitialized() )
//    {
//        int numChannels = g_soundLibrary3d->m_numChannels;
//        g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - 70, 12, "%d channels allocated", numChannels );
//    }

    if( g_profiler )
    {
        ProfiledElement *element = g_profiler->m_rootElement->m_children.GetData( "Advance SoundSystem" );
        if( element->m_lastNumCalls > 0 )
        {
            float occup = element->m_lastTotalTime * 100;
            if( occup > 15 ) glColor4f( 1.0, 0.3, 0.3, 1.0 );
			wchar_t temp[512];
			swprintf(temp, sizeof(temp)/sizeof(wchar_t), L"%ls %d%%", LANGUAGEPHRASE("dialog_cpuusage").m_unicodestring, int(occup));
            g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(50), GetMenuSize(17), UnicodeString(temp)  );
        }
        else
        {
            glColor4f( 1.0, 0.3, 0.3, 1.0 );
            g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(50), size, LANGUAGEPHRASE("dialog_cpuusageunknown") );
        }
    }

    glColor4f( 1.0, 1.0, 1.0, 1.0 );

	unsigned totalMemoryUsage, unusedMemoryUsage;
	g_cachedSampleManager.GetMemoryUsage( totalMemoryUsage, unusedMemoryUsage );

	const float MB = 1024.0f * 1024.0f;
	
	wchar_t tempString[512];
	swprintf(tempString, sizeof(tempString)/sizeof(wchar_t), L"%ls %2.1f Mb (unused %2.1f)", 
		LANGUAGEPHRASE("dialog_memoryusage").m_unicodestring, 
		totalMemoryUsage / MB,
		unusedMemoryUsage / MB );

    g_editorFont.DrawText2DCentre( m_x + m_w/2, m_y + m_h - GetMenuSize(70), size, UnicodeString(tempString) );
}







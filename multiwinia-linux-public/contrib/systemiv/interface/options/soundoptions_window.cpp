#include "lib/universal_include.h"
#include "lib/sound/sound_library_3d.h"
#include "lib/sound/sound_sample_bank.h"
#include "lib/sound/soundsystem.h"
#include "lib/render/renderer.h"
#include "lib/language_table.h"
#include "lib/preferences.h"
#include "lib/profiler.h"

#include "interface/components/drop_down_menu.h"
#include "interface/components/inputfield.h"

#include "soundoptions_window.h"



class RestartSoundButton : public InterfaceButton
{
public:
    void MouseUp()
    { 
        SoundOptionsWindow *parent = (SoundOptionsWindow *) m_parent;

        int oldMemoryUsage = g_preferences->GetInt( PREFS_SOUND_MEMORY );

        g_preferences->SetInt( PREFS_SOUND_MIXFREQ, parent->m_mixFreq );
        g_preferences->SetInt( PREFS_SOUND_CHANNELS, parent->m_numChannels );
        g_preferences->SetInt( PREFS_SOUND_HW3D, parent->m_useHardware3D );
        g_preferences->SetInt( PREFS_SOUND_SWAPSTEREO, parent->m_swapStereo );
        g_preferences->SetInt( PREFS_SOUND_DSPEFFECTS, parent->m_dspEffects );
        g_preferences->SetInt( PREFS_SOUND_MEMORY, parent->m_memoryUsage );
        g_preferences->SetInt( PREFS_SOUND_MASTERVOLUME, parent->m_masterVolume );

        if( parent->m_soundLib == 0 ) g_preferences->SetString( PREFS_SOUND_LIBRARY, "software" );
        else                          g_preferences->SetString( PREFS_SOUND_LIBRARY, "dsound" );
        
        g_soundSystem->RestartSoundLibrary();

        if( parent->m_memoryUsage != oldMemoryUsage )
        {
            //
            // Clear out the sample cache

            g_soundSampleBank->EmptyCache();

            for( int i = 0; i < g_soundSystem->m_sounds.Size(); ++i )    
            {
                if( g_soundSystem->m_sounds.ValidIndex(i) )
                {
                    SoundInstance *instance = g_soundSystem->m_sounds[i];
                    if( instance->m_soundSampleHandle )
                    {
                        delete instance->m_soundSampleHandle;
                        instance->m_soundSampleHandle = NULL;
                        instance->OpenStream(false);
                    }
                }
            }
        }


        if( parent->m_dspEffects )
        {
            g_soundSystem->PropagateBlueprints();
            // Causes all sounds to reload their DSP effects from the blueprints
        }
        else
        {
            g_soundSystem->StopAllDSPEffects();
        }

		g_preferences->Save();

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
            g_renderer->Text( realX+10, realY+9, White, 13, LANGUAGEPHRASE("dialog_unavailable") );
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


SoundOptionsWindow::SoundOptionsWindow()
:   InterfaceWindow( "Sound", "dialog_soundoptions", true )
{
    SetSize( 340, 350 );

    m_mixFreq       = g_preferences->GetInt( PREFS_SOUND_MIXFREQ, 22050 );
    m_numChannels   = g_preferences->GetInt( PREFS_SOUND_CHANNELS, 16 );
    m_useHardware3D = g_preferences->GetInt( PREFS_SOUND_HW3D, 0 );
    m_swapStereo    = g_preferences->GetInt( PREFS_SOUND_SWAPSTEREO, 0 );
    m_dspEffects    = g_preferences->GetInt( PREFS_SOUND_DSPEFFECTS, 1 );
    m_memoryUsage   = g_preferences->GetInt( PREFS_SOUND_MEMORY, 1 );
    m_masterVolume  = g_preferences->GetInt( PREFS_SOUND_MASTERVOLUME, 255 );

    char *soundLib  = g_preferences->GetString( PREFS_SOUND_LIBRARY );
    
    if( stricmp( soundLib, "dsound" ) == 0 ) m_soundLib = 1;
    else                                     m_soundLib = 0;
}


void SoundOptionsWindow::Create()
{
    InterfaceWindow::Create();

    int x = 150;
    int w = 170;
    int y = 30;
    int h = 30;
    
    InvertedBox *box = new InvertedBox();
    box->SetProperties( "invert", 10, 50, m_w - 20, m_h-140, " ", " ", false, false );        
    RegisterButton( box );

    DropDownMenu *soundLib = new DropDownMenu();
    soundLib->SetProperties( "Sound Library", x, y+=h, w, 20, "dialog_soundlibrary", " ", true, false );    
#ifdef HAVE_DSOUND
    soundLib->AddOption( LANGUAGEPHRASE("dialog_directsound"), 1 );
#else
    soundLib->AddOption( LANGUAGEPHRASE("dialog_softwaresound"), 0 );
#endif
    soundLib->RegisterInt( &m_soundLib );
    RegisterButton( soundLib );

    DropDownMenu *mixFreq = new DropDownMenu();
    mixFreq->SetProperties( "Mix Frequency", x, y+=h, w, 20, "dialog_mixfrequency", " ", true, false );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_11khz"), 11025 );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_22khz"), 22050 );
    mixFreq->AddOption( LANGUAGEPHRASE("dialog_44khz"), 44100 );
    mixFreq->RegisterInt( &m_mixFreq );
    RegisterButton( mixFreq );

    DropDownMenu *numChannels = new DropDownMenu();
    numChannels->SetProperties( "Num Channels", x, y+=h, w, 20, "dialog_numchannels", " ", true, false );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_8channels"), 8 );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_16channels"), 16 );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_32channels"), 32 );
    numChannels->AddOption( LANGUAGEPHRASE("dialog_64channels"), 64 );
    numChannels->RegisterInt( &m_numChannels );
    RegisterButton( numChannels );

    //DropDownMenu *memoryUsage = new DropDownMenu();
    //memoryUsage->SetProperties( "Memory Usage", x, y+=h, w, 20, "dialog_memoryusage", " ", true, false );
    //memoryUsage->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    //memoryUsage->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    //memoryUsage->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    //memoryUsage->RegisterInt( &m_memoryUsage );
    //RegisterButton( memoryUsage );

    DropDownMenu *swapStereo = new DropDownMenu();
    swapStereo->SetProperties( "Swap Stereo", x, y+=h, w, 20, "dialog_swapstereo", " ", true, false );
    swapStereo->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    swapStereo->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    swapStereo->RegisterInt( &m_swapStereo );
    RegisterButton( swapStereo );

#ifdef SOUNDOPTIONSWINDOW_USEHARDWARE3D
    HW3DDropDownMenu *hw3d = new HW3DDropDownMenu();
    hw3d->SetProperties( "Hardware3d Sound", x, y+=h, w, 20, "dialog_hw3dsound", " ", true, false );
    hw3d->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    hw3d->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    hw3d->RegisterInt( &m_useHardware3D );
    RegisterButton( hw3d );
#endif

#ifdef SOUNDOPTIONSWINDOW_USEDSPEFFECTS
    DropDownMenu *dspEffects = new DropDownMenu();
    dspEffects->SetProperties( "Realtime Effects", x, y+=h, w, 20, "dialog_realtimeeffects", " ", true, false );
    dspEffects->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    dspEffects->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    dspEffects->RegisterInt( &m_dspEffects );
    RegisterButton( dspEffects );
#endif
    
    CreateValueControl( "Master Volume", x, y+=h, w, 20, InputField::TypeInt, &m_masterVolume, 10, 0, 255, NULL, " ", false );

    CloseButton *cancel = new CloseButton();
    cancel->SetProperties( "Close", 10, m_h - 30, m_w / 2 - 15, 20, "dialog_close", " ", true, false );
    RegisterButton( cancel );

    RestartSoundButton *apply = new RestartSoundButton();
    apply->SetProperties( "Apply", cancel->m_x+cancel->m_w+10, m_h - 30, m_w / 2 - 15, 20, "dialog_apply", " ", true, false );
    RegisterButton( apply );    
}


void SoundOptionsWindow::Render( bool _hasFocus )
{
    InterfaceWindow::Render( _hasFocus );
        
    int x = m_x + 20;
    int y = m_y + 30;
    int h = 30;
    int size = 13;

    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_soundlibrary") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_mixfrequency") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_numchannels") );
    //g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_memoryusage") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_swapstereo") );

#ifdef SOUNDOPTIONSWINDOW_USEHARDWARE3D
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_hw3dsound") );
#endif

#ifdef SOUNDOPTIONSWINDOW_USEDSPEFFECTS
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_realtimeeffects") );
#endif

    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_mastervolume") );
    

    //
    // Sound system CPU usage

    //ProfiledElement *element = g_profiler->m_rootElement->m_children.GetData( "SoundSystem" );
    //if( element && element->m_lastNumCalls > 0 )
    //{
    //    float occup = element->m_lastTotalTime * 100;
    //    
    //    Colour colour = White;
    //    if( occup > 15 ) colour.Set( 255, 50, 50 );
    //    g_renderer->TextCentre( m_x + m_w/2, m_y + m_h - 50, colour, 17, "%s %d%%", LANGUAGEPHRASE("dialog_cpuusage"), int(occup) );
    //}
    //else
    //{
    //    g_renderer->TextCentre( m_x + m_w/2, m_y + m_h - 50, Colour(255,50,50), size, LANGUAGEPHRASE("dialog_cpuusageunknown") );
    //}


    //
    // Memory usage
    
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    float memoryUsage = g_soundSampleBank->GetMemoryUsage();
    memoryUsage /= 1024.0f;
    memoryUsage /= 1024.0f;
    g_renderer->TextCentre( m_x + m_w/2, m_y + m_h - 70, White, size, "%s %2.1f Mb", LANGUAGEPHRASE("dialog_memoryusage"), memoryUsage );
}



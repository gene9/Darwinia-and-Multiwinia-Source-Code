#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/gucci/window_manager.h"
#include "lib/sound/sound_library_3d.h"
#include "lib/sound/soundsystem.h"
#include "lib/render/renderer.h"

#include "interface/components/scrollbar.h"

#include "soundstats_window.h"


SoundStatsWindow::SoundStatsWindow()
:   InterfaceWindow( "Sound Stats" ),
    m_scrollBar(NULL)
{
    m_scrollBar = new ScrollBar(this);
    m_w = 550;
    m_h = 600;
    m_x = g_windowManager->WindowW() - m_w - 20;
    m_y = 30;
}


SoundStatsWindow::~SoundStatsWindow()
{
    delete m_scrollBar;
}


void SoundStatsWindow::Create()
{
    InterfaceWindow::Create();

    int x = m_w - 20;
    int y = 130;
    int w = 15;
    int h = m_h - y - 30;

    InvertedBox *box = new InvertedBox();
    box->SetProperties( "box", 10, y, m_w - 30, h, " ", " ", false, false );
    RegisterButton( box );

    int numRows = g_soundSystem->m_numChannels;
    int winSize = ( m_h - 200 ) / 10;

    m_scrollBar->Create( "ScrollBar", x, y, w, h, numRows, winSize, 1 );
}


void SoundStatsWindow::Render( bool hasFocus )
{
    InterfaceWindow::Render( hasFocus );


    //
    // Card stats

    int yPos = m_y + 20;
    int yDif = 12;
    int textSize = 10;
      

    int cpuUsage                = g_soundLibrary3d->GetCPUOverhead();
    int maxChannels             = g_soundLibrary3d->GetMaxChannels();
    int currentSoundInstances   = g_soundSystem->NumSoundInstances();
    int currentChannelsUsed     = g_soundSystem->NumChannelsUsed();
    int numSoundsDiscarded      = g_soundSystem->NumSoundsDiscarded();
    
    char const *hw3d = ( g_soundLibrary3d->Hardware3DSupport() && g_soundLibrary3d->m_hw3dDesired ? "Enabled" :
                 ( g_soundLibrary3d->Hardware3DSupport() && !g_soundLibrary3d->m_hw3dDesired ? "Disabled" : 
                                                                                         "Unavailable" ) );
    
    g_renderer->SetFont( "zerothre", false, false, true );

    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "CPU Usage               : %d", cpuUsage );
    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "Channels Max            : %d", maxChannels );
    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "Channels Used           : %d", currentChannelsUsed );       
    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "Sounds Total            : %d", currentSoundInstances );
    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "Sounds Discarded        : %d", numSoundsDiscarded );    
    g_renderer->Text( m_x + 10, yPos += yDif, White, 12, "Hardware 3D Sound       : %s", hw3d );

    yPos = m_y + 115;

    float titleX = m_x;
    g_renderer->Text( titleX+=50, yPos, White, 10, "SoundName" );
    g_renderer->Text( titleX+=100, yPos, White, 10, "Sample" );
    g_renderer->Text( titleX+=100, yPos, White, 10, "Volume" );
    g_renderer->Text( titleX+=70, yPos, White, 10, "LoopType" );
    g_renderer->Text( titleX+=80, yPos, White, 10, "Position" );
    g_renderer->Text( titleX+=90, yPos, White, 10, "TimeLeft" );

    yPos = m_y + 135;


    //
    // Currently playing sample on each channel

    int firstChannel = m_scrollBar->m_currentValue;
    int numChannels = ( m_h - 190 ) / 12;

    for( int i = 0; i < numChannels; ++i )
    {
        int x = m_x + 30;
                
        int channelIndex = i + firstChannel;
        if( channelIndex < g_soundSystem->m_numChannels )
        {
            float health = g_soundLibrary3d->GetChannelHealth(i);
            float green = health;
            float red = 1.0f-health;
                        
            g_renderer->RectFill( x-13, yPos, 10, 10, Colour(red*255,green*255,5,255) );                        
            g_renderer->Text( x, yPos, White, 11, "%2d", channelIndex );

            SoundInstanceId soundId = g_soundSystem->m_channels[channelIndex];
            SoundInstance *instance = g_soundSystem->GetSoundInstance( soundId );
            if( instance )
            {
                float priority = instance->m_perceivedVolume/2.0f;
                if( instance->m_positionType == SoundInstance::TypeMusic )
                {
                    priority = instance->m_perceivedVolume/10.0f;
                }
                float colour = 0.2f + (float) priority;
                Clamp( colour, 0.0f, 1.0f );
                Colour col( colour*255, colour*255, colour*255, 255 );

                Colour rectFill = col;
                rectFill.m_a = 50;

                g_renderer->RectFill( m_x+20, yPos, m_w-40, yDif, rectFill );
                g_renderer->Text( x + 20, yPos, col, 11, "%s", instance->GetDescriptor() );                
            }
        }

        yPos += yDif;
        if( channelIndex == g_soundSystem->m_numChannels-g_soundSystem->m_numMusicChannels-1 )
        {
            g_renderer->Line( m_x+20, yPos+3, x+m_w-50, yPos+3, White );
            g_renderer->Line( m_x+20, yPos+18, x+m_w-50, yPos+18, White );
            g_renderer->Text( x+100, yPos+5, White, 12, "Music Channels" );
            yPos+=yDif*2;
        }
    }    

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    g_renderer->SetFont();
}



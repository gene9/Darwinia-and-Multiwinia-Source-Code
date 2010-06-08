#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"
#include "lib/resource.h"
#include "lib/system_info.h"

#include "interface/scrollbar.h"
#include "interface/soundstats_window.h"

#include "sound/sound_library_3d.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "renderer.h"
#include "camera.h"
#include "main.h"


#ifdef SOUND_EDITOR


SoundStatsWindow::SoundStatsWindow( char *_name )
:   DarwiniaWindow( _name ),
    m_scrollBar(NULL)
{
    m_scrollBar = new ScrollBar(this);
    m_w = 400;
    m_h = 550;
    m_x = g_app->m_renderer->ScreenW() - m_w - 20;
    m_y = 30;
}


SoundStatsWindow::~SoundStatsWindow()
{
    delete m_scrollBar;
}


void SoundStatsWindow::Create()
{
    DarwiniaWindow::Create();

    int x = m_w - 20;
    int y = 170;
    int w = 15;
    int h = m_h - y - 30;

    int numRows = g_app->m_soundSystem->m_numChannels;
    int winSize = ( m_h - 200 ) / 10;

    m_scrollBar->Create( "ScrollBar", x, y, w, h, numRows, winSize, 1 );
}


void SoundStatsWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );


    //
    // Card stats

    int yPos = m_y + 30;
    int yDif = 11;
    int textSize = 10;
      

    int cpuUsage                = g_soundLibrary3d->GetCPUOverhead();
    int maxChannels             = g_soundLibrary3d->GetMaxChannels();
    int currentSoundInstances   = g_app->m_soundSystem->NumSoundInstances();
    int currentChannelsUsed     = g_app->m_soundSystem->NumChannelsUsed();
    int numSoundsDiscarded      = g_app->m_soundSystem->NumSoundsDiscarded();
    
    char const *hw3d = ( g_soundLibrary3d->Hardware3DSupport() && g_soundLibrary3d->m_hw3dDesired ? "Enabled" :
                 ( g_soundLibrary3d->Hardware3DSupport() && !g_soundLibrary3d->m_hw3dDesired ? "Disabled" : 
                                                                                         "Unavailable" ) );
    
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "CPU Usage               : %d", cpuUsage );
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "Channels Max            : %d", maxChannels );
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "Channels Used           : %d", currentChannelsUsed );       
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "Sounds Total            : %d", currentSoundInstances );
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "Sounds Discarded        : %d", numSoundsDiscarded );

    yPos += yDif;
    
    g_editorFont.DrawText2D( m_x + 10, yPos += yDif, textSize, "Hardware 3D Sound : %s", hw3d );

    yPos = m_y + 180;

    //
    // Sound HW available

	unsigned int deviceId = g_systemInfo->m_audioInfo.m_preferredDevice;
    char const *hwDescription = g_systemInfo->m_audioInfo.m_deviceNames[deviceId];
    LList<char *> *wrappedDescription = WordWrapText( hwDescription, 320, textSize, true);
    for( int i = 0; i < wrappedDescription->Size(); ++i )
    {
        g_editorFont.DrawText2D( m_x + 230, m_y + 40 + i * 12, textSize, wrappedDescription->GetData(i) );
    }
    delete wrappedDescription->GetData(0);
    delete wrappedDescription;

    //
    // Currently playing sample on each channel

    int firstChannel = m_scrollBar->m_currentValue;
    int numChannels = ( m_h - 200 ) / 10;

    for( int i = 0; i < numChannels; ++i )
    {
        int x = m_x + 20;
        int y = yPos + i * 10;
        
        int channelIndex = i + firstChannel;
        if( channelIndex < g_app->m_soundSystem->m_numChannels )
        {
            float health = g_soundLibrary3d->GetChannelHealth(i);
            float green = health;
            float red = 1.0f-health;
            glColor4f( red, green, 0.1f, 1.0f );
            glBegin( GL_QUADS );
                glVertex2i( x-13, y-7 );
                glVertex2i( x-3, y-7 );
                glVertex2i( x-3, y+1 );
                glVertex2i( x-13, y+1 );
            glEnd();

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );        
            g_editorFont.DrawText2D( x, y, 9, "%2d.", channelIndex );

            SoundInstanceId soundId = g_app->m_soundSystem->m_channels[channelIndex];
            SoundInstance *instance = g_app->m_soundSystem->GetSoundInstance( soundId );
            if( instance )
            {
                int priority = instance->m_calculatedPriority;
                float colour = 0.2f + (float) priority / 2.0f;
                if( colour > 1.0f ) colour = 1.0f;
                glColor4f( colour, colour, colour, 1.0f );

                g_editorFont.DrawText2D( x + 20, y, 9, "%s", instance->GetDescriptor() );                
            }
        }
    }    

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

//    int numRejected = g_app->m_soundSystem->m_sounds.NumUsed() - channelsPlaying;
//    g_editorFont.DrawText2D( m_x + m_w - 150, m_y + m_h - 12, 10, "%2d sounds rejected", numRejected );

}


#endif // SOUND_EDITOR

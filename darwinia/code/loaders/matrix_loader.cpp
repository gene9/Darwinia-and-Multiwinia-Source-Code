#include "lib/universal_include.h"

#include "lib/hi_res_time.h"
#include "lib/window_manager.h"
#include "lib/input/input.h"
#include "lib/math_utils.h"
#include "lib/resource.h"

#include "loaders/matrix_loader.h"

#include "app.h"
#include "renderer.h"

#include "sound/soundsystem.h"



int MatrixLoader::s_highlights[] = 
    { 
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
       0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 
       0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0,  
       0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
   };



MatrixLoader::MatrixLoader()
{
    memset( m_positions, 0, 40 * 30 * sizeof(float) );
}


void MatrixLoader::StartFrame()
{
    SetupFor2D( 800 );

    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
}


void MatrixLoader::Run()
{
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "LoaderMatrix", SoundSourceBlueprint::TypeMusic );
    
    float startTime = GetHighResTime();
    float timer = GetHighResTime();
    bool fallAway = false;
        
    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        //
        // Advance all the trails

        if( GetHighResTime() >= timer + MATRIXLOADER_UPDATEPERIOD )
        {
            timer = GetHighResTime();

            // Move everyone down 1 column, leaving trails
            for( int i = 0; i < 40; ++i )
            {
                if( darwiniaRandom() % 2 == 0 )
                {
                    for( int j = 30-1; j>0; --j )
                    {
                        m_positions[i][j] = m_positions[i][j-1];
                        if( m_positions[i][j] == 1.0f && s_highlights[j*40+i] != 0 )
                        {
                            s_highlights[j*40+i] *= 2;
                            s_highlights[j*40+i] = min( 100, s_highlights[j*40+i] );
                        }
                    }
                    m_positions[i][0] = m_positions[i][1] - 0.05f;
                    if( m_positions[i][1] == 1.0f ) m_positions[i][0] = 0.5f;
                    if( m_positions[i][0] < 0.0f ) m_positions[i][0] = 0.0f;
                }
            }

            // Spawn new trail randomly

            if( GetHighResTime() < startTime + 1.0f )
            {
                if( m_positions[20][0] == 0.0f ) m_positions[20][0] = 1.0f;
            }
            else if( GetHighResTime() < startTime + 15.0f )
            {                   
                int randomColumn = darwiniaRandom() % 40;
                m_positions[randomColumn][0] = 1.0f;
            }
            else if( GetHighResTime() > startTime + 20.0f && !fallAway)
            {
                for( int i = 0; i < 40; ++i )
                {
                    for( int j = 0; j < 30; ++j )
                    {
                        if( j < 10 || j > 18 )
                        {
                            m_positions[i][j] = 0;
                        }
                        else 
                        {
                            m_positions[i][j] = s_highlights[j*40+i] / 100.0f;                        
                            m_positions[i][j] *= (float) (j-10) / 8.0f;
                        }
                        s_highlights[j*40+i] = 0;
                    }
                }
                fallAway = true;
            }
            else if( GetHighResTime() > startTime + 23.0f )
            {
                return;
            }
        }
        


        //
        // Render all the trails

        StartFrame();
                
        for( int i = 0; i < 40; ++i )
        {
            for( int j = 0; j < 30; ++j )
            {
                float alpha = m_positions[i][j];
                if( s_highlights[j*40+i] > 0 ) 
                {
                    alpha = max(s_highlights[j*40+i]/100.0f, alpha);
                    alpha *= (0.2f + fabs( sinf( GetHighResTime() + j * i ) ) * 0.8f );
                }
                alpha = min( alpha, 1.0f );
                alpha = max( alpha, 0.0f );

                glColor4f( 0.4f, 1.0f, 0.4f, alpha );

                int x = 800 * (float) i / (float) 40;
                int y = 600 * (float) j / (float) 30;
                int w = 800 / 40;
                int h = 600 / 30;

                glBegin( GL_QUADS );
                    glTexCoord2i(0,1);      glVertex2i( x, y );
                    glTexCoord2i(1,1);      glVertex2i( x+w, y );
                    glTexCoord2i(1,0);      glVertex2i( x+w, y+h );
                    glTexCoord2i(0,0);      glVertex2i( x, y+h );
                glEnd();
            }
        }

        FlipBuffers();
        AdvanceSound();
    }

    g_app->m_soundSystem->StopAllSounds( WorldObjectId(), "Music LoaderMatrix" );
}


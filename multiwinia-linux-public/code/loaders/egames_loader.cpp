#include "lib/universal_include.h"

#ifdef EGAMESBUILD

#include "lib/avi_player.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/resource.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_3d.h"
#include "sound/sound_library_3d_dsound.h"

#include "renderer.h"

#include "app.h"

#include "egames_loader.h"

EgamesLoader *s_egamesLoader = NULL;


EgamesLoader::EgamesLoader()
:   Loader(),
    m_aviPlayer(NULL)
{
    s_egamesLoader = this;



    float fov = 60.0f;
    float nearPlane = 1.0f;
    float farPlane = 10000.0f;
    float screenW = g_app->m_renderer->ScreenW();
    float screenH = g_app->m_renderer->ScreenH();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, screenW / screenH, nearPlane, farPlane);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vector3 pos(0,0,0);
	Vector3 front(0,0,-1);
	Vector3 up(0,1,0);
    Vector3 forwards = pos + front;
	gluLookAt(pos.x, pos.y, pos.z,
              forwards.x, forwards.y, forwards.z,
              up.x, up.y, up.z);
    
    glDisable( GL_CULL_FACE );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}



bool AudioCallback( unsigned int _channelId, signed short *_buffer, unsigned int _numSamples, int *_silence )
{
    if( _channelId == 0 )
    {
        int numSamplesRead = s_egamesLoader->m_aviPlayer->GetAudioData( _numSamples, _buffer );

        if( numSamplesRead < _numSamples )
        {
            signed short *silenceBegin = _buffer + numSamplesRead * sizeof(signed short);
            if( g_app->m_soundSystem->IsInitialized() )
                g_soundLibrary3d->WriteSilence( silenceBegin, (_numSamples-numSamplesRead) );
        }
    }
    
    return true;
}


void EgamesLoader::RenderDarwinians( Vector3 const &_screenPos,
                                     Vector3 const &_screenRight,
                                     Vector3 const &_screenUp )
{

    Vector3 screenFront = _screenRight ^ _screenUp;
    screenFront.Normalise();

    int texId = g_app->m_resource->GetTexture( "sprites/darwinian.bmp" );
    glBindTexture( GL_TEXTURE_2D, texId );
    glEnable( GL_TEXTURE_2D );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glEnable        ( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.04f );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    srand(0);

    glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );

    for( float x = -0.9f; x < 0.9f; x+=0.2f )
    {
        for( float y = 0; y < 1.0f; y+=0.07f )
        {
            if( rand() % 100 < 65 )
            {
                Vector3 thisPos = _screenPos + _screenRight * x + screenFront * y * 700;
                thisPos += screenFront * 100;
                thisPos -= _screenUp;
                Vector3 thisRight = _screenRight * 0.07f;
                Vector3 thisUp = g_upVector * 30.0f;
                
                thisUp.RotateAroundZ( sinf(GetHighResTime())*0.2f );
                thisRight.RotateAroundZ( sinf(GetHighResTime())*0.2f );

                thisPos += Vector3( rand()%10, 0, rand()%10 );
                thisRight.x += rand()%10;
                thisUp.y += rand()%10;

                glBegin( GL_QUADS );
                    glTexCoord2i(0,0);      glVertex3fv( (thisPos-thisRight-thisUp).GetData() );
                    glTexCoord2i(1,0);      glVertex3fv( (thisPos+thisRight-thisUp).GetData() );
                    glTexCoord2i(1,1);      glVertex3fv( (thisPos+thisRight+thisUp).GetData() );
                    glTexCoord2i(0,1);      glVertex3fv( (thisPos-thisRight+thisUp).GetData() );
                glEnd();
            }
        }
    }


    glDisable( GL_ALPHA_TEST );
}



void EgamesLoader::Run()
{
    m_aviPlayer = new AviPlayer();
    int success = m_aviPlayer->OpenAvi( "cinemaware.avi" );

    if( success == -1 )
    {
        // Some clever fellow probably deleted the video
        delete m_aviPlayer;
        return;
    }

    if( g_app->m_soundSystem->IsInitialized() )
    {
        //g_soundLibrary3d = new SoundLibrary3dDirectSound();
        //g_soundLibrary3d->Initialise( 22050, 2, false, 20000, 20000 );
        g_soundLibrary3d->SetMainCallback( AudioCallback );
        g_soundLibrary3d->ResetChannel(0);
    }

    unsigned int texId;
    glEnable        (GL_TEXTURE_2D);
	glGenTextures	(1, &texId);
	glBindTexture	(GL_TEXTURE_2D, texId);
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glDisable( GL_BLEND );

    float startTime = GetHighResTime();

    while( !g_inputManager->controlEvent( ControlSkipMessage ) )
    {
        if( g_app->m_requestQuit ) break;
        float timeNow = GetHighResTime();
        int finished = m_aviPlayer->DecodeVideoFrame( timeNow - startTime );
        if( finished == 1 ) break;    
        
        int width = m_aviPlayer->GetWidth();
        int height = m_aviPlayer->GetHeight();
        float ratio = (float) height / (float) width;

    	glBindTexture	(GL_TEXTURE_2D, texId);
		glTexImage2D(GL_TEXTURE_2D, 0, 
                     GL_RGB, 
                     512, 256, 0, 
                     GL_RGB, GL_UNSIGNED_BYTE, 
                     m_aviPlayer->GetFrameData() );

        int renderW = 800;
        int renderH = 600;

        float texW = 320.0f / 512.0f;
        float texH = 240.0f / 256.0f;

        
        float timePassed = timeNow-startTime;
        float screenW = 320;
        float screenH = 240;
        float screenYpos = timePassed*2;
        float screenZpos = -300-timePassed*35;
        
        screenYpos = min(screenYpos,200);
        Vector3 screenPos(0,screenYpos,screenZpos);
        Vector3 screenUp(0,screenH,0);
        Vector3 screenRight(screenW,0,-screenW/3);

        glDisable( GL_TEXTURE_2D );
        
        glColor4f( 0.3f, 0.3f, 0.3f, 0.5f );

        glDisable( GL_DEPTH_TEST );

        float screenScale = 1.05f;
        glBegin( GL_QUADS );
            glVertex3fv((screenPos-screenRight*screenScale-screenUp*screenScale).GetData());
            glVertex3fv((screenPos+screenRight*screenScale-screenUp*screenScale).GetData());
            glVertex3fv((screenPos+screenRight*screenScale+screenUp*screenScale).GetData());
            glVertex3fv((screenPos-screenRight*screenScale+screenUp*screenScale).GetData());
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        
        glEnable( GL_TEXTURE_2D );
        glBegin( GL_QUADS );
            glTexCoord2f(0,0);                      glVertex3fv((screenPos-screenRight-screenUp).GetData());
            glTexCoord2f(texW,0);                   glVertex3fv((screenPos+screenRight-screenUp).GetData());
            glTexCoord2f(texW,texH);                glVertex3fv((screenPos+screenRight+screenUp).GetData());
            glTexCoord2f(0,texH);                   glVertex3fv((screenPos-screenRight+screenUp).GetData());
        glEnd();

        glDisable( GL_TEXTURE_2D );
        glEnable( GL_DEPTH_TEST );

        RenderDarwinians( screenPos, screenRight, screenUp );

        FlipBuffers();
        if( g_app->m_soundSystem->IsInitialized() )
            g_soundLibrary3d->Advance();
    }

    m_aviPlayer->CloseAvi();

    delete m_aviPlayer;

    glDeleteTextures( 1, &texId );

    g_app->m_soundSystem->RestartSoundLibrary();

    startTime = GetHighResTime();

    glDisable( GL_TEXTURE_2D );

    while( true )
    {
        if( GetHighResTime() > startTime+1.0f )
        {
            break;
        }
        FlipBuffers();
    }
}

#endif // EGAMESBUILD

#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"
#include "lib/targetcursor.h"

#include "interface/gesture_window.h"

#include "app.h"
#include "gesture_demo.h"
#include "taskmanager.h"
#include "gesture.h"
#include "global_world.h"


#ifdef GESTURE_EDITOR

class DrawGestureBox : public DarwiniaButton
{
public:
    void RenderBorder( int realX, int realY, bool highlighted, bool clicked )
    {
        glEnable    ( GL_BLEND );
        glDisable   ( GL_CULL_FACE );

        glColor4f   ( 0.0f, 0.0f, 0.0f, 0.5f );
        glBegin( GL_QUADS );
            glVertex2i( realX, realY );
            glVertex2i( realX + m_w, realY );
            glVertex2i( realX + m_w, realY + m_h );
            glVertex2i( realX, realY + m_h );
        glEnd();

        glColor4f   ( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin( GL_LINE_LOOP );
            glVertex2i( realX, realY );
            glVertex2i( realX + m_w, realY );
            glVertex2i( realX + m_w, realY + m_h );
            glVertex2i( realX, realY + m_h );
        glEnd();        
        
        glEnable    ( GL_CULL_FACE );
        glDisable   ( GL_BLEND );
    }

    void RenderGesture( int realX, int realY, bool highlighted, bool clicked )
    {
/*        glColor4f   ( 0.0f, 0.0f, 1.0f, 1.0f );
        glLineWidth ( 2.0f );
        glEnable    ( GL_LINE_SMOOTH );
        glEnable    ( GL_BLEND );
        glBegin     ( GL_LINE_STRIP );
        for( int i = 0; i < g_app->m_gesture->GetNumMouseSamples(); ++i )
        {
            int sampleX, sampleY;
            g_app->m_gesture->GetMouseSample( i, &sampleX, &sampleY );
            glVertex2i( realX + sampleX, realY + sampleY );
        }
        glEnd();

        int rectSize = 3;
        glLineWidth( 1.0f );
        glColor4f( 0.9f, 0.9f, 0.2f, 1.0f );
        for( int i = 0; i < g_app->m_gesture->GetNumMouseSamples(); ++i )
        {
            int sampleX, sampleY;
            g_app->m_gesture->GetMouseSample( i, &sampleX, &sampleY );
            glBegin ( GL_LINE_LOOP );        
                glVertex2i( realX + sampleX - rectSize, realY + sampleY - rectSize );
                glVertex2i( realX + sampleX + rectSize, realY + sampleY - rectSize );
                glVertex2i( realX + sampleX + rectSize, realY + sampleY + rectSize );
                glVertex2i( realX + sampleX - rectSize, realY + sampleY + rectSize );
            glEnd();        
        }

        glDisable   ( GL_BLEND );
        glDisable   ( GL_LINE_SMOOTH );*/
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        RenderBorder    ( realX, realY, highlighted, clicked );
        RenderGesture   ( realX, realY, highlighted, clicked );
    }

    void MouseDown()
    {
        if( !g_app->m_gesture->IsRecordingGesture() )
        {
            g_app->m_gesture->BeginGesture();
        }
        
        int x = g_target->X();
        int y = g_target->Y();
        g_app->m_gesture->AddSample( x, y );
    }

    void MouseUp()
    {
        g_app->m_gesture->EndGesture();
    }
};


class GestureStatusButton : public DarwiniaButton
{
public:
    int m_symbolIndex;
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {        
        int m_symbolId = g_app->m_taskManager->MapGestureToTask(m_symbolIndex);
        
        DarwiniaButton::Render( realX, realY, highlighted, clicked );

        if( m_symbolId < GlobalResearch::NumResearchItems &&
            m_symbolIndex < g_app->m_gesture->GetNumSymbols() )
        {
            char newCaption[256];
            sprintf( newCaption, "%d.%s", m_symbolIndex+1, Task::GetTaskName( m_symbolId ) );
            SetCaption( newCaption );

            char *gestureName = Task::GetTaskName( m_symbolId );
            char name[128];
            sprintf( name, "icons/gesture_%s.bmp", gestureName );
            int textureId = g_app->m_resource->GetTexture( name );

            int w = 64;
            int h = 64;

            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, textureId );
	        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

            glBegin( GL_QUADS );
                glTexCoord2f(0.0f,1.0f);        glVertex2i( realX + 13, realY + 16 );
                glTexCoord2f(1.0f,1.0f);        glVertex2i( realX + 13 + w, realY + 16 );
                glTexCoord2f(1.0f,0.0f);        glVertex2i( realX + 13 + w, realY + 16 + h );
                glTexCoord2f(0.0f,0.0f);        glVertex2i( realX + 13, realY + 16 + h );
            glEnd();

            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_TEXTURE_2D );

            if( g_app->m_gesture->GetSymbolID() == m_symbolIndex )
            {
                w = m_w;
                h = m_h;
                glColor4f( 1.0f, 1.0f, 0.0f, 1.0f );
                glLineWidth( 2.0f );
                glBegin( GL_LINE_LOOP );
                    glVertex2i( realX, realY);
                    glVertex2i( realX + w, realY);
                    glVertex2i( realX + w, realY + h );
                    glVertex2i( realX, realY + h );
                glEnd();
            }

            int numSamples = g_app->m_gesture->GetNumTrainers( m_symbolIndex );
            g_editorFont.DrawText2D( realX + 5, realY + m_h - 30, 12, "%d", numSamples );
            g_editorFont.DrawText2DRight( realX + m_w - 3, realY + m_h - 30, 10, "samples" );
        
            double conf = g_app->m_gesture->GetCalculatedConfidence( m_symbolIndex );
            g_editorFont.DrawText2D( realX + 5, realY + m_h - 18, 12, "%d%%", int(conf*100) );        
            g_editorFont.DrawText2DRight( realX + m_w - 3, realY + m_h - 18, 10, "confidence" );

            double maha = g_app->m_gesture->GetMahalanobisDistance( m_symbolIndex );
            g_editorFont.DrawText2D( realX + 5, realY + m_h - 6, 12, "%d", (int) maha );
            g_editorFont.DrawText2DRight( realX + m_w - 3, realY + m_h - 6, 10, "deviation" );
        }
    }
};


class SaveDataButton : public DarwiniaButton
{
    void MouseUp()
    {
        g_app->m_gesture->SaveTrainingData( "gestures.txt" );
    }
};


class LoadDataButton : public DarwiniaButton
{
    void MouseUp()
    {
        g_app->m_gesture->LoadTrainingData( "gestures.txt" );
    }
};


class NewGestureButton : public DarwiniaButton
{
    void MouseUp()
    {
        g_app->m_gesture->NewSymbol();
    }
};


class RecordGestureDemoButton: public DarwiniaButton
{
	void MouseUp()
	{
		RecordGestureDemo();
	}
};


GestureWindow::GestureWindow( char *_name )
:   DarwiniaWindow( _name )
{
}


void GestureWindow::Create()
{
    DarwiniaWindow::Create();

    DrawGestureBox *dgb = new DrawGestureBox();
    int h = m_h - 180;
    int w = h * 4 / 3;    
    dgb->SetShortProperties( "DrawBox", 10, 30, w, h );
    RegisterButton( dgb );

    int numSymbols = (m_w - 100) / 110;

    for( int i = 0; i < numSymbols; ++i )
    {
        GestureStatusButton *gsb = new GestureStatusButton();
        gsb->m_symbolIndex = i;
        char name[64];
        sprintf( name, "gesture%d", i+1 );
        gsb->SetShortProperties( name, 10 + i * 110, m_h - 135, 90, 120 );
        RegisterButton( gsb );
    }

	RecordGestureDemoButton *rgd = new RecordGestureDemoButton();
	rgd->SetShortProperties( "Record Demo", m_w - 100, m_h - 70, 90, 15 );
	RegisterButton( rgd );

    NewGestureButton *ng = new NewGestureButton();
    ng->SetShortProperties( "New Gesture", m_w - 100, m_h - 100, 90, 15 );
    RegisterButton( ng );
    
    LoadDataButton *ld = new LoadDataButton();
    ld->SetShortProperties( "Load Data", m_w - 100, m_h - 50, 90, 15 );
    RegisterButton( ld );

    SaveDataButton *sd = new SaveDataButton();
    sd->SetShortProperties( "Save Data", m_w - 100, m_h - 30, 90, 15 );
    RegisterButton( sd );

}

void GestureWindow::Update()
{
    DarwiniaWindow::Update();
}

void GestureWindow::Keypress( int keyCode, bool shift, bool ctrl, bool alt )
{
    /*if (keyCode >= KEY_1 && keyCode <= KEY_9)
    {
        int symbolId = keyCode - KEY_1;
        g_app->m_gesture->AddTrainingSample( symbolId );
        //g_app->m_gesture->ClearMouseSamples();
    }*/
}

#endif // GESTURE_EDITOR

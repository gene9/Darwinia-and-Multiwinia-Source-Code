#include "lib/universal_include.h"

#include "lib/hi_res_time.h"
#include "lib/avi_generator.h"

#include "app.h"
#include "renderer.h"
#include "camera.h"
#include "user_input.h"

#include "interface/grabber_window.h"

#ifdef AVI_GENERATOR

class StartRecordButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( !g_app->m_aviGenerator )
        {
            SetFakeTimeMode();

            g_app->m_aviGenerator = new AVIGenerator();
	        g_app->m_aviGenerator->m_bih.biSize = sizeof(BITMAPINFOHEADER);
	        g_app->m_aviGenerator->m_bih.biWidth = g_app->m_renderer->ScreenW();
	        g_app->m_aviGenerator->m_bih.biHeight = g_app->m_renderer->ScreenH();
	        g_app->m_aviGenerator->m_bih.biBitCount = 24;
	        g_app->m_aviGenerator->m_bih.biCompression = BI_RGB;
        	g_app->m_aviGenerator->InitEngine();
        }
    }
};

class StopRecordButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( g_app->m_aviGenerator )
        {
            SetRealTimeMode();

            g_app->m_aviGenerator->ReleaseEngine();
            delete g_app->m_aviGenerator;
            g_app->m_aviGenerator = NULL;
        }
    }
};


GrabberWindow::GrabberWindow( char *_name )
:   DarwiniaWindow( _name )
{
}


void GrabberWindow::Create()
{
	DarwiniaWindow::Create();

    StartRecordButton *start = new StartRecordButton();
    start->SetProperties( "Start", 10, 30, 70, 15, "Start", "Start" );
    RegisterButton( start );

    StopRecordButton *stop = new StopRecordButton();
    stop->SetProperties( "Stop", 90, 30, 70, 15, "Stop", "Stop" );
    RegisterButton( stop );
}


#endif // AVI_GENERATOR

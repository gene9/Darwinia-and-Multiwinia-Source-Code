/*
 *      STRESS TEST
 *
 *      MetaServer load testing tool
 *
 *
 */

#include "lib/universal_include.h"
#include "lib/gucci/window_manager.h"
#include "lib/gucci/window_manager_win32.h"
#include "lib/gucci/input.h"
#include "lib/gucci/input_win32.h"
#include "lib/render/renderer.h"


void AppMain()
{
    int windowW = 600;
    int windowH = 400;

    g_windowManager = new WindowManagerWin32();    
    g_windowManager->CreateWin( windowW, windowH, true, -1, -1, -1, -1, "SystemIV App" );    
    g_inputManager = new InputManagerWin32();
    g_renderer = new Renderer();
    

    while( true )
    {
        g_renderer->ClearScreen(true, true);
        g_renderer->Set2DViewport( 0, windowW, windowH, 0, 0, 0, 0, windowW, windowH );
        g_renderer->RectFill( 100, 100, 100, 100, White );

        g_inputManager->Advance();
        g_windowManager->Flip();
    }
}

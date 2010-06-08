#include "lib/universal_include.h"

#include <SHELLAPI.H>

#include "lib/gucci/window_manager.h"
#include "lib/gucci/window_manager_win32.h"
#include "lib/debug_utils.h"


void AppMain()
{
    int textureSize = 1024;
    int weight      = 0;
    bool italic     = false;
    bool underline  = false;
    char *fontName   = "kremlin";
    char *backupName = "arial";



    // dont edit any more

    g_windowManager = new WindowManager();
    g_windowManager->CreateWin( textureSize, textureSize, true, 32, 60, 32, "FontExtract" );        
    HDC dc = g_windowManager->m_win32Specific->m_hDC;
    int charSize = textureSize / 16;

    HFONT theFont = CreateFont( charSize, 0, 0, 0, 
                                weight, 
                                italic, 
                                underline, false, 
                                DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS,
                                CLIP_DEFAULT_PRECIS,
                                NONANTIALIASED_QUALITY,
                                FIXED_PITCH,
                                fontName );

    HFONT backupFont = CreateFont( charSize, 0, 0, 0, 
                                   weight,
                                   italic,
                                   underline, false,
                                   DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS,
                                   ANTIALIASED_QUALITY,
                                   FIXED_PITCH,
                                   backupName );

    SetTextColor( dc, RGB(255,255,255) );
    SetBkColor( dc, RGB(0,0,0) );

    RECT fillRect;
    fillRect.left = 0;
    fillRect.right = textureSize;
    fillRect.top = 0;
    fillRect.bottom = textureSize;

    FillRect( dc, &fillRect, (HBRUSH)GetStockObject(BLACK_BRUSH) );

    while( true )
    {   
        HGDIOBJ prevObj = SelectObject( dc, theFont );

        //
        // Main font

        for( int i = 0; i < 256; ++i )
        {
            int xPos = (i % 16);
            int yPos = (i / 16);
        
            char caption[32];
            sprintf( caption, "%c", i );
            
            RECT drawRect;
            drawRect.top = yPos * charSize;
            drawRect.bottom = yPos * charSize + charSize;
            drawRect.left = xPos * charSize;
            drawRect.right = xPos * charSize + charSize;
        
            int result = DrawText( dc,
                                   caption, -1,
                                   &drawRect,
                                   DT_CENTER );

            ABC abcs[1];
            GetCharABCWidths( dc, i, i, abcs );

            if( abcs[0].abcB == 1 )
            {
                SelectObject( dc, backupFont );
                int result = DrawText( dc,
                                       caption, -1,
                                       &drawRect,
                                       DT_CENTER );
                SelectObject( dc, theFont );
            }
        }
        

        SelectObject(dc, prevObj);
        
        g_windowManager->PollForMessages();           
    }
       
}

 //#include "lib/universal_include.h"
#include "lib/random.h"

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h> 
#include <unistd.h> 
#include <string.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include "eclipse.h"
#include "lib/llist.h"
#include "lib/debug_utils.h"

// ============================================================================

static LList <EclWindow *> windows;

static void (*clearDraw) (int, int, int, int) = NULL;
static void (*tooltipCallback) (EclWindow *, EclButton *) = NULL;

static LList <DirtyRect *> dirtyrects;

static int buttonDownMouseX = 0;
static int buttonDownMouseY = 0;

static int mouseX = 0;
static int mouseY = 0;
static bool lmb = false;
static bool rmb = false;

static int screenW = 0;
static int screenH = 0;

static int mouseDownWindowX = 0;
static int mouseDownWindowY = 0;

static int tooltipTimer = 0;

static int maximiseOldX = 0;
static int maximiseOldY = 0;
static int maximiseOldW = 0;
static int maximiseOldH = 0;

static char mouseDownWindow    [SIZE_ECLWINDOW_NAME] = "None";                     // Which window have I just clicked in
static char windowFocus        [SIZE_ECLWINDOW_NAME] = "None";                     // Which window is at the front
static char popupWindow        [SIZE_ECLWINDOW_NAME] = "None";                     // Current popup window
static char maximisedWindow    [SIZE_ECLWINDOW_NAME] = "None";                     // Which window is maximised
static char currentButton      [SIZE_ECLBUTTON_NAME] = "None";                     // Current highlighted button

// ============================================================================

DirtyRect::DirtyRect()
:   m_x(0), 
    m_y(0), 
    m_width(0), 
    m_height(0) 
{
}

DirtyRect::DirtyRect( int newx, int newy, int newwidth, int newheight )
:   m_x(newx), 
    m_y(newy), 
    m_width(newwidth), 
    m_height(newheight) 
{
}


void EclInitialise ( int _screenW, int _screenH )
{
    screenW = _screenW;
    screenH = _screenH;
    EclDirtyRectangle ( 0, 0, screenW, screenH );
}

void EclUpdateMouse ( int _mouseX, int _mouseY, bool _lmb, bool _rmb )
{

    int oldMouseX = mouseX;
    int oldMouseY = mouseY;

    mouseX = _mouseX;
    mouseY = _mouseY;

    EclWindow *currentHighlightedWindow = EclGetWindow( mouseX, mouseY );
    if( currentHighlightedWindow )
    {
        EclDirtyWindow( currentHighlightedWindow );
    }

    if ( !_lmb && !lmb && !_rmb && !rmb )                        // No buttons changed, mouse move only
    {
        EclWindow *currentWindow = EclGetWindow( windowFocus );
        if ( currentWindow )
        {
            EclButton *button = currentWindow->GetButton ( mouseX - currentWindow->m_x, mouseY - currentWindow->m_y );
            if ( button )
            {
                if ( strcmp ( currentButton, button->m_name ) != 0 ) 
                {
                    strcpy ( currentButton, button->m_name );
                    EclDirtyWindow ( currentWindow );
                    tooltipTimer = EclGetAccurateTime() + 1000;
                }
                else
                {
                    if( EclGetAccurateTime() > tooltipTimer )
                    {
                        if( tooltipCallback ) tooltipCallback( currentWindow, button );                    
                    }
                }
            }
            else
            {
                if ( strcmp ( currentButton, "None" ) != 0 )
                {
                    strcpy ( currentButton, "None" );
                    EclDirtyWindow ( currentWindow );
                    if( tooltipCallback ) tooltipCallback( NULL, NULL );
                }
            }
        }
        else
        {
            if ( strcmp ( currentButton, "None" ) != 0 )
            {
                strcpy ( currentButton, "None" );
                if( tooltipCallback ) tooltipCallback( NULL, NULL );
            }
        }
    }
    else if ( _lmb && !lmb )                                     // Left button down
    {
		buttonDownMouseX = mouseX;
		buttonDownMouseY = mouseY;

        EclWindow *currentWindow = EclGetWindow( mouseX, mouseY );
        if ( currentWindow )
        {            
            if ( strcmp ( windowFocus, "None" ) != 0 ) EclDirtyWindow ( windowFocus );
            strcpy ( windowFocus, currentWindow->m_name );
            EclBringWindowToFront( currentWindow->m_name );
            strcpy ( mouseDownWindow, currentWindow->m_name );
           
            EclButton *button = currentWindow->GetButton ( mouseX - currentWindow->m_x, mouseY - currentWindow->m_y );
            if ( button )
            {
                strcpy ( currentButton, button->m_name );
                button->MouseDown();
            }
            else
            {
                strcpy( currentButton, "None" );
                currentWindow->MouseEvent( true, false, false, true );
                if( currentWindow->m_movable ) 
                {
                    mouseDownWindowX = mouseX - currentWindow->m_x;
                    mouseDownWindowY = mouseY - currentWindow->m_y;
                }
            }
        
        }
        else
        {
            if ( strcmp ( windowFocus, "None" ) != 0 ) 
            {
                EclDirtyWindow ( windowFocus );
                strcpy ( windowFocus, "None" );
            }
        }

        lmb = true;
    }
    else if ( _lmb && lmb )                                 // Left button dragged
    {
		buttonDownMouseX = mouseX;
		buttonDownMouseY = mouseY;

        if ( strcmp ( mouseDownWindow, "None" ) != 0 ) {

            EclWindow *window = EclGetWindow( mouseDownWindow );
            EclButton *button = window->GetButton( mouseX - window->m_x, mouseY - window->m_y );
            
            if( button )
            {
                button->MouseDown();
            }
            else
            {
                if( strcmp( currentButton, "None" ) == 0 )
                {
                    int newWidth = window->m_w;
                    int newHeight = window->m_h;
                    bool sizeChanged = false;

                    if( oldMouseY > window->m_y + window->m_h - 4 &&
                        oldMouseY < window->m_y + window->m_h + 4 )
                    {                
                        newHeight = mouseY - window->m_y;
                        sizeChanged = true;
                    }
            
                    if( oldMouseX > window->m_x + window->m_w - 4 &&
                        oldMouseX < window->m_x + window->m_w + 4 )
                    {
                        newWidth = mouseX - window->m_x;
                        sizeChanged = true;
                    }

                    if( sizeChanged )
                    {
                        if( window->m_resizable )
                        {
                            if( newWidth < 60 ) newWidth = 60;
                            if( newHeight < 40 ) newHeight = 40;  
                            EclSetWindowSize( mouseDownWindow, newWidth, newHeight );
                        }
                    }
                    else 
                    {
                        if( window->m_movable )
                        {
                            EclSetWindowPosition ( mouseDownWindow, 
                                                   mouseX - mouseDownWindowX, 
                                                   mouseY - mouseDownWindowY );
                        }
                    }
                }
            }
        }
    }
    else if ( !_lmb && lmb )                                // Left button up
    {
        strcpy ( mouseDownWindow, "None" );
        lmb = false;

        EclWindow *currentWindow = EclGetWindow( buttonDownMouseX, buttonDownMouseY );
        if ( currentWindow )
        {
            EclButton *button = currentWindow->GetButton( buttonDownMouseX - currentWindow->m_x, buttonDownMouseY - currentWindow->m_y );
            if ( button ) {
                EclDirtyWindow ( currentWindow );
                button->MouseUp ();
            }
            else
            {
                currentWindow->MouseEvent( true, false, true, false );
                EclRemovePopup();
            }
        }
        else
        {
            EclRemovePopup();
        }
    }

    if ( _rmb && !rmb )
    {
        // Right button down
    }
    else if ( _rmb && rmb )
    {
        // Right button dragged
    }
    else if ( !_rmb && rmb )
    {
        // Right button up
    }

}

void EclUpdateKeyboard ( int keyCode, bool shift, bool ctrl, bool alt )
{
    EclWindow *currentWindow = EclGetWindow( windowFocus );
    if ( currentWindow )
    {
        currentWindow->Keypress( keyCode, shift, ctrl, alt );
    }
}

void EclRegisterTooltipCallback ( void (*_callback) (EclWindow *, EclButton *) )
{
    tooltipCallback = _callback;
}

void EclRender ()
{

    bool maximiseRender = false;

    //
    // Render any maximised Window?

    if( strcmp( maximisedWindow, "None" ) != 0 )
    {
        EclWindow *maximised = EclGetWindow( maximisedWindow );
        if( maximised )
        {
            clearDraw ( maximised->m_x, maximised->m_y, maximised->m_w, maximised->m_h );
            maximised->Render( true );
            maximiseRender = true;
        }
        else
        {
            EclUnMaximise();
        }
    }

    if( !maximiseRender )
    {

        //
        // Clear all dirty rectangle areas

        if ( clearDraw )
        {
            for( int i = 0; i < dirtyrects.Size(); ++i )
            {
                DirtyRect *dr = dirtyrects.GetData(i);
                clearDraw ( dr->m_x, dr->m_y, dr->m_width, dr->m_height );
            }
        }

        //
        // Draw all dirty buttons

        for ( int i = windows.Size() - 1; i >= 0; --i )
        {
            EclWindow *window = windows.GetData(i);
            if ( window->m_dirty ) {
                bool hasFocus = ( strcmp ( window->m_name, windowFocus ) == 0 );
                window->Render( hasFocus );
                //window->m_dirty = false;
            }
        }

    }

}

void EclUpdate ()
{

    //
    // Update all windows

    for ( int i = 0; i < windows.Size(); ++i )
    {
        EclWindow *window = windows.GetData(i);
        window->Update();
    }

}

void EclShutdown ()
{
    windows.EmptyAndDelete();
    dirtyrects.EmptyAndDelete();
}

char *EclGetCurrentButton ()
{
    return currentButton;
}

char *EclGetCurrentClickedButton ()
{
    if ( lmb )
        return currentButton;

    else
        return "None";
}

char *EclGenerateUniqueWindowName( char *name )
{
    static char uniqueName[SIZE_ECLWINDOW_NAME];

    int index = 1;
    strcpy( uniqueName, name );
    while( EclGetWindow( uniqueName ) )
    {
        ++index;
        sprintf( uniqueName, "%s%d", name, index );
    }

    return uniqueName;
}

void EclRegisterWindow ( EclWindow *window, EclWindow *parent )
{
//    DebugAssert( window );

    if ( EclGetWindow ( window->m_name ) )
    {        
    }

    if( parent && window->m_x == 0 && window->m_y == 0 )
    {
        // We should place the window in a decent location
        int left = screenW / 2 - parent->m_x;
        int above = screenH / 2 - parent->m_y;
        if( left > window->m_w / 2 )    window->m_x = int( parent->m_x + parent->m_w * (float) darwiniaRandom() / (float) DARWINIA_RAND_MAX );
        else                            window->m_x = int( parent->m_x - window->m_w * (float) darwiniaRandom() / (float) DARWINIA_RAND_MAX );
        if( above > window->m_h / 2 )   window->m_y = int( parent->m_y + parent->m_h * (float) darwiniaRandom() / (float) DARWINIA_RAND_MAX );
        else                            window->m_y = int( parent->m_y - window->m_h/2 * (float) darwiniaRandom() / (float) DARWINIA_RAND_MAX );
    }

	window->MakeAllOnScreen();
    windows.PutDataAtStart ( window );
    window->Create();
    EclDirtyWindow( window );

}

void EclRegisterPopup ( EclWindow *window )
{
    EclRemovePopup();
    //DebugAssert( window );
    strcpy( popupWindow, window->m_name );
    EclRegisterWindow( window );
}

void EclRemovePopup ()
{
    if( EclGetWindow( popupWindow ) )
    {
        EclRemoveWindow( popupWindow );
    }
    strcpy( popupWindow, "None" );
}

void EclRemoveWindow ( char *name )
{
    
    int index = EclGetWindowIndex(name);
    if ( index != -1 )
    {
    
        EclWindow *window = windows.GetData(index);
        EclDirtyRectangle ( window->m_x, window->m_y, window->m_w, window->m_h );
        windows.RemoveData(index);
        delete window;

        if( strcmp( mouseDownWindow, name ) == 0 )
        {
            strcpy( mouseDownWindow, "None" );
        }

        if( strcmp( windowFocus, name ) == 0 )
        {
            strcpy( windowFocus, "None" );
        }

    }
    else
    {
    }
    
}

void EclSetWindowPosition ( char *name, int x, int y )
{

    EclWindow *window = EclGetWindow(name);
    if ( window )
    {
        EclDirtyWindow( window );
        window->m_x = x;
        window->m_y = y;
        EclDirtyRectangle( window->m_x, window->m_y, window->m_w, window->m_h );
    }
    else
    {
    }

}

void EclSetWindowSize ( char *name, int w, int h )
{
    EclWindow *window = EclGetWindow(name);
    if ( window )
    {
        window->Remove();
        EclDirtyWindow( window );
        EclDirtyRectangle( window->m_x, window->m_y, window->m_w, window->m_h );
        window->m_w = w;
        window->m_h = h;
        window->Create();
        EclDirtyRectangle( window->m_x, window->m_y, window->m_w, window->m_h );
    }
    else
    {
    }
}

void EclBringWindowToFront ( char *name )
{

    int index = EclGetWindowIndex(name);
    if ( index != -1 )
    {
        EclWindow *window = windows.GetData(index);
        windows.RemoveData(index);
        windows.PutDataAtStart(window);
        EclDirtyWindow( window );
    }
    else
    {
    }

}

bool EclMouseInWindow( EclWindow *window )
{
    //DebugAssert( window );
    return( EclGetWindow( mouseX, mouseY ) == window );
}

bool EclMouseInButton( EclWindow *window, EclButton *button )
{
    //DebugAssert( window );
    //DebugAssert( button );

    return( EclMouseInWindow(window) &&
            mouseX >= window->m_x + button->m_x &&
            mouseX <= window->m_x + button->m_x + button->m_w &&
            mouseY >= window->m_y + button->m_y &&
            mouseY <= window->m_y + button->m_y + button->m_h );
}

bool EclIsTextEditing()
{
	 EclWindow *currentWindow = EclGetWindow( windowFocus );
	 return (currentWindow && strcmp( currentWindow->m_currentTextEdit, "None" ) != 0);
}

int EclGetWindowIndex ( char *name )
{
    for ( int i = 0; i < windows.Size(); ++i )
    {
        EclWindow *window = windows.GetData(i);
        if ( strcmp ( window->m_name, name ) == 0 )
            return i;
    }

    return -1;
}

EclWindow *EclGetWindow ( char *name )
{
    
    int index = EclGetWindowIndex (name);
    if ( index == -1 )
    {
        return NULL;
    }
    else
    {
        return windows.GetData(index);
    }

}

EclWindow *EclGetWindow ( int x, int y )
{

    for ( int i = 0; i < windows.Size(); ++i )
    {
        EclWindow *window = windows.GetData(i);
        if ( x >= window->m_x && x <= window->m_x + window->m_w &&
             y >= window->m_y && y <= window->m_y + window->m_h )
        {
            return window;
        }
    }

    return NULL;
}

void EclMaximiseWindow ( char *name )
{
    EclUnMaximise();
    EclWindow *w = EclGetWindow( name );
    if( w )
    {
        strcpy( maximisedWindow, name );
        strcpy( mouseDownWindow, name );
        strcpy( windowFocus, name );
        maximiseOldX = w->m_x;
        maximiseOldY = w->m_y;
        maximiseOldW = w->m_w;
        maximiseOldH = w->m_h;
        w->SetPosition( 0, 0 );
        w->SetSize( screenW, screenH );
    }
}

void EclUnMaximise ()
{
    EclWindow *w = EclGetWindow( maximisedWindow );
    strcpy( maximisedWindow, "None" );

    if( w )
    {
        w->SetPosition( maximiseOldX, maximiseOldY );
        w->SetSize( maximiseOldW, maximiseOldH );
    }

    EclDirtyRectangle( 0, 0, screenW, screenH );
}

LList <EclWindow *> *EclGetWindows ()
{
    return &windows;
}

int EclGetAccurateTime ()
{


#ifdef WIN32

	return GetTickCount ();

#else

	// Linux version

	static bool initted = false; 
	static struct timeval startTime; 
	struct timeval tv; 
  
	if (!initted) { 
		initted = true; 
		gettimeofday(&startTime, NULL); 
		return 0; 
	} 

	gettimeofday(&tv, NULL); 
  
	long diff_usec = tv.tv_usec - startTime.tv_usec; 
	if (diff_usec < 0) { 
		diff_usec += 1000000; 
		tv.tv_sec --; 
	} 
	long diff_sec = tv.tv_sec - startTime.tv_sec; 

	return 1000 * diff_sec + diff_usec / 1000; 

#endif
}

int EclGetScreenW()
{
	return screenW;
}

int EclGetScreenH()
{
	return screenH;
}

void EclRegisterClearFunction ( void (*_clearDraw) (int, int, int, int) )
{
    clearDraw = _clearDraw;
}

void EclDirtyWindow ( char *name )
{

    EclWindow *window = EclGetWindow(name);
    if ( window )
    {
        EclDirtyWindow( window );
    }
    else
    {
    }
}

void EclDirtyWindow ( EclWindow *window )
{
    //DebugAssert( window );

    if( !window->m_dirty )
    {
        window->m_dirty = true;
        EclDirtyRectangle ( window->m_x, window->m_y, window->m_w, window->m_h );
    }
}

void EclDirtyRectangle ( int x, int y, int w, int h )
{
    DirtyRect *dr = new DirtyRect(x, y, w, h);
    dirtyrects.PutData( dr );

    for ( int i = 0; i < windows.Size(); ++i )
    {
        EclWindow *window = windows.GetData(i);
        if ( EclRectangleOverlap( x, y, w, h, window->m_x, window->m_y, window->m_w, window->m_h ) )
            EclDirtyWindow ( window );
    }
}

void EclResetDirtyRectangles ()
{
    while ( dirtyrects.GetData(0) )
    {
        DirtyRect *dr = dirtyrects.GetData(0);
        delete dr;
        dirtyrects.RemoveData(0);
    }    

    for( int i =0; i < windows.Size(); ++i )
    {
        EclWindow *window = windows.GetData(i);
        window->m_dirty = false;
    }
}

LList<DirtyRect *> *EclGetDirtyRects ()
{
    return &dirtyrects;
}

bool EclRectangleOverlap ( int x1, int y1, int w1, int h1,
						   int x2, int y2, int w2, int h2 )
{

	int maxleft = x1 > x2 ? x1 : x2;			
	int maxtop  = y1 > y2 ? y1 : y2;
	
	int minright  = x1 + w1 < x2 + w2 ? x1 + w1 : x2 + w2;
	int minbottom = y1 + h1 < y2 + h2 ? y1 + h1 : y2 + h2;

	if ( maxtop <= minbottom && maxleft <= minright ) 
		return true;

	else
		return false;

}

char *EclGetCurrentFocus()
{
    return windowFocus;
}

void EclSetCurrentFocus( char *name )
{
    if( strlen( name ) < SIZE_ECLWINDOW_NAME )
    {
        strcpy( windowFocus, name );
    }
}
#include "lib/universal_include.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "lib/tosser/llist.h"
#include "lib/hi_res_time.h"
#include "lib/debug_utils.h"
#include "lib/profiler.h"

#include "eclipse.h"

#pragma warning( disable : 4996 4189 )

// ============================================================================

static LList <EclWindow *> windows;

static void (*tooltipCallback) (EclWindow *, EclButton *, float) = NULL;

static int mouseX = 0;
static int mouseY = 0;
static bool lmb = false;
static bool rmb = false;

static int screenW = 0;
static int screenH = 0;

static int mouseDownWindowX = -1;
static int mouseDownWindowY = -1;
static bool mouseDownRight = false;
static bool mouseDownBottom = false;

static float tooltipTimer = 0;

static int maximiseOldX = 0;
static int maximiseOldY = 0;
static int maximiseOldW = 0;
static int maximiseOldH = 0;

static char mouseDownWindow    [SIZE_ECLWINDOW_NAME] = "None";                     // Which window have I just clicked in
static char windowFocus        [SIZE_ECLWINDOW_NAME] = "None";                     // Which window is at the front
static char maximisedWindow    [SIZE_ECLWINDOW_NAME] = "None";                     // Which window is maximised
static char currentButton      [SIZE_ECLBUTTON_NAME] = "None";                     // Current highlighted button

static bool grabbedButton = false;


void EclInitialise ( int _screenW, int _screenH )
{
    screenW = _screenW;
    screenH = _screenH;
}

void EclUpdateMouse ( int _mouseX, int _mouseY, bool _lmb, bool _rmb )
{

    int oldMouseX = mouseX;
    int oldMouseY = mouseY;

    mouseX = _mouseX;
    mouseY = _mouseY;

    EclWindow *currentWindow = EclGetWindow( mouseX, mouseY );

    if ( !_lmb && !lmb && !_rmb && !rmb )                        // No buttons changed, mouse move only
    {
        if ( currentWindow )
        {
            EclButton *button = currentWindow->GetButton ( mouseX - currentWindow->m_x, mouseY - currentWindow->m_y );
            if ( button )
            {
                if ( strcmp ( currentButton, button->m_name ) != 0 ) 
                {
                    strcpy ( currentButton, button->m_name );
                    tooltipTimer = GetHighResTime();
                }
            }
            else
            {
                if ( strcmp ( currentButton, "None" ) != 0 )
                {
                    strcpy ( currentButton, "None" );
                    tooltipTimer = 0.0f;
                }
            }
        }
        else
        {
            if ( strcmp ( currentButton, "None" ) != 0 )
            {
                strcpy ( currentButton, "None" );
                tooltipTimer = 0.0f;
            }
        }
    }
    else if ( _lmb && !lmb )                                     // Left button down
    {
        EclWindow *currentWindow = EclGetWindow( mouseX, mouseY );
        if ( currentWindow )
        {            
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

                    if( mouseY > currentWindow->m_y + currentWindow->m_h - 6 )
                    {
                        mouseDownBottom = true;
                    }

                    if( mouseX > currentWindow->m_x + currentWindow->m_w - 6 )
                    {
                        mouseDownRight = true;
                    }
                }
            }
        
        }
        else
        {
            if ( strcmp ( windowFocus, "None" ) != 0 ) 
            {
                strcpy ( windowFocus, "None" );
            }
        }

        lmb = true;
    }
    else if ( _lmb && lmb )                                 // Left button dragged
    {
        if ( strcmp ( mouseDownWindow, "None" ) != 0 ) {

            EclWindow *window = EclGetWindow( mouseDownWindow );
            EclButton *button;
			
			if (grabbedButton) 
				button = window->GetButton( EclGetCurrentButton() );
			else
				button = window->GetButton( mouseX - window->m_x, mouseY - window->m_y );			
            
            if( button && mouseDownWindowX < 0.0f )
            {
                button->MouseDown();
            }
            else
            {
                if( strcmp( currentButton, "None" ) == 0 ||
                    mouseDownWindowX >= 0.0f )
                {
                    int newWidth = window->m_w;
                    int newHeight = window->m_h;
                    bool sizeChanged = false;

                    if( mouseDownBottom )
                    {                
                        newHeight = mouseY - window->m_y;
                        sizeChanged = true;
                    }
            
                    if( mouseDownRight )
                    {
                        newWidth = mouseX - window->m_x;
                        sizeChanged = true;
                    }

                    if( sizeChanged )
                    {
                        if( newWidth < window->m_minW ) newWidth = window->m_minW;
                        if( newHeight < window->m_minH ) newHeight = window->m_minH;  
                        EclSetWindowSize( mouseDownWindow, newWidth, newHeight );
                    }
					else if ( window->m_movable )
                    {
                        EclSetWindowPosition ( mouseDownWindow, 
                                               mouseX - mouseDownWindowX, 
                                               mouseY - mouseDownWindowY );
                    }
                }
            }
        }
    }
    else if ( !_lmb && lmb )                                // Left button up
    {
        strcpy ( mouseDownWindow, "None" );
        mouseDownWindowX = -1;
        mouseDownWindowY = -1;
        mouseDownBottom = false;
        mouseDownRight = false;
		grabbedButton = false;
        
        lmb = false;

        EclWindow *currentWindow = EclGetWindow( mouseX, mouseY );
        if ( currentWindow )
        {
            EclButton *button = currentWindow->GetButton( mouseX - currentWindow->m_x, mouseY - currentWindow->m_y );
            if ( button ) {
                button->MouseUp ();
            }
            else
            {
                currentWindow->MouseEvent( true, false, true, false );
            }
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



int EclMouseStatus()
{
    EclWindow *mouseWindow = EclGetWindow( mouseX, mouseY );

    bool resizeH = false;
    bool resizeV = false;

    if( mouseWindow && mouseY > mouseWindow->m_y + mouseWindow->m_h - 6 )
    {
        resizeV = true;    
    }

    if( mouseWindow && mouseX > mouseWindow->m_x + mouseWindow->m_w - 6 )
    {
        resizeH = true;
    }

    if( mouseDownBottom ) resizeV = true;
    if( mouseDownRight ) resizeH = true;

    if( resizeH && 
        resizeV )   return EclMouseStatus_ResizeHV;
    if( resizeH )   return EclMouseStatus_ResizeH;
    if( resizeV )   return EclMouseStatus_ResizeV;


    if( mouseWindow ) 
    {
        EclButton *button = mouseWindow->GetButton ( mouseX - mouseWindow->m_x, 
                                                     mouseY - mouseWindow->m_y );

        if( button )
        {
            return EclMouseStatus_InWindow;
        }
        else
        {
            return EclMouseStatus_Draggable;
        }
    }
    
    
    return EclMouseStatus_NoWindow;
}


void EclUpdateKeyboard ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
    EclWindow *currentWindow = EclGetWindow( windowFocus );
    if ( currentWindow )
    {
        currentWindow->Keypress( keyCode, shift, ctrl, alt, ascii );
    }
}

void EclRegisterTooltipCallback ( void (*_callback) (EclWindow *, EclButton *, float) )
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
        for ( int i = windows.Size() - 1; i >= 0; --i )
        {
            EclWindow *window = windows.GetData(i);
            bool hasFocus = ( strcmp ( window->m_name, windowFocus ) == 0 );
            
            START_PROFILE( window->m_name );
            window->Render( hasFocus );
            END_PROFILE( window->m_name );
        }
    }


    //
    // Render the tooltip

    if( tooltipTimer > 0.0f && tooltipCallback )
    {
        EclWindow *window = EclGetWindow();
        if( window )
        {
            EclButton *button = window->GetButton( EclGetCurrentButton() );
            {
                if( button )
                {
                    float timer = GetHighResTime() - tooltipTimer;
                    tooltipCallback( window, button, timer );
                }
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
    
    AppAssert( window );

    if ( EclGetWindow ( window->m_name ) )
    {
        AppReleaseAssert( false, "Window %s already exists", window->m_name );
    }

    if( parent && window->m_x == 0 && window->m_y == 0 )
    {
        // We should place the window in a decent locatiom
        int left = screenW / 2 - parent->m_x;
        int above = screenH / 2 - parent->m_y;
        if( left > window->m_w / 2 )    window->m_x = int( parent->m_x + parent->m_w * (float) rand() / (float) RAND_MAX );
        else                            window->m_x = int( parent->m_x - window->m_w * (float) rand() / (float) RAND_MAX );
        if( above > window->m_h / 2 )   window->m_y = int( parent->m_y + parent->m_h * (float) rand() / (float) RAND_MAX );
        else                            window->m_y = int( parent->m_y - window->m_h/2 * (float) rand() / (float) RAND_MAX );
    }

    if( window->m_x < 0 ) window->m_x = 0;
    if( window->m_y < 0 ) window->m_y = 0;
    if( window->m_x + window->m_w > screenW ) window->m_x = screenW - window->m_w;
    if( window->m_y + window->m_h > screenH ) window->m_y = screenH - window->m_h;

    windows.PutDataAtStart ( window );
    window->Create();

    EclSetCurrentFocus( window->m_name );
}


void EclRemoveWindow ( char *name )
{
    
    int index = EclGetWindowIndex(name);
    if ( index != -1 )
    {
    
        EclWindow *window = windows.GetData(index);

        if( strcmp( mouseDownWindow, name ) == 0 )
        {
            strcpy( mouseDownWindow, "None" );
        }

        if( strcmp( windowFocus, name ) == 0 )
        {
            strcpy( windowFocus, "None" );
        }

        windows.RemoveData(index);
        delete window;
    }
    else
    {
        AppDebugOut( "EclRemoveWindow failed on window %s\n", name );
    }
    
}


void EclRemoveAllWindows()
{
    while( windows.ValidIndex(0) )
    {
        EclRemoveWindow( windows[0]->m_name );
    }
}


void EclSetWindowPosition ( char *name, int x, int y )
{

    EclWindow *window = EclGetWindow(name);
    if ( window )
    {
        window->m_x = x;
        window->m_y = y;
    }
    else
    {
        AppDebugOut( "EclSetWindowPosition failed on window %s\n", name );
    }

}

void EclSetWindowSize ( char *name, int w, int h )
{
    EclWindow *window = EclGetWindow(name);
    if ( window )
    {
        window->Remove();
        window->m_w = w;
        window->m_h = h;
        window->Create();
    }
    else
    {
        AppDebugOut( "EclSetWindowSize failed on window %s\n", name );
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
    }
    else
    {
        AppDebugOut( "EclBringWindowToFront failed on window %s\n", name );
    }

}

bool EclMouseInWindow( EclWindow *window )
{
    AppAssert( window );

    return( mouseX >= window->m_x &&
            mouseX <= window->m_x + window->m_w &&
            mouseY >= window->m_y &&
            mouseY <= window->m_y + window->m_h );

}

bool EclMouseInButton( EclWindow *window, EclButton *button )
{
    AppAssert( window );
    AppAssert( button );

    return( mouseX >= window->m_x + button->m_x &&
            mouseX <= window->m_x + button->m_x + button->m_w &&
            mouseY >= window->m_y + button->m_y &&
            mouseY <= window->m_y + button->m_y + button->m_h );
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

EclWindow *EclGetWindow()
{
    //EclWindow *currentWindow = EclGetWindow( windowFocus );
    //return currentWindow;

    return EclGetWindow( mouseX, mouseY );
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
}

LList <EclWindow *> *EclGetWindows ()
{
    return &windows;
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

void EclGrabButton()
{
	grabbedButton = true;
}

void EclReleaseButton()
{
	grabbedButton = false;
}

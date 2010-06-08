
/*
 *          ECLIPSE
 *        version 2.0
 *
 *
 *  Generic Interface Library
 *
 */

#ifndef _included_eclipse_h
#define _included_eclipse_h

#include "eclwindow.h"
#include "eclbutton.h"


// ============================================================================
// High level management

void        EclInitialise                   ( int screenW, int screenH );
void        EclShutdown                     ();

void        EclUpdateMouse                  ( int mouseX, int mouseY, bool lmb, bool rmb );
void        EclUpdateKeyboard               ( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii );
void        EclRender                       ();
void        EclUpdate                       ();


// ============================================================================
// Window management


void        EclRegisterWindow               ( EclWindow *window, EclWindow *parent=NULL );
void        EclRemoveWindow                 ( char *name );
void        EclRemoveAllWindows             ();

void        EclBringWindowToFront           ( char *name );
void        EclSetWindowPosition            ( char *name, int x, int y );
void        EclSetWindowSize                ( char *name, int w, int h );

int         EclGetWindowIndex               ( char *name );                                             // -1 = failure
EclWindow   *EclGetWindow                   ( char *name );
EclWindow   *EclGetWindow                   ( int x, int y );    
EclWindow   *EclGetWindow                   ();                                                         // get current focussed window                                       

void        EclMaximiseWindow               ( char *name );
void        EclUnMaximise                   ();

char        *EclGetCurrentButton            ();
char        *EclGetCurrentClickedButton     ();

char        *EclGetCurrentFocus             ();
void        EclSetCurrentFocus              ( char *name );

void		EclGrabButton					();
void		EclReleaseButton				();

LList       <EclWindow *> *EclGetWindows    ();


// ============================================================================
// Mouse control

bool        EclMouseInWindow                ( EclWindow *window );
bool        EclMouseInButton                ( EclWindow *window, EclButton *button );

enum
{
    EclMouseStatus_NoWindow,
    EclMouseStatus_InWindow,
    EclMouseStatus_Draggable,
    EclMouseStatus_ResizeH,
    EclMouseStatus_ResizeV,
    EclMouseStatus_ResizeHV
};

int         EclMouseStatus                  ();                                                         


// ============================================================================
// Other shit


void        EclRegisterTooltipCallback      ( void (*_callback) (EclWindow *, EclButton *, float) );
char        *EclGenerateUniqueWindowName    ( char *name );                                             // In static mem (don't delete!)


#endif

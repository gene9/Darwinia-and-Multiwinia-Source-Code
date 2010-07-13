#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/gucci/input.h"
#include "lib/render/renderer.h"
#include "lib/render/styletable.h"
#include "lib/debug_utils.h"

#include "core.h"
#include "scrollbar.h"


ScrollBar::ScrollBar( EclWindow *parent )
:   m_x(0),
    m_y(0),
    m_w(0),
    m_h(0),
    m_numRows(0),
    m_winSize(0),
    m_currentValue(0)
{
    AppAssert( parent );
    strcpy( m_parentWindow, parent->m_name );
    strcpy( m_name, "New Scrollbar" );
}

ScrollBar::~ScrollBar()
{
}

void ScrollBar::Create( char *name, 
                        int x, int y, int w, int h,
                        int numRows, int winSize, 
                        int stepSize )
{

    strcpy( m_name, name );
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_numRows = numRows;
    m_winSize = winSize;

    EclWindow *parent = EclGetWindow( m_parentWindow );
    AppAssert( parent );

    char barName[256];
    char upName[256];
    char downName[256];

    sprintf( barName, "%s bar", name );
    sprintf( upName, "%s up", name );
    sprintf( downName, "%s down", name );

    ScrollChangeButton *up = new ScrollChangeButton(this, stepSize*-1);
    up->SetProperties( upName, x, y, w, 18, " ", " ", false, false );
    parent->RegisterButton( up );

    ScrollBarButton *bar = new ScrollBarButton(this);
    bar->SetProperties( barName, x, y+19, w, h-37, " ", " ", false, false );
    parent->RegisterButton( bar );

    ScrollChangeButton *down = new ScrollChangeButton(this, stepSize);
    down->SetProperties( downName, x, y+h-18, w, 18, " ", " ", false, false );
    parent->RegisterButton( down );

}

void ScrollBar::Remove()
{

    EclWindow *parent = EclGetWindow( m_parentWindow );
    if( parent )
    {
        char barName[256];
        char upName[256];
        char downName[256];

        sprintf( barName, "%s bar", m_name );
        sprintf( upName, "%s up", m_name );
        sprintf( downName, "%s down", m_name );

        parent->RemoveButton( barName );
        parent->RemoveButton( upName );
        parent->RemoveButton( downName );
    }
}

void ScrollBar::SetNumRows ( int newValue )
{
    m_numRows = newValue;
    SetCurrentValue( m_currentValue );
}

void ScrollBar::SetCurrentValue( int newValue )
{
    m_currentValue = newValue;

    if( m_numRows <= m_winSize )
    {
        m_currentValue = 0;
    }
    else
    {
        if( m_currentValue < 0 )
            m_currentValue = 0;

        if( m_currentValue > m_numRows - m_winSize )
            m_currentValue = m_numRows - m_winSize;
    }
}

void ScrollBar::SetWinSize ( int newValue )
{
    m_winSize = newValue;
    SetCurrentValue( m_currentValue );
}

void ScrollBar::ChangeCurrentValue( int newValue )
{
    SetCurrentValue( m_currentValue + newValue );
}


ScrollBarButton::ScrollBarButton( ScrollBar *scrollBar )
:   EclButton(),
    m_scrollBar( scrollBar ),
    m_grabOffset(-1)
{
}

void ScrollBarButton::Render( int realX, int realY, bool highlighted, bool clicked )
{    
    AppAssert( m_scrollBar );

    if( m_scrollBar->m_numRows <= m_scrollBar->m_winSize )
    {
        return;
    }

    Colour background = g_styleTable->GetPrimaryColour(STYLE_INPUT_BACKGROUND);
    Colour borderA = g_styleTable->GetPrimaryColour(STYLE_INPUT_BORDER);
    Colour borderB = g_styleTable->GetSecondaryColour(STYLE_INPUT_BORDER);

    // Background
    g_renderer->RectFill( realX, realY, m_w, m_h, background );
    g_renderer->Line        ( realX, realY, realX + m_w, realY, borderA );
    g_renderer->Line        ( realX, realY, realX, realY + m_h, borderA );
    g_renderer->Line        ( realX + m_w, realY, realX + m_w, realY + m_h, borderB );
    g_renderer->Line        ( realX, realY + m_h, realX + m_w, realY + m_h, borderB );


    //
    // Bar

    char *styleName             = STYLE_BUTTON_BACKGROUND;
    if( highlighted ) styleName = STYLE_BUTTON_HIGHLIGHTED;
    if( clicked ) styleName     = STYLE_BUTTON_CLICKED;

    Colour primaryCol       = g_styleTable->GetPrimaryColour(styleName);
    Colour secondaryCol     = g_styleTable->GetSecondaryColour(styleName);
    Colour borderPrimary    = g_styleTable->GetPrimaryColour(STYLE_BUTTON_BORDER);
    Colour borderSeconary   = g_styleTable->GetSecondaryColour(STYLE_BUTTON_BORDER);


    float barTop = m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows;
    float barEnd = m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows;    
    if( barEnd >= m_h ) barEnd = m_h-1;

    float xPos = realX+1;
    float yPos = realY + barTop;
    float width = m_w - 2;
    float height = barEnd-barTop;

    g_renderer->RectFill( xPos, yPos, width, height, 
                          primaryCol, primaryCol, secondaryCol, secondaryCol );

    g_renderer->Line ( xPos, yPos, xPos+width, yPos, borderPrimary );
    g_renderer->Line ( xPos, yPos, xPos, yPos+height, borderPrimary );
    g_renderer->Line ( xPos, yPos+height, xPos+width, yPos+height, borderSeconary );
    g_renderer->Line ( xPos+width, yPos, xPos+width, yPos+height, borderSeconary );
    
}

void ScrollBarButton::MouseUp()
{
    if( m_grabOffset == -1 )
    {

        AppAssert( m_scrollBar );

        EclWindow *parent = EclGetWindow( m_scrollBar->m_parentWindow );
        AppAssert( parent );

        int barTop = int( parent->m_y + m_y + m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows );
        int barEnd = int( parent->m_y + m_y + m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows );    
        if( barEnd >= m_h ) barEnd = m_h-1;

        int mouseY = g_inputManager->m_mouseY;

        if( mouseY < barTop )
        {
            m_scrollBar->ChangeCurrentValue( -m_scrollBar->m_winSize );
        }
        else if( mouseY > barEnd )
        {
            m_scrollBar->ChangeCurrentValue( m_scrollBar->m_winSize );
        }

    }
    else
    {
        m_grabOffset = -1;
    }

}

void ScrollBarButton::MouseDown()
{
    int mouseY = g_inputManager->m_mouseY;

    if( m_grabOffset > -1 )
    {
        AppAssert( m_scrollBar );
        int pixelsInside = mouseY - ( m_parent->m_y + m_y );
        float fractionInside = (float) pixelsInside / (float) m_h;
        int centreVal = fractionInside * m_scrollBar->m_numRows;
        int desiredVal = centreVal - m_grabOffset;
        m_scrollBar->SetCurrentValue( desiredVal );
		EclGrabButton();
    }
    else
    {
        AppAssert( m_scrollBar );
        EclWindow *parent = EclGetWindow( m_scrollBar->m_parentWindow );
        AppAssert( parent );
        int barTop = int( m_parent->m_y + m_y + m_h * (float) m_scrollBar->m_currentValue / (float) m_scrollBar->m_numRows );
        int barEnd = int( m_parent->m_y + m_y + m_h * (float) (m_scrollBar->m_currentValue + m_scrollBar->m_winSize) / (float) m_scrollBar->m_numRows );    
        //if( barEnd >= m_h ) barEnd = m_h-1;
        if( mouseY >= barTop && mouseY <= barEnd )
        {
            m_grabOffset = m_scrollBar->m_winSize * float( mouseY - barTop ) / float( barEnd - barTop );
        }
    }
}

ScrollChangeButton::ScrollChangeButton( ScrollBar *scrollbar, int amount )
:   InterfaceButton(),
    m_scrollBar( scrollbar ),
    m_amount( amount )
{
}

void ScrollChangeButton::MouseDown()
{
    AppAssert( m_scrollBar );
    m_scrollBar->ChangeCurrentValue( m_amount );
}

void ScrollChangeButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    if( m_scrollBar->m_numRows <= m_scrollBar->m_winSize )
    {
        return;
    }

    InterfaceButton::Render( realX, realY, highlighted, clicked );

    float midPointX = realX + m_w/2.0f;
    float midPointY = realY + m_h/2.0f;

    glBegin( GL_TRIANGLES );
    if( m_amount < 0 )
    {
        glVertex2f( midPointX, midPointY-5 );
        glVertex2f( midPointX+4, midPointY+4 );
        glVertex2f( midPointX-4, midPointY+4 );
    }
    else
    {
        glVertex2f( midPointX, midPointY+3 );
        glVertex2f( midPointX-4, midPointY-4 );
        glVertex2f( midPointX+4, midPointY-4 );
    }
    glEnd();
}

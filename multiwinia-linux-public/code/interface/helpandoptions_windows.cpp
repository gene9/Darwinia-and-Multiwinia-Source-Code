#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "interface/helpandoptions_windows.h"
#include "interface/scrollbar.h"

#include "game_menu.h"
#include "multiwinia.h"

#include "UI/ImageButton.h"
#include "UI/AuthPageButton.h"

#include "worldobject/crate.h"

#include "lib/unicode/unicode_string.h"

#define MAX_PAGES 5

class HelpWindowButton : public GameMenuButton
{
public:
    HelpWindowButton()
    :   GameMenuButton( UnicodeString() )
    {
    }

    void MouseUp()
	{	
		g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        EclRegisterWindow( new SelectHelpWindow(), m_parent );
        g_app->m_doMenuTransition = true;
    }
};

class CrateHelpWindowButton : public GameMenuButton
{
public:
    CrateHelpWindowButton()
    :   GameMenuButton( UnicodeString() )
    {
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        EclRegisterWindow( new CrateHelpWindow(), m_parent );
        g_app->m_doMenuTransition = true;
    }
};

class OptionsButton : public GameMenuButton
{
public:
    OptionsButton()
    :   GameMenuButton("dialog_options")
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_options"))
		{
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
			EclRegisterWindow( new OptionsMenuWindow(), m_parent );
            g_app->m_doMenuTransition = true;
		}
    }
};

TextButton::TextButton(char *_string)
:   DarwiniaButton(),
    m_rightAligned(false),
    m_shadow(false)
{
    if( _string )
    {
        m_string = strdup( _string );
    }
}

TextButton::TextButton( UnicodeString _unicode )
{
    TextButton(NULL);
    m_unicode = _unicode;
}

void TextButton::ChangeString( char *_newString )
{
    if( m_string )
    {
        delete m_string;
        m_string = NULL;
    }

    m_string = strdup( _newString );
}

void TextButton::Render( int realX, int realY, bool highlighted, bool clicked )
{    
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    UnicodeString string;
    if( m_unicode.Length() > 0 )
    {
        string = m_unicode;
    }
    else
    {
        string = LANGUAGEPHRASE(m_string);
    }
    LList<UnicodeString *> *wrapped = WordWrapText( string, m_w*1.5f, m_fontSize );
    int y = realY + m_h * 0.05f;
    int x = realX + m_w * 0.05f;

    if( m_centered ) x = realX + m_w/2.0f;

    for( int i = 0; i < wrapped->Size(); ++i )
    {
        if( m_centered )
        {
            if( m_shadow )
            {
                g_editorFont.SetRenderOutline(true);
                glColor4f(1.0f, 1.0f, 1.0f, 0.0f );
                g_editorFont.DrawText2DCentre( x, y, m_fontSize, *wrapped->GetData(i) );
                g_editorFont.SetRenderOutline(false);
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            }

            g_editorFont.DrawText2DCentre( x, y, m_fontSize, *wrapped->GetData(i) );
        }
        else if( m_rightAligned )
        {
            g_editorFont.DrawText2DRight( x, y, m_fontSize, *wrapped->GetData(i) );
        }
        else
        {
            if( m_shadow )
            {
                g_editorFont.SetRenderOutline(true);
                glColor4f(1.0f, 1.0f, 1.0f, 0.0f );
                g_editorFont.DrawText2D( x, y, m_fontSize, *wrapped->GetData(i) );
                g_editorFont.SetRenderOutline(false);
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            }
            g_editorFont.DrawText2D( x, y, m_fontSize, *wrapped->GetData(i) );
        }
        y+=m_fontSize * 1.2f;
    }
	wrapped->EmptyAndDelete();
    delete wrapped;

    /*glBegin( GL_LINE_LOOP );
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
        glVertex2f( realX, realY + m_h );
    glEnd();*/
}

HelpAndOptionsWindow::HelpAndOptionsWindow()
:   GameOptionsWindow( "multiwinia_menu_helpandoptions" )
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;
}


void HelpAndOptionsWindow::Create()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    GameMenuButton *button = new HelpWindowButton();
    button->SetShortProperties( "multiwinia_menu_help", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_howtoplay") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

#ifdef INCLUDE_CRATE_HELP_WINDOW
	button = new CrateHelpWindowButton();
    button->SetShortProperties( "multiwinia_menu_cratehelp", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_cratehelp") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
#endif

    button = new OptionsButton();
    button->SetShortProperties( "multiwinia_menu_options", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_settings") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

#if !defined MULTIWINIA_DEMOONLY 
    if( g_app->m_atMainMenu)
    {
		AuthPageButton *auth = new AuthPageButton();
		auth->SetShortProperties( "auth_page_button", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_auth") );
		auth->m_fontSize = fontSize;
		RegisterButton( auth );
		m_buttonOrder.PutData( auth );
    }
#endif

    yPos = leftY + leftH - buttonH * 2;

	button = new MenuCloseButton("close");
    button->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
	m_backButton = button;
}

class HelpTypeButton : public GameMenuButton
{
public:
    int m_helpType;
    HelpTypeButton()
    :   GameMenuButton( UnicodeString() ),
        m_helpType(-1)
    {
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        if( m_helpType == -1 )
        {
            EclRegisterWindow( new GeneralHelpWindow, m_parent );
        }
        else
        {
            // game type help page
            EclRegisterWindow( new GameTypeHelpWindow( Multiwinia::s_gameBlueprints[m_helpType]->GetName(), m_helpType ) );
        }
        g_app->m_doMenuTransition = true;
    }
};

SelectHelpWindow::SelectHelpWindow()
:   GameOptionsWindow("multiwinia_menu_selecthelp")
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;
}

void SelectHelpWindow::Create()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_howtoplay" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    HelpTypeButton *button = new HelpTypeButton();
    button->SetShortProperties( "general_help", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_helppage_1") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    for( int i = 0; i < Multiwinia::s_gameBlueprints.Size()-1; ++i )
    {
#if defined(HIDE_INVALID_GAMETYPES) || defined(MULTIWINIA_DEMOONLY)
        if( !g_app->IsGameModePermitted(i) )
        {
            continue;
        }
#endif

        button = new HelpTypeButton();
        button->SetShortProperties( Multiwinia::s_gameBlueprints[i]->GetName(), buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE(Multiwinia::s_gameBlueprints[i]->GetName()) );
        button->m_fontSize = fontSize;
        button->m_helpType = i;
        RegisterButton( button );
        m_buttonOrder.PutData( button );
    }

    yPos = leftY + leftH - buttonH * 2;

	//SetTitle( LANGUAGEPHRASE( "multiwinia_menu_helpandoptions" ) );

	MenuCloseButton *closebutton = new MenuCloseButton("close");
    closebutton->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    closebutton->m_fontSize = fontSize;
    RegisterButton( closebutton );
    m_buttonOrder.PutData( closebutton );
	m_backButton = closebutton;
}

CrateHelpWindow::CrateHelpWindow()
:   GameOptionsWindow("multiwinia_menu_selecthelp"),
	m_crateX(0.0f),
	m_crateY(0.0f),
	m_crateW(0.0f),
	m_crateH(0.0f),
	m_crateFontSize(0.0f),
    m_crateGap(0.0f),
	m_crateButtonOrderStartIndex(0),
	m_scrollBar(NULL)
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;
}

CrateHelpWindow::~CrateHelpWindow()
{
	m_crateButtons.Empty();
}

void CrateHelpWindow::Create()
{
	float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float titleX, titleY, titleW, titleH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH );
    GetPosition_TitleBar(titleX, titleY, titleW, titleH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    float buttonX = x;

    buttonW = (titleW / 2.0f) * 0.9f;

	m_crateX = buttonX;
	m_crateY = y;
	m_crateW = titleW * 0.9f;
	m_crateH = (g_app->m_renderer->ScreenH()/16) * 1.6f; // 64 pixels is height of achievement images
	m_crateFontSize = fontSmall;
    m_crateGap = m_crateH;
	m_crateButtonOrderStartIndex = m_buttonOrder.Size();

    float yPos = leftY;

	yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, titleW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_crates") );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.5f;

	m_crateY = yPos;

	int scrollbarW = buttonH / 2.0f;
	int scrollbarX = leftX + titleW - (scrollbarW + 10);// 10 + buttonW;
	int scrollbarY = m_crateY;//leftY+10+buttonH*1.6f;
	int scrollbarH = (leftY + leftH - buttonH * 2)-scrollbarY;

	m_scrollBar = new ScrollBar(this);
	m_scrollBar->Create("scrollBar", scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0, 10 );

	int numCrates = Crate::NumCrateRewards;
	char name[256];
	Crate c;

	UnicodeString ohnoes;
	for( int i = numCrates-1; i >= 0; i-- )
	{
		CrateButton* cButton = new CrateButton(i, m_crateFontSize);
		sprintf( name, "crate_tooltip_%s", c.GetName(i) );
		if( LANGUAGEPHRASE(name) == UnicodeString(name) ) ohnoes = ohnoes + UnicodeString(name) + UnicodeString("\n");
		sprintf( name, "crate_%s", c.GetName(i) );
		
		cButton->SetShortProperties( 
			name, m_crateX, m_crateY, m_crateW, m_crateH, cButton->m_caption );
		cButton->m_yOffset = m_crateGap*i;

		RegisterButton( cButton );
		m_crateButtons.PutData( cButton );
	}

	if( m_scrollBar )
	{
		int numRows = numCrates * m_crateH;
		numRows -= m_scrollBar->m_h;
		numRows += (g_app->m_renderer->ScreenW()/20) * 0.6f;
		numRows /= 4;
		m_scrollBar->SetNumRows(numRows);
	}

	yPos = leftY + leftH - buttonH * 2;

    MenuCloseButton *closebutton = new MenuCloseButton("close");
    closebutton->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    closebutton->m_fontSize = fontSize;
    RegisterButton( closebutton );
    m_buttonOrder.PutData( closebutton );
	m_backButton = closebutton;

	m_renderWholeScreen = true;
}

CrateButton::CrateButton( int _crateNumber, float _fontSize )
	: m_crateNumber( _crateNumber ),
	m_imageWidth(0),
	m_imageHeight(0),
	m_wrapped(NULL),
	m_yOffset(0),
	m_reset(true),
	m_startTime(-1),
	m_noTexture(false)
{
	m_fontSize = _fontSize;

	Crate c;

	sprintf( m_textureName, "icons/icon_%s.bmp", c.GetName(_crateNumber) );
	if( !g_app->m_resource->DoesTextureExist(m_textureName) )
	{
		m_noTexture = true;
	}

	char temp[256];
	sprintf(temp, "cratename_%s", c.GetName(_crateNumber) );
	m_crateName = LANGUAGEPHRASE(temp);

	m_imageWidth = g_app->m_renderer->ScreenW() / 20;
	m_imageHeight = m_imageWidth;//Keep it square

	sprintf(temp, "crate_tooltip_%s", c.GetName(_crateNumber) );
	
	if( m_wrapped ) 
	{
		m_wrapped->EmptyAndDelete();
		delete m_wrapped;
	}

	UnicodeString hint;
	char hintName[256];
	sprintf(hintName, "crate_hint_%s", c.GetName(_crateNumber));
	if( stricmp(hintName, LANGUAGEPHRASE(hintName).m_charstring) != 0 )
	{
		hint = UnicodeString("\n") + LANGUAGEPHRASE("crate_hint_message");
		hint.ReplaceStringFlag(L'H', LANGUAGEPHRASE(hintName));
	}

	m_caption = m_crateName + UnicodeString("\n") + LANGUAGEPHRASE(temp) + hint;
}

CrateButton::~CrateButton()
{
	if( m_wrapped )
	{
		m_wrapped->EmptyAndDelete();
		delete m_wrapped;
	}
}

void CrateButton::Render(int realX, int realY, bool highlighted, bool clicked)
{
	highlighted = false;
	m_highlightedThisFrame = false;
	m_disabled = true;
	m_inactive = true;
	clicked = false;

	if( m_reset )
	{
		if( !g_app->m_renderer->IsMenuTransitionComplete() ) return;

		m_startTime = GetHighResTime();
		m_reset = false;
	}

	if( !m_wrapped )
	{
		int fontWidth = g_editorFont.GetTextWidth(1,m_fontSize);
		m_wrapped = WordWrapText( m_caption, (m_w-m_imageWidth) * 0.9f, fontWidth );
		m_caption = UnicodeString();
	}

	CrateHelpWindow* chw = (CrateHelpWindow*)m_parent;
	int offset = 0;
	if( chw->m_scrollBar ) offset = chw->m_scrollBar->m_currentValue;

	float loadPos = (GetHighResTime() - m_startTime) * 750;
	loadPos = min(loadPos, m_yOffset);
	int position = realY - (offset*4) + loadPos;

	if( position < chw->m_scrollBar->m_y - (m_imageHeight/10) - m_h ||
		position > chw->m_scrollBar->m_y - (m_imageHeight/10) + chw->m_scrollBar->m_h )
	{
		return;
	}

	realY = position;

	float leftX, leftY, leftW, leftH;
	float titleX, titleY, titleW, titleH;
	GetPosition_LeftBox(leftX, leftY, leftW, leftH );
	GetPosition_TitleBar(titleX, titleY, titleW, titleH );

	g_app->m_renderer->Clip( leftX, chw->m_scrollBar->m_y - (m_imageHeight/10), m_w, chw->m_scrollBar->m_h );

	// Background colour
	glColor4f(0.4f, 0.4f, 0.4f, 0.5f);
	glBegin(GL_QUADS);
		glVertex2f(realX - m_imageWidth/10, realY - m_imageHeight/10);
		glVertex2f(realX + m_w,				realY - m_imageHeight/10);
		glVertex2f(realX + m_w,				realY + m_imageHeight*1.2f);
		glVertex2f(realX - m_imageWidth/10, realY + m_imageHeight*1.2f);
	glEnd();

	Vector2 iconCentre(realX + m_imageWidth/2, realY + m_imageHeight/2);
	float delta = m_imageWidth/2;

	glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/icon_shadow.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glDepthMask     ( false );
    glColor4f( 0.9f, 0.9f, 0.9f, 0.0f );
    
	glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );	glVertex2f( iconCentre.x - delta, iconCentre.y - delta );
        glTexCoord2i( 1, 1 );	glVertex2f( iconCentre.x + delta, iconCentre.y - delta );
        glTexCoord2i( 1, 0 );	glVertex2f( iconCentre.x + delta, iconCentre.y + delta );
        glTexCoord2i( 0, 0 );	glVertex2f( iconCentre.x - delta, iconCentre.y + delta );
    glEnd();

	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

	// Crate icon
	glEnable        ( GL_TEXTURE_2D );
	glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_noTexture ? "icons/icon_template.bmp" : m_textureName) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

	glColor4f   ( 1.0f, 1.0f, 1.0f, 1.0f );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );	glVertex2f( iconCentre.x - delta, iconCentre.y - delta );
        glTexCoord2i( 1, 1 );	glVertex2f( iconCentre.x + delta, iconCentre.y - delta );
        glTexCoord2i( 1, 0 );	glVertex2f( iconCentre.x + delta, iconCentre.y + delta );
        glTexCoord2i( 0, 0 );	glVertex2f( iconCentre.x - delta, iconCentre.y + delta );
    glEnd();

	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_TEXTURE_2D );

	// Text 
	// First line is name, the rest is description
	if( m_wrapped )
	{
		UnicodeString newString;
		int gap = m_fontSize * 1.2f;
		int space = m_h * 0.075f;
		int x = realX + m_imageWidth + m_w * 0.05f;
		int y = realY + space;

		for( int i = 0; i < m_wrapped->Size(); i++ )
		{
			if( !m_wrapped->ValidIndex(i) ) continue;

			newString = *(m_wrapped->GetData(i));
			float fontSize = m_fontSize;
			if( i == 0 ) fontSize *= 1.25f;

			glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
			g_editorFont.SetRenderOutline(true);
			g_editorFont.DrawText2DSimple( x, y, fontSize, newString );

			glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			g_editorFont.SetRenderOutline(false);
			g_editorFont.DrawText2DSimple( x, y, fontSize, newString );

			y+=fontSize * 1.2f;
		}
	}

	g_app->m_renderer->EndClip();
}

class GeneralNextPageButton : public GameMenuButton
{
public:
    int m_dir;
    GeneralNextPageButton()
    :   GameMenuButton(UnicodeString()),
        m_dir(1)
    {
    }

    void MouseUp()
    {
        GeneralHelpWindow *parent = (GeneralHelpWindow *)m_parent;
        parent->ChangePage( parent->m_page += m_dir );
    }
};

GeneralHelpWindow::GeneralHelpWindow()
:   GameOptionsWindow("multiwinia_help_general"),
    m_page(1)
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    m_renderWholeScreen = true;
}

void GeneralHelpWindow::Create()
{
    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    float titleX, titleY, titleW, titleH;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    GetPosition_TitleBar( titleX, titleY, titleW, titleH );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY+gap+buttonH;
    float fontSize = fontSmall;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, titleW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_help_general" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    int imageH = leftH / 3.5f;
    int imageW = imageH;
    int textW = titleW - imageH - (buttonX - leftX);
    int textH = leftH - buttonH * 3;

    //yPos = leftY + leftH - buttonH * 2;

    char image1[512];
    char text1[512];
    sprintf( image1, "mwhelp/general%d." TEXTURE_EXTENSION, m_page );
#ifdef TARGET_OS_MACOSX
	sprintf( text1, "multiwinia_helppage_general%d_mac", m_page );
	if (!ISLANGUAGEPHRASE(text1))
		sprintf( text1, "multiwinia_helppage_general%d", m_page );
#else
	sprintf( text1, "multiwinia_helppage_general%d", m_page );
#endif

	ImageButton *image = new ImageButton(image1);
    image->SetShortProperties( "image1", buttonX, yPos, imageW, imageH, UnicodeString() );
    RegisterButton( image );

    TextButton *text = new TextButton(text1);
    text->SetShortProperties( "general1", buttonX+imageW, yPos- textH*0.04f, textW, textH, UnicodeString() );
    text->m_fontSize = fontSize;
    RegisterButton( text );

    yPos = leftY + leftH - buttonH * 3;

    buttonW = (titleW * 0.9f) /2.0f;

    GeneralNextPageButton *prev = new GeneralNextPageButton();
    prev->SetShortProperties( "prev_page", buttonX, yPos, buttonW , buttonH, LANGUAGEPHRASE("dialog_previous") );
    prev->m_fontSize = fontMed;
    prev->m_dir = -1;
    RegisterButton(prev);
    m_buttonOrder.PutData( prev );

    GeneralNextPageButton *next = new GeneralNextPageButton();
    next->SetShortProperties( "next_page", buttonX + titleW / 2.0f, yPos, buttonW , buttonH, LANGUAGEPHRASE("dialog_next") );
    next->m_fontSize = fontMed;
    RegisterButton(next);
    m_buttonOrder.PutData( next );

	MenuCloseButton *closebutton = new MenuCloseButton("close");
    closebutton->SetShortProperties( "multiwinia_menu_back", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    closebutton->m_fontSize = fontMed;
    RegisterButton( closebutton );
    m_buttonOrder.PutData( closebutton );
	m_backButton = closebutton;
}

void GeneralHelpWindow::ChangePage(int _pageNum)
{
    if( _pageNum > MAX_PAGES || _pageNum < 1 ) 
    {
        EclRemoveWindow( m_name );
        return;
    }
    Remove();
    m_page = _pageNum;
    Create();
}

class NextPageButton : public GameMenuButton
{
public:
    int m_dir;
    NextPageButton()
    :   GameMenuButton(UnicodeString()),
        m_dir(1)
    {
    }

    void MouseUp()
    {
        GameTypeHelpWindow *parent = (GameTypeHelpWindow *)m_parent;
        parent->ChangePage( parent->m_page += m_dir );
    }
};


GameTypeHelpWindow::GameTypeHelpWindow(char *_name, int _typeId)
:   GameOptionsWindow( _name ),
    m_gameType( _typeId ),
    m_page(1)
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    m_renderWholeScreen = true;
}

void GameTypeHelpWindow::ChangePage(int _pageNum)
{
    /*if( _pageNum > MAX_PAGES || _pageNum < 1 ) 
    {
        EclRemoveWindow( m_name );
        return;
    }*/
    Remove();
    m_page = _pageNum;
    Create();
}

void GameTypeHelpWindow::Create()
{
    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    float titleX, titleY, titleW, titleH;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    GetPosition_TitleBar( titleX, titleY, titleW, titleH );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY+gap+buttonH;
    float fontSize = fontSmall;

    yPos += buttonH * 0.5f;
    char stringId[256];
    sprintf( stringId, "multiwinia_help_gametypetitle_%d", m_gameType );
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, titleW-20, buttonH*1.5f, LANGUAGEPHRASE(stringId ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    buttonW = (titleW * 0.9f) /2.0f;

    {
        int imageH = leftH / 3.5f;
        int imageW = imageH;
        int textW = titleW - imageH - (buttonX - leftX);
        int textH = leftH - buttonH * 3;

        //yPos = leftY + leftH - buttonH * 2;

        char image1[512];
        sprintf( image1, "mwhelp/help_%d_%d." TEXTURE_EXTENSION, m_gameType, m_page );

	    ImageButton *image = new ImageButton(image1);
        image->SetShortProperties( "image1", buttonX, yPos, imageW, imageH, UnicodeString() );
        RegisterButton( image );

        char stringId[512];
        sprintf( stringId, "multiwinia_helppage_%d_%d", m_gameType+1, m_page );
        if( g_app->m_langTable->DoesPhraseExist( stringId ) )
        {
			//yPos += buttonH * 0.9;

            TextButton *text = new TextButton( stringId );
            text->SetShortProperties( "text1", buttonX+imageW, yPos-textH*0.04f, textW, textH, UnicodeString() );
            text->m_fontSize = fontSize;
            RegisterButton( text );

            yPos = leftY + leftH - buttonH * 3;

            NextPageButton *prev = new NextPageButton();
            prev->SetShortProperties( "prev_page", buttonX, yPos, buttonW , buttonH, LANGUAGEPHRASE("dialog_previous") );
            prev->m_fontSize = fontMed;
            prev->m_dir = -1;
            RegisterButton(prev);
            m_buttonOrder.PutData( prev );

            NextPageButton *next = new NextPageButton();
            next->SetShortProperties( "next_page", buttonX + titleW / 2.0f, yPos, buttonW, buttonH, LANGUAGEPHRASE("dialog_next") );
            next->m_fontSize = fontMed;
            RegisterButton(next);
            m_buttonOrder.PutData( next );

	        MenuCloseButton *closebutton = new MenuCloseButton("close");
            closebutton->SetShortProperties( "multiwinia_menu_back", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
            closebutton->m_fontSize = fontMed;
            RegisterButton( closebutton );
            m_buttonOrder.PutData( closebutton );
			m_backButton = closebutton;
        }
        else
        {
            EclRemoveWindow( m_name );
            return;
        }
    }        
}

void GameTypeHelpWindow::GetLabelPos(int _gameType, int _labelId, int &_x, int &_y )
{
    switch( _gameType )
    {
        case Multiwinia::GameTypeAssault:
        {
            switch ( _labelId )
            {
                case 0: _x = 175;   _y = 65;    break;
                case 1: _x = 530;   _y = 125;    break;
                case 2: _x = 60;   _y = 225;    break;
                case 3: _x = 350;   _y = 400;    break;
            }
            break;
        }

        case Multiwinia::GameTypeBlitzkreig:
        {
            switch ( _labelId )
            {
                case 0: _x = 170;   _y = 140;   break;
                case 1: _x = 360;   _y = 345;   break;
            }
            break;
        }

        case Multiwinia::GameTypeCaptureTheStatue:
        {
            switch ( _labelId )
            {
                case 0: _x = 260;   _y = 120;   break;
                case 1: _x = 320;   _y = 105;   break;
                case 2: _x = 365;   _y = 295;   break;
            }
            break;
        }

        case Multiwinia::GameTypeKingOfTheHill:
        {
            switch ( _labelId )
            {
                case 0: _x = 385; _y = 130; break;
                case 1: _x = 420; _y = 200; break;
            }
            break;
        }

        case Multiwinia::GameTypeRocketRiot:
        {
            switch ( _labelId )
            {
                case 0: _x = 200;   _y = 50;    break;
                case 1: _x = 460;   _y = 50;    break;
                case 2: _x = 70;   _y = 300;    break;
                case 3: _x = 455;   _y = 350;    break;
            }
            break;
        }

        case Multiwinia::GameTypeSkirmish:
        {
            switch( _labelId )
            {
                case 0: _x = 370;   _y = 100;    break;
                case 1: _x = 430;   _y = 195;    break;
                case 2: _x = 160;   _y = 270;    break;
            };
            break;
        }
    }
}

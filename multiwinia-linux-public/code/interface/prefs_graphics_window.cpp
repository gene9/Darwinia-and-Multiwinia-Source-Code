#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "interface/prefs_graphics_window.h"
#include "interface/drop_down_menu.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "level_file.h"
#include "water.h"
#include "game_menu.h"

#define GRAPHICS_LANDDETAIL         "RenderLandscapeDetail"
#define GRAPHICS_WATERDETAIL        "RenderWaterDetail"
#define GRAPHICS_PIXELRANGE         "RenderPixelShader"
#define GRAPHICS_BUILDINGDETAIL     "RenderBuildingDetail"
#define GRAPHICS_ENTITYDETAIL       "RenderEntityDetail"
#define GRAPHICS_CLOUDDETAIL        "RenderCloudDetail"


class ApplyGraphicsButton : public GameMenuButton
{
public:

    ApplyGraphicsButton()
    : GameMenuButton("dialog_apply")
    {
    }

    void MouseUp()
    {
        if( m_inactive ) return;

        PrefsGraphicsWindow *parent = (PrefsGraphicsWindow *)m_parent;

        g_prefsManager->SetInt( GRAPHICS_LANDDETAIL, parent->m_landscapeDetail );
        g_prefsManager->SetInt( GRAPHICS_WATERDETAIL, parent->m_waterDetail );
        g_prefsManager->SetInt( GRAPHICS_PIXELRANGE, parent->m_pixelEffectRange );
        g_prefsManager->SetInt( GRAPHICS_BUILDINGDETAIL, parent->m_buildingDetail );
        g_prefsManager->SetInt( GRAPHICS_ENTITYDETAIL, parent->m_entityDetail );
        g_prefsManager->SetInt( GRAPHICS_CLOUDDETAIL, parent->m_cloudDetail );

        g_prefsManager->Save();

        if( g_app->m_location )
        {            
        	LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
			g_app->m_location->m_landscape.Init(def);

            delete g_app->m_location->m_water;
            g_app->m_location->m_water = new Water();
                        
            /*for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    building->SetDetail( parent->m_buildingDetail );
                }
            }*/
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        PrefsGraphicsWindow *parent = (PrefsGraphicsWindow *)m_parent;
        if( parent->m_landscapeDetail == g_prefsManager->GetInt( GRAPHICS_LANDDETAIL ) &&
            parent->m_waterDetail == g_prefsManager->GetInt( GRAPHICS_WATERDETAIL ) &&
            parent->m_pixelEffectRange == g_prefsManager->GetInt( GRAPHICS_PIXELRANGE) &&
            parent->m_buildingDetail == g_prefsManager->GetInt( GRAPHICS_BUILDINGDETAIL ) &&
            parent->m_entityDetail == g_prefsManager->GetInt( GRAPHICS_ENTITYDETAIL ) &&
            parent->m_cloudDetail == g_prefsManager->GetInt( GRAPHICS_CLOUDDETAIL ) )
        {
            m_inactive = true;
        }
        else 
        {
            m_inactive = false;
        }
        

        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }
};


PrefsGraphicsWindow::PrefsGraphicsWindow()
:   GameOptionsWindow("dialog_graphicsoptions")
{
    /*SetMenuSize( 340, 300 );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );*/

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    m_landscapeDetail   = g_prefsManager->GetInt( GRAPHICS_LANDDETAIL, 1 );
    m_waterDetail       = g_prefsManager->GetInt( GRAPHICS_WATERDETAIL, 1 );
    m_cloudDetail       = g_prefsManager->GetInt( GRAPHICS_CLOUDDETAIL, 1 );
    m_pixelEffectRange  = g_prefsManager->GetInt( GRAPHICS_PIXELRANGE, 1 );
    m_buildingDetail    = g_prefsManager->GetInt( GRAPHICS_BUILDINGDETAIL, 1 );
    m_entityDetail      = g_prefsManager->GetInt( GRAPHICS_ENTITYDETAIL, 1 );

    //g_app->m_profiler->m_doGlFinish = true;
}


PrefsGraphicsWindow::~PrefsGraphicsWindow()
{
    //g_app->m_profiler->m_doGlFinish = false;
}


void PrefsGraphicsWindow::Create()
{
    //DarwiniaWindow::Create();

	/*int fontSize = GetMenuSize(11);
	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int x = m_w/2;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w/2 - border * 2;
	int h = buttonH + border;*/


    /*InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y+border/2, m_w - 20, GetMenuSize(190) );        
    RegisterButton( box );*/

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0;
    float y = leftY;
    float fontSize = fontSmall;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    buttonH *= 0.65f;
    float border = gap + buttonH;

   /* GameMenuDropDown *landDetail = new GameMenuDropDown();
    landDetail->SetShortProperties( "dialog_landscapedetail", dropDownX, y, dropDownW, buttonH, LANGUAGEPHRASE("dialog_landscapedetail") );
    landDetail->AddOption( "dialog_high", 1 );
    landDetail->AddOption( "dialog_medium", 2 );
    landDetail->AddOption( "dialog_low", 3 );
    landDetail->AddOption( "dialog_upgrade", 4 );
    landDetail->RegisterInt( &m_landscapeDetail );
	landDetail->m_fontSize = fontSize;
    RegisterButton( landDetail );
	m_buttonOrder.PutData( landDetail );*/

    GameMenuDropDown *waterDetail = new GameMenuDropDown();
    waterDetail->SetShortProperties( "dialog_waterdetail", x, y+=buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_waterdetail") );
    waterDetail->AddOption( "dialog_high", 1 );
    waterDetail->AddOption( "dialog_medium", 2 );
    waterDetail->AddOption( "dialog_low", 3 );
    waterDetail->RegisterInt( &m_waterDetail );
	waterDetail->m_fontSize = fontSize;
    waterDetail->m_cycleMode = true;
    RegisterButton( waterDetail );
	m_buttonOrder.PutData( waterDetail );

    GameMenuDropDown *cloudDetail = new GameMenuDropDown();
    cloudDetail->SetShortProperties( "dialog_skydetail", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_skydetail") );
    cloudDetail->AddOption( "dialog_high", 1 );
    cloudDetail->AddOption( "dialog_medium", 2 );
    cloudDetail->AddOption( "dialog_low", 3 );
    cloudDetail->RegisterInt( &m_cloudDetail );
	cloudDetail->m_fontSize = fontSize;
    cloudDetail->m_cycleMode = true;
    RegisterButton( cloudDetail );
	m_buttonOrder.PutData( cloudDetail );

    GameMenuDropDown *buildingDetail = new GameMenuDropDown();
    buildingDetail->SetShortProperties( "dialog_buildingdetail", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_buildingdetail") );
    buildingDetail->AddOption( "dialog_high", 1 );
    buildingDetail->AddOption( "dialog_medium", 2 );
    buildingDetail->AddOption( "dialog_low", 3 );
    buildingDetail->RegisterInt( &m_buildingDetail );
	buildingDetail->m_fontSize = fontSize;
    buildingDetail->m_cycleMode = true;
    RegisterButton( buildingDetail );
	m_buttonOrder.PutData( buildingDetail );

    GameMenuDropDown *entityDetail = new GameMenuDropDown();
    entityDetail->SetShortProperties( "dialog_entitydetail", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_entitydetail") );
    entityDetail->AddOption( "dialog_high", 1 );
    entityDetail->AddOption( "dialog_medium", 2 );
    entityDetail->AddOption( "dialog_low", 3 );
    entityDetail->RegisterInt( &m_entityDetail );
	entityDetail->m_fontSize = fontSize;
    entityDetail->m_cycleMode = true;
    RegisterButton( entityDetail );
	m_buttonOrder.PutData( entityDetail );

   /* GameMenuCheckBox *pixelRange = new GameMenuCheckBox();
    pixelRange->SetShortProperties( "dialog_pixeleffect", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_pixeleffect") );
    pixelRange->RegisterInt( &m_pixelEffectRange );
	pixelRange->m_fontSize = fontSize;
    RegisterButton( pixelRange );
	m_buttonOrder.PutData( pixelRange );*/

    buttonH /= 0.65f;
    int buttonY = leftY + leftH - buttonH * 3;

    ApplyGraphicsButton *apply = new ApplyGraphicsButton();
    apply->SetShortProperties( "dialog_apply", x, buttonY, buttonW, buttonH, LANGUAGEPHRASE("dialog_apply") );
    apply->m_fontSize = fontMed;
    //apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );

    MenuCloseButton *cancel = new MenuCloseButton("dialog_close");
    cancel->SetShortProperties( "dialog_close", x, buttonY + buttonH + 5, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    cancel->m_fontSize = fontMed;
    //cancel->m_centered = true;    
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );
	m_backButton = cancel;

}


void RenderCPUUsage( LList<char *> *elements, int x, int y )
{
    float totalOccup = 0.0;
    for( int i = 0; i < elements->Size(); ++i )
    {
        if( g_profiler )
        {
            ProfiledElement *element = g_profiler->m_rootElement->m_children.GetData( elements->GetData(i) );    
            if( element && element->m_lastNumCalls > 0 )
            {
                float occup = element->m_lastTotalTime * 100;
                totalOccup += occup;
            }
        }
    }

    if( totalOccup > 0.0 )
    {
        //if( totalOccup > 25 ) glColor4f( 1.0, 0.3, 0.3, 1.0 );
        g_editorFont.DrawText2DCentre( x, y, 15, "%d%%", int(totalOccup) );
    }
    else
    {
        glColor4f( 1.0, 0.3, 0.3, 1.0 );
        g_editorFont.DrawText2DCentre( x, y, 15, "-" );
    }
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
}

void PrefsGraphicsWindow::Render( bool _hasFocus )
{
    GameOptionsWindow::Render( _hasFocus ); 
    return;

    DarwiniaWindow::Render( _hasFocus );

	int border = GetClientRectX1() + 10;
	int h = GetMenuSize(20)+ border;
    int x = m_x + 20;
    int y = m_y + GetClientRectY1() + border;
    int size = GetMenuSize(13);
    LList<char *> elements;

    g_editorFont.DrawText2D( x, y+=border, size, LANGUAGEPHRASE("dialog_landscapedetail") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_waterdetail") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_skydetail") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_buildingdetail") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_entitydetail") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_pixeleffect") );

    char fpsCaption[64];
    sprintf( fpsCaption, "%d FPS", g_app->m_renderer->m_fps );
    g_editorFont.DrawText2DCentre( m_x+m_w/2, m_y + m_h - GetMenuSize(60), GetMenuSize(20), fpsCaption );

}

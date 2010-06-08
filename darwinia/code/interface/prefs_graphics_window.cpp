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

#define GRAPHICS_LANDDETAIL         "RenderLandscapeDetail"
#define GRAPHICS_WATERDETAIL        "RenderWaterDetail"
#define GRAPHICS_PIXELRANGE         "RenderPixelShader"
#define GRAPHICS_BUILDINGDETAIL     "RenderBuildingDetail"
#define GRAPHICS_ENTITYDETAIL       "RenderEntityDetail"
#define GRAPHICS_CLOUDDETAIL        "RenderCloudDetail"


class ApplyGraphicsButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        if( g_app->m_location )
        {
            PrefsGraphicsWindow *parent = (PrefsGraphicsWindow *)m_parent;

            g_prefsManager->SetInt( GRAPHICS_LANDDETAIL, parent->m_landscapeDetail );
            g_prefsManager->SetInt( GRAPHICS_WATERDETAIL, parent->m_waterDetail );
            g_prefsManager->SetInt( GRAPHICS_PIXELRANGE, parent->m_pixelEffectRange );
            g_prefsManager->SetInt( GRAPHICS_BUILDINGDETAIL, parent->m_buildingDetail );
            g_prefsManager->SetInt( GRAPHICS_ENTITYDETAIL, parent->m_entityDetail );
            g_prefsManager->SetInt( GRAPHICS_CLOUDDETAIL, parent->m_cloudDetail );
            
        	LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
			g_app->m_location->m_landscape.Init(def);

            delete g_app->m_location->m_water;
            g_app->m_location->m_water = new Water();
                        
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    building->SetDetail( parent->m_buildingDetail );
                }
            }

            g_prefsManager->Save();
        }
    }
};


PrefsGraphicsWindow::PrefsGraphicsWindow()
:   DarwiniaWindow(LANGUAGEPHRASE("dialog_graphicsoptions"))
{
    SetMenuSize( 360, 300 );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );

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
    DarwiniaWindow::Create();

	int fontSize = GetMenuSize(11);
	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int x = m_w * 0.6;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - x - border * 2;
	int buttonW2 = m_w/2 - border * 2;
	int h = buttonH + border;

    InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y+border/2, m_w - 20, GetMenuSize(190) );        
    RegisterButton( box );

    DropDownMenu *landDetail = new DropDownMenu();
    landDetail->SetShortProperties( LANGUAGEPHRASE("dialog_landscapedetail"), x, y+=border, buttonW, buttonH );
    landDetail->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    landDetail->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    landDetail->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    landDetail->AddOption( LANGUAGEPHRASE("dialog_upgrade"), 4 );
    landDetail->RegisterInt( &m_landscapeDetail );
	landDetail->m_fontSize = fontSize;
    RegisterButton( landDetail );
	m_buttonOrder.PutData( landDetail );

    DropDownMenu *waterDetail = new DropDownMenu();
    waterDetail->SetShortProperties( LANGUAGEPHRASE("dialog_waterdetail"), x, y+=h, buttonW, buttonH );
    waterDetail->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    waterDetail->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    waterDetail->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    waterDetail->RegisterInt( &m_waterDetail );
	waterDetail->m_fontSize = fontSize;
    RegisterButton( waterDetail );
	m_buttonOrder.PutData( waterDetail );

    DropDownMenu *cloudDetail = new DropDownMenu();
    cloudDetail->SetShortProperties( LANGUAGEPHRASE("dialog_skydetail"), x, y+=h, buttonW, buttonH );
    cloudDetail->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    cloudDetail->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    cloudDetail->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    cloudDetail->RegisterInt( &m_cloudDetail );
	cloudDetail->m_fontSize = fontSize;
    RegisterButton( cloudDetail );
	m_buttonOrder.PutData( cloudDetail );

    DropDownMenu *buildingDetail = new DropDownMenu();
    buildingDetail->SetShortProperties( LANGUAGEPHRASE("dialog_buildingdetail"), x, y+=h, buttonW, buttonH );
    buildingDetail->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    buildingDetail->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    buildingDetail->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    buildingDetail->RegisterInt( &m_buildingDetail );
	buildingDetail->m_fontSize = fontSize;
    RegisterButton( buildingDetail );
	m_buttonOrder.PutData( buildingDetail );

    DropDownMenu *entityDetail = new DropDownMenu();
    entityDetail->SetShortProperties( LANGUAGEPHRASE("dialog_entitydetail"), x, y+=h, buttonW, buttonH );
    entityDetail->AddOption( LANGUAGEPHRASE("dialog_high"), 1 );
    entityDetail->AddOption( LANGUAGEPHRASE("dialog_medium"), 2 );
    entityDetail->AddOption( LANGUAGEPHRASE("dialog_low"), 3 );
    entityDetail->RegisterInt( &m_entityDetail );
	entityDetail->m_fontSize = fontSize;
    RegisterButton( entityDetail );
	m_buttonOrder.PutData( entityDetail );

    DropDownMenu *pixelRange = new DropDownMenu();
    pixelRange->SetShortProperties( LANGUAGEPHRASE("dialog_pixeleffect"), x, y+=h, buttonW, buttonH );
    pixelRange->AddOption( LANGUAGEPHRASE("dialog_full"), 1 );
	pixelRange->AddOption( LANGUAGEPHRASE("dialog_partial"), 2 );
    pixelRange->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    pixelRange->RegisterInt( &m_pixelEffectRange );
	pixelRange->m_fontSize = fontSize;
    RegisterButton( pixelRange );
	m_buttonOrder.PutData( pixelRange );


    CloseButton *cancel = new CloseButton();
    cancel->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, m_h - h, buttonW2, buttonH );
    cancel->m_fontSize = GetMenuSize(13);
    cancel->m_centered = true;    
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );

    ApplyGraphicsButton *apply = new ApplyGraphicsButton();
    apply->SetShortProperties( LANGUAGEPHRASE("dialog_apply"), m_w - buttonW2 - border, m_h - h, buttonW2, buttonH );
    apply->m_fontSize = GetMenuSize(13);
    apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );

}


void RenderCPUUsage( LList<char *> *elements, int x, int y )
{
#ifdef PROFILER_ENABLED
    float totalOccup = 0.0f;
    for( int i = 0; i < elements->Size(); ++i )
    {
        ProfiledElement *element = g_app->m_profiler->m_rootElement->m_children.GetData( elements->GetData(i) );    
        if( element && element->m_lastNumCalls > 0 )
        {
            float occup = element->m_lastTotalTime * 100;
            totalOccup += occup;
        }
    }

    if( totalOccup > 0.0f )
    {
        //if( totalOccup > 25 ) glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
        g_editorFont.DrawText2DCentre( x, y, 15, "%d%%", int(totalOccup) );
    }
    else
    {
        glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
        g_editorFont.DrawText2DCentre( x, y, 15, "-" );
    }
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
#endif    
}

void PrefsGraphicsWindow::Render( bool _hasFocus )
{
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

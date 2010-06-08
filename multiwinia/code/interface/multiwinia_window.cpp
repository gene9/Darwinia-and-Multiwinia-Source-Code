#include "lib/universal_include.h"
#include "lib/language_table.h"
#include "lib/text_renderer.h"

#include "lib/filesys/filesys_utils.h"

#include "interface/multiwinia_window.h"
#include "interface/input_field.h"
#include "interface/filedialog.h"
#include "interface/drop_down_menu.h"
#include "interface/helpandoptions_windows.h"
#include "multiwinia_filedialog.h"

#include "app.h"
#include "level_file.h"
#include "multiwinia.h"
#include "main.h"

#include "network/networkvalue.h"

class CreateMapButton : public GameMenuButton
{
public:
    CreateMapButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        NewMapWindow *parent = (NewMapWindow *) m_parent;
        char filename[512];
        TextReader *reader = NULL;

        if( !IsDirectory(g_app->GetMapDirectory()) ) 
        {
            CreateDirectory(g_app->GetMapDirectory());
        }
        sprintf( filename, "%s%s", g_app->GetMapDirectory(), parent->m_mapFilename.Get() );
        reader = new TextFileReader(filename);

        if( DoesFileExist(filename) )
        //if( reader )
        {
            delete reader;
            parent->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_editor_mapexists"));
            return;
        }

        LevelFile levelFile;
        sprintf( levelFile.m_mapFilename, "%s", parent->m_mapFilename.Get() );      
        sprintf( levelFile.m_missionFilename, "null" );
        strlwr( levelFile.m_mapFilename );

        levelFile.Save();

        UnicodeString message = LANGUAGEPHRASE("multiwinia_editor_mapcreated");
        message.ReplaceStringFlag( 'F', parent->m_mapFilename.Get() );
        parent->CreateErrorDialogue(message, true);
    }
};


class EditMapButton : public GameMenuButton
{
public:
    EditMapButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        MultiwiniaWindow *parent = (MultiwiniaWindow *) m_parent;

        char filename[512];
        sprintf( filename, "levels/%s", parent->m_mapFilename.Get() );
        TextReader *reader = g_app->m_resource->GetTextReader( filename );

        if( reader )
        {
            delete reader; 

            strcpy( g_app->m_requestedMap, parent->m_mapFilename.Get() );        
            strcpy( g_app->m_requestedMission, "null" );

            g_app->m_requestToggleEditing = true;
            g_app->m_requestedLocationId = 999;

            EclRemoveWindow( "dialog_mainmenu" );
            EclRemoveWindow( "dialog_multiwinia" );
        }
    }
};


class PlayMapButton : public DarwiniaButton
{
    void MouseUp()
    {
        MultiwiniaWindow *parent = (MultiwiniaWindow *) m_parent;

        strcpy( g_app->m_requestedMap, parent->m_mapFilename.Get() );
        strcpy( g_app->m_requestedMission, "null" );
        
        g_app->m_requestToggleEditing = false;
        g_app->m_requestedLocationId = 999;

        g_app->m_multiwinia->SetGameResearch( parent->m_researchLevel );
        g_app->m_multiwinia->SetGameOptions( parent->m_gameType, parent->m_params );
        
        EclRemoveWindow( "dialog_mainmenu" );
        EclRemoveWindow( "dialog_multiwinia" );
    }
};

/*
class SelectMapFileDialog : public GameOptionsWindow
{
public:
    SelectMapFileDialog( char *name, char const *parent, 
                         char const *path=NULL, char const *filter=NULL,
                         bool allowMultiSelect=false )
    :   GameOptionsWindow( name )
    {
    }
    
    void FileSelected( char *_filename )
    {
        MultiwiniaWindow *window = (MultiwiniaWindow *) EclGetWindow( m_parent );

        window->m_mapFilename.Set( _filename );
    }
};
*/

class SelectMapButton : public GameMenuButton
{
public:    

    char m_filter[64];
    char m_path[256];

    SelectMapButton()
    :   GameMenuButton(UnicodeString())
    {
        strcpy( m_filter, "mp_*" );
        strcpy( m_path, "levels/" );
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        MultiwiniaFileDialog *fileDialog = new MultiwiniaFileDialog( "Select File", m_parent->m_name, m_path, m_filter, false );    
        EclRegisterWindow( fileDialog );
        g_app->m_doMenuTransition = true;
    }
};

class NewMapWindowButton : public GameMenuButton
{
public:
    NewMapWindowButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        EclRegisterWindow( new NewMapWindow() );
        g_app->m_doMenuTransition = true;
    }
};

MultiwiniaWindow::MultiwiniaWindow()
:   GameOptionsWindow( "dialog_multiwinia" ),
    m_gameType(0),
    m_mapFilename( m_fileString, 256 )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    //SetMenuSize( 200, 230 );
//    SetPosition( screenW/2.0f - m_w/2.0f,
//                 screenH/2.0f - m_h/2.0f );

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );

    m_mapFilename.Set("mp_blank.txt" );

    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        m_researchLevel[i] = 3;
    }    
}

class CurrentMapButton : public DarwiniaButton
{
public:
    CurrentMapButton()
    :   DarwiniaButton()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        int y = realY + m_h * 0.05f;
        int x = realX + m_w * 0.05f;

        MultiwiniaWindow *parent = (MultiwiniaWindow *)m_parent;

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline( true );
        g_titleFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_editor_current") );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_titleFont.SetRenderOutline( false );
        g_titleFont.DrawText2D( x, y, m_fontSize, LANGUAGEPHRASE("multiwinia_editor_current") );

        x = realX + m_w - (m_w * 0.05f);

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline( true );
        g_titleFont.DrawText2DRight( x, y, m_fontSize, parent->m_mapFilename.Get() );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_titleFont.SetRenderOutline( false );
        g_titleFont.DrawText2DRight( x, y, m_fontSize, parent->m_mapFilename.Get() );
    }
};


void MultiwiniaWindow::Create()
{
    GameOptionsWindow::Create();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY;
    float fontSize = fontMed;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("taskmanager_mapeditor" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    //buttonH *= 0.65f;

   /* CurrentMapButton *tb = new CurrentMapButton();
    tb->SetShortProperties( "current", x, y += buttonH + gap, buttonW, buttonH * 0.65f, UnicodeString() );
    tb->m_fontSize = fontSmall;
    //tb->m_centered = true;
    RegisterButton( tb );*/

    SelectMapButton *selectMap = new SelectMapButton();
    selectMap->SetShortProperties( "SelectMap", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_editor_edit") );
    selectMap->m_fontSize = fontSize;
    strcpy(selectMap->m_filter, "*.mwm");
    sprintf(selectMap->m_path, "%s", g_app->GetMapDirectory() );
    RegisterButton( selectMap );
    m_buttonOrder.PutData( selectMap );

#ifdef TARGET_DEBUG
    selectMap = new SelectMapButton();
    selectMap->SetShortProperties( "SelectMap", x, y+=buttonH+gap, buttonW, buttonH, UnicodeString("Edit Official Maps") );
    selectMap->m_fontSize = fontSize;
    RegisterButton( selectMap );
    m_buttonOrder.PutData( selectMap );
#endif

   /* EditMapButton *editMap = new EditMapButton();
    editMap->SetShortProperties( "Edit", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_editor_edit") );
    editMap->m_fontSize = fontSize;
    RegisterButton( editMap );
    m_buttonOrder.PutData( editMap );*/

    //y+=buttonH;

    NewMapWindowButton *newmap = new NewMapWindowButton();
    newmap->SetShortProperties("newmapbutton", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_editor_create") );
    newmap->m_fontSize = fontSize;
    RegisterButton( newmap );
    m_buttonOrder.PutData( newmap );

    //buttonH /= 0.65f;
    float yPos = leftY + leftH - buttonH * 2;

    MenuCloseButton *close = new MenuCloseButton("dialog_back");
    close->SetShortProperties( "dialog_close", x, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    close->m_fontSize = fontMed;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
	m_backButton = close;
}

void MultiwiniaWindow::Update()
{
    if( strcmp( m_mapFilename.Get(), "mp_blank.txt" ) != 0 )
    {
        char filename[512];
        TextReader *reader = NULL;

//#ifdef TARGET_DEBUG
        sprintf( filename, "levels/%s", m_mapFilename.Get() );
        reader = g_app->m_resource->GetTextReader( filename );
//#else
        if( !reader )
        {
            sprintf( filename, "%s%s", g_app->GetMapDirectory(), m_mapFilename.Get() );
            reader = new TextFileReader( filename );
        }
//#endif

        if( reader )
        {
            delete reader; 

            strcpy( g_app->m_requestedMap, m_mapFilename.Get() );        
            strcpy( g_app->m_requestedMission, "null" );

            g_app->m_requestToggleEditing = true;
            g_app->m_requestedLocationId = 999;

            RemoveAllWindows();
        }
    }
}


// ============================================================================

NewMapWindow::NewMapWindow()
:   GameOptionsWindow("new_map"),
    m_mapFilename( m_fileString, 256 )
{
    m_mapFilename.Set( "mp_newmap.txt" );
}

void NewMapWindow::Create()
{
    GameOptionsWindow::Create();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontSmall;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_editor_create" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH;

    buttonH *= 0.65f;

    CreateMenuControl( "multiwinia_editor_mapfilename", InputField::TypeString, &m_mapFilename, y+=buttonH+gap, 0, NULL, x, buttonW, fontSize );
    m_buttonOrder.PutData( GetButton("multiwinia_editor_mapfilename") );

    CreateMapButton *createMap = new CreateMapButton();
    createMap->SetShortProperties( "Create", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_editor_create") );
    createMap->m_fontSize = fontSize;
    RegisterButton( createMap );
    m_buttonOrder.PutData( createMap );

    buttonH /= 0.65f;
    float yPos = leftY + leftH - buttonH * 2;

    MenuCloseButton *close = new MenuCloseButton("dialog_back");
    close->SetShortProperties( "dialog_close", x, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    close->m_fontSize = fontMed;
    //close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
	m_backButton = close;
}
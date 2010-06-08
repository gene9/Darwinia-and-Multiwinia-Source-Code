#include "lib/universal_include.h"

#include <time.h>

#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#include "interface/tree_window.h"

#include "worldobject/tree.h"

#include "app.h"
#include "location.h"
#include "location_editor.h"
#include "input_field.h"
#include "camera.h"
#include "renderer.h"
#include "global_world.h"
#include "level_file.h"


#ifdef LOCATION_EDITOR

class TreeButton : public DarwiniaButton
{
public:
    enum
    {
        TypeGenerate,
        TypeRandomise,
        TypeClone
    };
    int m_type;

    void MouseUp()
    {
        TreeWindow *tw = (TreeWindow *) m_parent;
        Building *building = g_app->m_location->GetBuilding( tw->m_selectionId );
        AppDebugAssert( building && building->m_type == Building::TypeTree );
        Tree *tree = (Tree *) building;

        switch( m_type )
        {
            case TypeGenerate:
                tree->Generate();
                break;

            case TypeRandomise:
                tree->m_seed = (int) frand(99999);
                tree->Generate();
                break;

            case TypeClone:
	            Vector3 rayStart;
	            Vector3 rayDir;
	            g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
									         g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
                Vector3 _pos;
                g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );
    
                Building *newBuilding = Building::CreateBuilding( Building::TypeTree );   
                newBuilding->Initialise( building );
                newBuilding->SetDetail( 1 );
                newBuilding->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );            
                g_app->m_location->m_levelFile->m_buildings.PutData( newBuilding );                
                
                AppSeedRandom(time(NULL));
                Tree *newTree = (Tree *) newBuilding;
                newTree->m_pos = _pos;
                newTree->m_seed = (int) frand(99999);
                newTree->m_height = tree->m_height * (1.0 + sfrand(0.3) );
                newTree->Generate();
                break;
        }
    }
};




TreeWindow::TreeWindow( char *_name )
:   DarwiniaWindow( _name ),
    m_selectionId(-1)
{
}

void TreeWindow::Create()
{
    DarwiniaWindow::Create();

    m_selectionId = g_app->m_locationEditor->m_selectionId;
    Building *building = g_app->m_location->GetBuilding( m_selectionId );
    AppDebugAssert( building && building->m_type == Building::TypeTree );
    Tree *tree = (Tree *) building;

    int y = 25;
    int h = 18;

    TreeButton *generate = new TreeButton();
    generate->m_type = TreeButton::TypeGenerate;
    generate->SetShortProperties( "editor_generate", 10, y, m_w - 20,-1, LANGUAGEPHRASE("editor_generate") );
    RegisterButton( generate );

    TreeButton *randomise = new TreeButton();
    randomise->m_type = TreeButton::TypeRandomise;
    randomise->SetShortProperties( "editor_randomise", 10, y+=h, m_w - 20, -1, LANGUAGEPHRASE("editor_randomise") );
    RegisterButton( randomise );

    TreeButton *clone = new TreeButton();
    clone->m_type = TreeButton::TypeClone;
    clone->SetShortProperties( "editor_clonesimilar", 10, y+=h, m_w - 20, -1, LANGUAGEPHRASE("editor_clonesimilar") );
    RegisterButton( clone );

    CreateColourControl( "editor_branchcolour", &tree->m_branchColour, y+=h, NULL, 10, m_w - 7 );
    CreateColourControl( "editor_leafcolour", &tree->m_leafColour, y+=h, NULL, 10, m_w - 7 );
    
    CreateValueControl( "editor_height",        &tree->m_height, y+=h, 1.0, 1.0, 1000.0 );
    CreateValueControl( "editor_budsize",       &tree->m_budsize, y+=h, 0.05, 0.0, 50.0, generate );
    CreateValueControl( "editor_pushup",        &tree->m_pushUp, y+=h, 0.01, 0.0, 5.0, generate );
    CreateValueControl( "editor_pushout",       &tree->m_pushOut, y+=h, 0.01, 0.0, 5.0, generate );
    CreateValueControl( "editor_iterations",    &tree->m_iterations, y+=h, 1, 1, 10, generate );
    CreateValueControl( "editor_seed",          &tree->m_seed, y+=h, 1, 0, 99999, generate );
	CreateValueControl( "editor_leafdrop",      &tree->m_leafDropRate, y+=h, 1, 0, 50 );
    CreateValueControl( "editor_spiritdrop",    &tree->m_spiritDropRate, y+=h, 1, 0, 50 );
    CreateValueControl( "editor_evil",          &tree->m_evil, y+=h, 1, 0, 1 );
}

void TreeWindow::Update()
{
    if( !g_app->m_locationEditor )
    {
        EclRemoveWindow( m_name );
		return;
    }

    if( g_app->m_locationEditor->m_selectionId != m_selectionId )
    {
        EclRemoveWindow( m_name );
		return;
    }

    Building *building = g_app->m_location->GetBuilding( m_selectionId );
    if( !building || building->m_type != Building::TypeTree )
    {
        EclRemoveWindow( m_name );
    }
}

#endif // LOCATION_EDITOR

#include "lib/universal_include.h"

#include <time.h>

#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

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
        DarwiniaDebugAssert( building && building->m_type == Building::TypeTree )
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
                newBuilding->SetDetail( g_prefsManager->GetInt( "RenderBuildingDetail", 1 ) );
                newBuilding->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );
                g_app->m_location->m_levelFile->m_buildings.PutData( newBuilding );

                darwiniaSeedRandom(time(NULL));
                Tree *newTree = (Tree *) newBuilding;
                newTree->m_pos = _pos;
                newTree->m_seed = (int) frand(99999);
                newTree->m_height = tree->m_height * (1.0f + sfrand(0.3f) );
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
    DarwiniaDebugAssert( building && building->m_type == Building::TypeTree )
    Tree *tree = (Tree *) building;

    int y = 25;
    int h = 18;

    TreeButton *generate = new TreeButton();
    generate->m_type = TreeButton::TypeGenerate;
    generate->SetShortProperties( LANGUAGEPHRASE("editor_generate"), 10, y, m_w - 20 );
    RegisterButton( generate );

    TreeButton *randomise = new TreeButton();
    randomise->m_type = TreeButton::TypeRandomise;
    randomise->SetShortProperties( LANGUAGEPHRASE("editor_randomise"), 10, y+=h, m_w - 20 );
    RegisterButton( randomise );

    TreeButton *clone = new TreeButton();
    clone->m_type = TreeButton::TypeClone;
    clone->SetShortProperties( LANGUAGEPHRASE("editor_clonesimilar"), 10, y+=h, m_w - 20 );
    RegisterButton( clone );

    CreateColourControl( LANGUAGEPHRASE("editor_branchcolour"), &tree->m_branchColour, y+=h, NULL, 10, m_w - 7 );
    CreateColourControl( LANGUAGEPHRASE("editor_leafcolour"), &tree->m_leafColour, y+=h, NULL, 10, m_w - 7 );

    CreateValueControl( LANGUAGEPHRASE("editor_height"),        InputField::TypeFloat, &tree->m_height, y+=h, 1.0f, 1.0f, 1000.0f );
    CreateValueControl( LANGUAGEPHRASE("editor_budsize"),       InputField::TypeFloat, &tree->m_budsize, y+=h, 0.05f, 0.0f, 50.0f, generate );
    CreateValueControl( LANGUAGEPHRASE("editor_pushup"),        InputField::TypeFloat, &tree->m_pushUp, y+=h, 0.01f, 0.0f, 5.0f, generate );
    CreateValueControl( LANGUAGEPHRASE("editor_pushout"),       InputField::TypeFloat, &tree->m_pushOut, y+=h, 0.01f, 0.0f, 5.0f, generate );
    CreateValueControl( LANGUAGEPHRASE("editor_iterations"),    InputField::TypeInt, &tree->m_iterations, y+=h, 1, 1, 10, generate );
    CreateValueControl( LANGUAGEPHRASE("editor_seed"),          InputField::TypeInt, &tree->m_seed, y+=h, 1, 0, 99999, generate );
	CreateValueControl( LANGUAGEPHRASE("editor_leafdrop"),      InputField::TypeInt, &tree->m_leafDropRate, y+=h, 1, 0, 50 );
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

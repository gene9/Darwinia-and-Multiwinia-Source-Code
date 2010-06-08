#include "lib/universal_include.h"

#include <math.h>
#include <limits.h>

#include "interface/buildings_window.h"
#include "interface/camera_mount_window.h"
#include "interface/camera_anim_window.h"
#include "interface/editor_window.h"
#include "interface/instant_unit_window.h"
#include "interface/landscape_window.h"
#include "interface/lights_window.h"
#include "interface/tree_window.h"

#include "lib/3d_sprite.h"
#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "worldobject/building.h"
#include "worldobject/worldobject.h"	// For class Light

#include "app.h"
#include "camera.h"
#include "landscape.h"
#include "location.h"
#include "level_file.h"
#include "location_editor.h"
#include "renderer.h"
#include "routing_system.h"
#include "team.h"
#include "user_input.h"


#ifdef LOCATION_EDITOR

#define INSTANT_UNIT_SIZE_FACTOR 6.0f


LocationEditor::LocationEditor()
:	m_tool(ToolNone),
	m_waitingForRelease(false),
	m_selectionId(-1),
	m_subSelectionId(-1),
	m_mode(ModeNone),
    m_moveBuildingsWithLandscape(1)
{
	MainEditWindow *mainWin = new MainEditWindow(LANGUAGEPHRASE("editor_mainedit"));
	mainWin->m_w = 150;
	mainWin->m_h = 140;
	mainWin->m_x = g_app->m_renderer->ScreenW();
	mainWin->m_y = 0;
	EclRegisterWindow(mainWin);
}


LocationEditor::~LocationEditor()
{
	MainEditWindow *mainWin = (MainEditWindow *)EclGetWindow(LANGUAGEPHRASE("editor_mainedit"));

	// Remove current tool window
	if (mainWin->m_currentEditWindow)
	{
		EclRemoveWindow(mainWin->m_currentEditWindow->m_name);
		mainWin->m_currentEditWindow = NULL;
	}

	// Remove main edit window
	EclRemoveWindow(LANGUAGEPHRASE("editor_mainedit"));
}


int LocationEditor::DoesRayHitBuilding(Vector3 const &rayStart, Vector3 const &rayDir)
{
	Location *location = g_app->m_location;

	for (int i = 0; i < location->m_levelFile->m_buildings.Size(); i++)
	{
		if (!location->m_levelFile->m_buildings.ValidIndex(i))	continue;

		Building *building = location->m_levelFile->m_buildings.GetData(i);
		bool result = building->DoesRayHit(rayStart, rayDir);
		if (result)
		{
			return building->m_id.GetUniqueId();
		}
	}

	return -1;
}


int LocationEditor::DoesRayHitInstantUnit(Vector3 const &rayStart, Vector3 const &rayDir)
{
	Location *location = g_app->m_location;

	for (int i = 0; i < location->m_levelFile->m_instantUnits.Size(); i++)
	{
		if (!location->m_levelFile->m_instantUnits.ValidIndex(i))	continue;

		InstantUnit *iu = location->m_levelFile->m_instantUnits.GetData(i);
		Vector3 pos(iu->m_posX, 0, iu->m_posZ);
		pos.y = location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
		bool result = RaySphereIntersection(rayStart, rayDir, pos, 
											sqrtf(iu->m_number) * INSTANT_UNIT_SIZE_FACTOR);
		if (result)
		{
			return i;
		}
	}

	return -1;
}



int LocationEditor::DoesRayHitCameraMount(Vector3 const &rayStart, Vector3 const &rayDir)
{
	Shape *camShape = g_app->m_resource->GetShape("camera.shp");
	Vector3 centre = camShape->CalculateCentre(g_identityMatrix34);
	float radius = camShape->CalculateRadius(g_identityMatrix34, centre);

	for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
	{
		CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
		if (RaySphereIntersection(rayStart, rayDir, mount->m_pos, radius))
		{
			return i;
		}
	}

	return -1;
}


int	LocationEditor::IsPosInLandTile(Vector3 const &pos)
{
	Landscape *land = &g_app->m_location->m_landscape;
	LList<LandscapeTile *> *tiles = &g_app->m_location->m_levelFile->m_landscape.m_tiles;
	int smallestId = -1;
	int smallestSize = INT_MAX;

	for (int i = 0; i < tiles->Size(); ++i)
	{
		LandscapeTile *tile = tiles->GetData(i);
		float worldX = tile->m_posX;
		float worldZ = tile->m_posZ;
		float sizeX = tile->m_size;
		float sizeZ = tile->m_size;
		if (pos.x > worldX &&
			pos.x < worldX + sizeX &&
			pos.z > worldZ &&
			pos.z < worldZ + sizeZ) 
		{
			if (tile->m_size < smallestSize)
			{
				smallestId = i;
				smallestSize = tile->m_size;
			}
		}
	}

	return smallestId;
}


int	LocationEditor::IsPosInFlattenArea(Vector3 const &pos)
{
	LList<LandscapeFlattenArea *> *areas = &g_app->m_location->m_levelFile->m_landscape.m_flattenAreas;
	Landscape *land = &g_app->m_location->m_landscape;

	for (int i = 0; i < areas->Size(); ++i)
	{
		LandscapeFlattenArea *area = areas->GetData(i);
		float halfSize = area->m_size;
		float size = halfSize * 2.0f;
		float worldX = area->m_centre.x - halfSize;
		float worldZ = area->m_centre.z - halfSize;
		if (pos.x > worldX &&
			pos.x < worldX + size &&
			pos.z > worldZ &&
			pos.z < worldZ + size) 
		{
			return i;
		}
	}

	return -1;
}


void LocationEditor::CreateEditWindowForMode(int _mode)
{
	MainEditWindow *mainWin = (MainEditWindow *)EclGetWindow(LANGUAGEPHRASE("editor_mainedit"));

	// Remove existing window
	if (mainWin->m_currentEditWindow)
	{
		EclRemoveWindow(mainWin->m_currentEditWindow->m_name);
		mainWin->m_currentEditWindow = NULL;
	}

	DarwiniaWindow *window = NULL;
	switch(_mode)
	{
		case LocationEditor::ModeLandTile:
			window = new LandscapeEditWindow(LANGUAGEPHRASE("editor_landscape"));
			window->m_w = 220;
			break;
		case LocationEditor::ModeLandFlat:
			window = new LandscapeEditWindow(LANGUAGEPHRASE("editor_landscape"));
			window->m_w = 220;
			break;
		case LocationEditor::ModeLight:
			window = new LightsEditWindow(LANGUAGEPHRASE("editor_lights"));
			window->m_w = 220;
			break;
		case LocationEditor::ModeBuilding:
			window = new BuildingsCreateWindow(LANGUAGEPHRASE("editor_buildings"));
			window->m_w = 220;
			break;
		case LocationEditor::ModeInstantUnit:
			window = new InstantUnitCreateWindow(LANGUAGEPHRASE("editor_instantunits"));
			window->m_w = 220;
			break;
		case LocationEditor::ModeCameraMount:
			{
				window = new CameraMountEditWindow(LANGUAGEPHRASE("editor_cameramounts"));
				window->m_w = 320;
//				DarwiniaWindow *otherWin = new CameraAnimMainEditWindow(LANGUAGEPHRASE("editor_cameraanims"));
//				otherWin->m_w = 270;
//				otherWin->m_h = 100;
//				otherWin->m_x = 340;
//				otherWin->m_y = g_app->m_renderer->ScreenH() - window->m_h;
//				EclRegisterWindow(otherWin);
			}
			break;
	}

	window->m_h = 100;
	window->m_x = 0;
	window->m_y = g_app->m_renderer->ScreenH() - window->m_h;
	EclRegisterWindow(window);
	
	mainWin->m_currentEditWindow = window;
}


void LocationEditor::RequestMode(int _mode)
{
	if (_mode == m_mode) return;
	if (_mode != ModeNone) 
	{
		CreateEditWindowForMode(_mode);
	}
	
	m_mode = _mode;
	m_selectionId = -1;
}


int LocationEditor::GetMode()
{
	return m_mode;
}


void LocationEditor::AdvanceModeNone()
{
	Vector3 pos = g_app->m_userInput->GetMousePos3d();
	Vector3 rayStart, rayDir;
	g_app->m_camera->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );

	if ( g_inputManager->controlEvent( ControlSelectLocation ) ) // TODO: Really?
	{
		if (DoesRayHitBuilding(rayStart, rayDir) != -1)				RequestMode(ModeBuilding);
		else if (DoesRayHitInstantUnit(rayStart, rayDir) != -1)		RequestMode(ModeInstantUnit);
		else if (DoesRayHitCameraMount(rayStart, rayDir) != -1)		RequestMode(ModeCameraMount);
		else if (IsPosInLandTile(pos) != -1)						RequestMode(ModeLandTile);
		else if (IsPosInFlattenArea(pos) != -1)						RequestMode(ModeLandTile);
	}
}


void LocationEditor::MoveBuildingsInTile( LandscapeTile *_tile, float _dX, float _dZ )
{    
    for( int i = 0; i < g_app->m_location->m_levelFile->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_levelFile->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_levelFile->m_buildings[i];
            if( building->m_pos.x >= _tile->m_posX &&
                building->m_pos.z >= _tile->m_posZ &&
                building->m_pos.x <= _tile->m_posX + _tile->m_size &&
                building->m_pos.z <= _tile->m_posZ + _tile->m_size )
            {
                building->m_pos.x += _dX;
                building->m_pos.z += _dZ;
            }
        }
    }

}


void LocationEditor::AdvanceModeLandTile()
{
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();

	int newSelectionId = -1;
	if ( g_inputManager->controlEvent( ControlTileSelect ) )
	{
		newSelectionId = IsPosInLandTile(mousePos3D);
	}

	if (m_selectionId == -1)
	{
		// No selection

		Landscape *land = &g_app->m_location->m_landscape;
		LList<LandscapeTile *> *tiles = &g_app->m_location->m_levelFile->m_landscape.m_tiles;
		
		// Has the user selected a tile
		if (newSelectionId != -1) 
		{
			LandscapeTile *tile = tiles->GetData(newSelectionId);
			m_tool = ToolMove;
			m_selectionId = newSelectionId;
			m_newLandscapeX = tile->m_posX;
			m_newLandscapeZ = tile->m_posZ;
			m_waitingForRelease = true;

			EclWindow *cw = EclGetWindow(LANGUAGEPHRASE("editor_landscape"));
			DarwiniaDebugAssert(cw);
			LandscapeTileEditWindow *ew = new LandscapeTileEditWindow(LANGUAGEPHRASE("editor_landscapetile"), newSelectionId);
			ew->m_w = cw->m_w;
			ew->m_h = 150;
			ew->m_x = cw->m_x;
			EclRegisterWindow(ew);
			ew->m_y = cw->m_y - ew->m_h - 10;
		}
	}
	
	if (m_selectionId != -1)
	{
		// There is a current selection

		if( g_inputManager->controlEvent( ControlTileDrop ) )
		{
			// Move the selected landscape to the new position and regenerate it
			LandscapeTile *tileDef = g_app->m_location->m_levelFile->m_landscape.m_tiles.GetData(m_selectionId);
			if( m_newLandscapeX != tileDef->m_posX ||
				m_newLandscapeZ != tileDef->m_posZ )
			{
                if( m_moveBuildingsWithLandscape )
                {
                    MoveBuildingsInTile( tileDef, m_newLandscapeX - tileDef->m_posX, m_newLandscapeZ - tileDef->m_posZ );
                }

				tileDef->m_posX = m_newLandscapeX;
				tileDef->m_posZ = m_newLandscapeZ;
				LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
				g_app->m_location->m_landscape.Init(def);
			}
		}
		if( g_inputManager->controlEvent( ControlTileSelect ) ) // TODO: Should this be ControlTileGrab?
		{
			if (newSelectionId == m_selectionId)
			{
				// The user "grabs" the landscape at this position
				LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
				LandscapeTile *tileDef = g_app->m_location->m_levelFile->m_landscape.m_tiles.GetData(m_selectionId);
				m_landscapeGrabX = mousePos3D.x - tileDef->m_posX;
				m_landscapeGrabZ = mousePos3D.z - tileDef->m_posZ;
			}
			else
			{
				// The user has deselected the landscape
				m_tool = ToolNone;
				m_selectionId = -1;
				m_waitingForRelease = true;
				EclRemoveWindow(LANGUAGEPHRASE("editor_landscapetile"));
                EclRemoveWindow(LANGUAGEPHRASE("editor_guidegrid"));
			}
		}
		else if ( g_inputManager->controlEvent( ControlTileDrag ) )
		{
			// The user "drags" the landscape around
			m_newLandscapeX = mousePos3D.x - m_landscapeGrabX;
			m_newLandscapeZ = mousePos3D.z - m_landscapeGrabZ;        
		}
	}
}


void LocationEditor::AdvanceModeLandFlat()
{
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();

	int newSelectionId = -1;
	if ( g_inputManager->controlEvent( ControlTileSelect ) )
	{
		Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
		newSelectionId = IsPosInFlattenArea(mousePos);
	}

	if (m_selectionId == -1)
	{
		// If there isn't currently any selection, then check for a new one
		if (newSelectionId != -1)
		{
			m_selectionId = newSelectionId;
			m_waitingForRelease = true;
			
			EclWindow *cw = EclGetWindow(LANGUAGEPHRASE("editor_landscape"));
			DarwiniaDebugAssert(cw);
			LandscapeFlattenAreaEditWindow *ew = new LandscapeFlattenAreaEditWindow("Flatten Area", newSelectionId);
			ew->m_w = cw->m_w;
			ew->m_h = 100;
			ew->m_x = 0;
			EclRegisterWindow(ew);
			ew->m_y = cw->m_y - ew->m_h - 10;
		}
	}
	else
	{
		Location *location = g_app->m_location;

		if ( g_inputManager->controlEvent( ControlTileSelect ) )
		{
			if (newSelectionId == m_selectionId)
			{
				// The user "grabs" the landscape at this position
				LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
				LandscapeFlattenArea *areaDef = g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.GetData(m_selectionId);
				m_landscapeGrabX = mousePos3D.x - areaDef->m_centre.x;
				m_landscapeGrabZ = mousePos3D.z - areaDef->m_centre.z;
			}
			else
			{
				// The user has deselected the flatten area
				m_selectionId = newSelectionId;
				m_waitingForRelease = true;
				EclRemoveWindow("Flatten Area");
			}
		}
		else if ( g_inputManager->controlEvent( ControlTileDrag ) )
		{
			// The user "drags" the flatten area around
			LandscapeFlattenArea *areaDef = g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.GetData(m_selectionId);
			areaDef->m_centre.x = mousePos3D.x - m_landscapeGrabX;
			areaDef->m_centre.z = mousePos3D.z - m_landscapeGrabZ;
		}
	}
}


void LocationEditor::AdvanceModeBuilding()
{
	BuildingEditWindow *ew = (BuildingEditWindow *)EclGetWindow(LANGUAGEPHRASE("editor_buildingid"));
	if (ew && EclMouseInWindow(ew))	return;
	
	Camera *cam = g_app->m_camera;

	// Find the ID of the building the user is clicking on
	int newSelectionId = -1;
	if ( g_inputManager->controlEvent( ControlTileSelect ) )
	{
		Vector3 rayStart, rayDir;
		cam->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
		newSelectionId = DoesRayHitBuilding(rayStart, rayDir);
	}

	if (m_selectionId == -1)
	{
		// If there isn't currently any selection, then check for a new one
		if (newSelectionId != -1)
		{
			m_selectionId = newSelectionId;
			BuildingsCreateWindow *cw = (BuildingsCreateWindow*)EclGetWindow(LANGUAGEPHRASE("editor_buildings"));
			DarwiniaDebugAssert(!ew);
			DarwiniaDebugAssert(cw);
			BuildingEditWindow *bew = new BuildingEditWindow(LANGUAGEPHRASE("editor_buildingid"));
			bew->m_w = cw->m_w;
			bew->m_h = 140;
			bew->m_x = cw->m_x;
			EclRegisterWindow(bew);
			bew->m_y = cw->m_y - bew->m_h - 10;
			m_waitingForRelease = true;

            Location *location = g_app->m_location;
		    Building *building = location->GetBuilding(m_selectionId);            
            if( building->m_type == Building::TypeTree )
            {
                TreeWindow *tw = new TreeWindow( LANGUAGEPHRASE("editor_treeditor") );
                tw->m_w = cw->m_w;
                tw->m_h = 230;
                tw->m_y = bew->m_y - tw->m_h - 10;
                tw->m_x = bew->m_x;
                EclRegisterWindow( tw );
            }
		}
	}
	else 
	{
		Location *location = g_app->m_location;
		Building *building = location->GetBuilding(m_selectionId);
	
		
		if ( g_inputManager->controlEvent( ControlTileSelect ) )                  // If left mouse is clicked then consider creating a new link
		{
			if (newSelectionId == -1)
			{
				EclRemoveWindow(LANGUAGEPHRASE("editor_buildingid"));
				m_selectionId = -1;
			}
			else if (m_tool == ToolLink)
			{
				building->SetBuildingLink(newSelectionId);
				m_tool = ToolNone;
			}
		}		
		else if ( g_inputManager->controlEvent( ControlTileDrag ) && newSelectionId == -1 )  // Otherwise consider rotation and movement
		{
			switch (m_tool)
			{
				case ToolMove:
				{
					Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
					building->m_pos = mousePos;
					break;
				}
				case ToolRotate:
				{
					Vector3 front = building->m_front;
					front.RotateAroundY((float)g_target->dX() * 0.05f);
					building->m_front = front;
					break;
				}
			}
		}
	}
}


void LocationEditor::AdvanceModeLight()
{
	Location *location = g_app->m_location;

	if (location->m_lights.ValidIndex(m_selectionId) &&  g_inputManager->controlEvent( ControlTileDrag ) )
	{
		Light *worldLight = location->m_lights.GetData(m_selectionId);
		Vector3 front(worldLight->m_front[0], worldLight->m_front[1], worldLight->m_front[2]);
		front.RotateAroundY((float)g_target->dX() * 0.05f);
		worldLight->SetFront(front);
	}
}


void LocationEditor::AdvanceModeInstantUnit()
{
	Camera *cam = g_app->m_camera;

	int newSelectionId = -1;
	if ( g_inputManager->controlEvent( ControlTileSelect ) ) // TODO: Should be something else?
	{
		Vector3 rayStart, rayDir;
		cam->GetClickRay( g_target->X(), g_target->Y(), &rayStart, &rayDir );
		newSelectionId = DoesRayHitInstantUnit(rayStart, rayDir);
	}

	if (m_selectionId == -1)
	{
		if (newSelectionId != -1)
		{
			m_selectionId = newSelectionId;
			EclWindow *cw = EclGetWindow(LANGUAGEPHRASE("editor_instantunits"));
			DarwiniaDebugAssert(cw);
			InstantUnitEditWindow *ew = new InstantUnitEditWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
			ew->m_w = cw->m_w;
			ew->m_h = 160;
			ew->m_x = cw->m_x;
			EclRegisterWindow(ew);
			ew->m_y = cw->m_y - ew->m_h - 10;
			m_waitingForRelease = true;
		}
	}
	else 
	{
		if ( g_inputManager->controlEvent( ControlTileSelect ) ) // TODO: Something else?
		{
			if (newSelectionId != m_selectionId)
			{
				m_selectionId = -1;
				EclRemoveWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
			}
		}
		else if ( g_inputManager->controlEvent( ControlTileDrag ) ) // TODO: Something else?
		{
			InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(m_selectionId);
			switch (m_tool)
			{
				case ToolMove:
				{
					Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
					iu->m_posX = mousePos.x;
					iu->m_posZ = mousePos.z;
					break;
				}
			}
		}
	}
}


void LocationEditor::AdvanceModeCameraMount()
{
  	if ( g_inputManager->controlEvent( ControlTileSelect ) )
	{
		CameraAnimSecondaryEditWindow *win = (CameraAnimSecondaryEditWindow*)
												EclGetWindow(LANGUAGEPHRASE("editor_cameraanim"));
		if (win && win->m_newNodeArmed)
		{
			Vector3 rayStart, rayDir;
			g_app->m_camera->GetClickRay( g_target->X(), 
										  g_target->Y(),
										  &rayStart, &rayDir );
			int mountId = DoesRayHitCameraMount(rayStart, rayDir);
			if (mountId != -1)
			{
				CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[mountId];
				DarwiniaDebugAssert(mount);
				
				CameraAnimation *anim = g_app->m_location->m_levelFile->m_cameraAnimations[m_selectionId];
				DarwiniaDebugAssert(anim);
				
				CamAnimNode *node = new CamAnimNode;
				node->m_mountName = strdup(mount->m_name);

				anim->m_nodes.PutData(node);

				win->m_newNodeArmed = false;
				win->RemoveButtons();
				win->AddButtons();
			}
		}
	}
}


void LocationEditor::Advance()
{
    if( !EclGetWindow( g_target->X(), g_target->Y() ) )
    {
	    if (m_waitingForRelease &&
			( g_inputManager->controlEvent( ControlTileDrag ) ||
			  g_inputManager->controlEvent( ControlTileDrop ) )) // TODO: Something else?
		{
			return;
		}
	    m_waitingForRelease = false;

		switch (m_mode)
		{
			case ModeNone:				AdvanceModeNone();				break;
			case ModeLandTile:			AdvanceModeLandTile();			break;
			case ModeLandFlat:			AdvanceModeLandFlat();			break;
			case ModeBuilding:			AdvanceModeBuilding();			break;
			case ModeLight:				AdvanceModeLight();				break;
			case ModeInstantUnit:		AdvanceModeInstantUnit();		break;
			case ModeCameraMount:		AdvanceModeCameraMount();		break;
		}
    }
}


void LocationEditor::RenderUnit(InstantUnit *_iu)
{
	char *typeName = Entity::GetTypeName(_iu->m_type);
	
	float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(_iu->m_posX, _iu->m_posZ);
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_editorFont.DrawText3DCentre(Vector3(_iu->m_posX, landHeight + 15.0f, _iu->m_posZ),
								  15.0f, "%d %s(s)", _iu->m_number, typeName);


	// Render troops
	int maxX = (int)sqrtf(_iu->m_number);
	int maxZ = _iu->m_number / maxX;
	float pitch = 10.0f;
	float offsetX = -maxX * pitch * 0.5f;
	float offsetZ = -maxZ * pitch * 0.5f;
	RGBAColour colour;
    if( _iu->m_teamId >= 0 ) colour = g_app->m_location->m_teams[_iu->m_teamId].m_colour;
	colour.a = 200;
    glColor4ubv(colour.GetData());
    
    Vector3 camUp = g_app->m_camera->GetUp() * 5.0f;
    Vector3 camRight = g_app->m_camera->GetRight() * 5.0f;

    glDisable   (GL_CULL_FACE );
    glEnable    (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE);
    glDepthMask (false);
	glBegin     (GL_QUADS);
    
    //
    // Render dots for the number and team of the unit

	for (int x = 0; x < maxX; ++x)
	{
		for (int z = 0; z < maxZ; ++z)
		{
			Vector3 pos(_iu->m_posX + offsetX + x * pitch, 0,
						_iu->m_posZ + offsetZ + z * pitch);
			pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 2.0f;
			glVertex3fv( (pos - camUp - camRight).GetData() );
            glVertex3fv( (pos - camUp + camRight).GetData() );
            glVertex3fv( (pos + camUp + camRight).GetData() );
            glVertex3fv( (pos + camUp - camRight).GetData() );
		}
	}

    glEnd();


    //
    // Render our spread circle

    if( m_mode == ModeInstantUnit )
    {
        int numSteps = 30;
        float angle = 0.0f;

        colour.a = 100;
        glColor4ubv(colour.GetData() );
        glLineWidth( 2.0f );
        glBegin( GL_LINE_LOOP );
        for( int i = 0; i <= numSteps; ++i )
        {
            float xDiff = _iu->m_spread * sinf(angle);
            float zDiff = _iu->m_spread * cosf(angle);
            Vector3 pos = Vector3(_iu->m_posX, 0.0f, _iu->m_posZ) + Vector3(xDiff,5,zDiff);
	        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 10.0f;
            if( pos.y < 2 ) pos.y = 2;
            glVertex3fv( pos.GetData() );
            angle += 2.0f * M_PI / (float) numSteps;
        }
        glEnd();
    }
    

    glDisable   (GL_BLEND);
    glDepthMask (true);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable    (GL_CULL_FACE );

}


void LocationEditor::RenderModeLandTile()
{
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	Landscape *land = &g_app->m_location->m_landscape;

	// Highlight any tile under our mouse cursor
	LList<LandscapeTile *> *tiles = &g_app->m_location->m_levelFile->m_landscape.m_tiles;
	for (int i = 0; i < tiles->Size(); ++i)
	{
		if (i == m_selectionId) continue;

		LandscapeTile *tile = tiles->GetData(i);
		float worldX = tile->m_posX - tile->m_heightMap->m_cellSizeX;
		float worldZ = tile->m_posZ - tile->m_heightMap->m_cellSizeY;
		float sizeX = tile->m_size;
		float sizeY = tile->m_desiredHeight - tile->m_outsideHeight;
		float sizeZ = tile->m_size;
		if (mousePos3D.x > worldX &&
			mousePos3D.x < worldX + sizeX &&
			mousePos3D.z > worldZ &&
			mousePos3D.z < worldZ + sizeZ) 
		{
			Vector3 centre(worldX, tile->m_outsideHeight, worldZ);
			centre.x += sizeX * 0.5f;
			centre.y += sizeY * 0.5f;
			centre.z += sizeZ * 0.5f;
			RenderCube(centre, sizeX, sizeY, sizeZ, RGBAColour(128,255,128,99));
		}
	}

    // Render guide grids

    if( EclGetWindow(LANGUAGEPHRASE("editor_guidegrid")) )
    {
        glEnable( GL_LINE_SMOOTH );
        glEnable( GL_BLEND );

	    if (m_selectionId != -1)
	    {
            LandscapeTile *tile = tiles->GetData(m_selectionId);

            if( tile->m_guideGrid &&
                tile->m_guideGrid->GetNumColumns() > 0 )
            {
                float gridCellSize = tile->m_size / (float) (tile->m_guideGrid->GetNumColumns()+1);
                for( int x = 0; x < tile->m_guideGrid->GetNumColumns()-1; ++x )
                {
                    for( int z = 0; z < tile->m_guideGrid->GetNumColumns()-1; ++z )
                    {                    
                        float value1 = tile->m_desiredHeight * (tile->m_guideGrid->GetData( x, z ) / 256.0f);
                        float value2 = tile->m_desiredHeight * (tile->m_guideGrid->GetData( x+1, z ) / 256.0f );
                        float value3 = tile->m_desiredHeight * (tile->m_guideGrid->GetData( x+1, z+1 ) / 256.0f );
                        float value4 = tile->m_desiredHeight * (tile->m_guideGrid->GetData( x, z+1 ) / 256.0f );
                 
                        float tileX = tile->m_posX + (x+1) * gridCellSize;
                        float tileZ = tile->m_posZ + (z+1) * gridCellSize;
                        float tileW = gridCellSize;
                        float tileH = gridCellSize;

                        glDisable( GL_DEPTH_TEST );
                        glLineWidth( 1.0f );
                        glColor4f( 1.0f, 0.0f, 0.0f, 0.2f );
                        glBegin( GL_LINE_LOOP );
                            glVertex3f( tileX, tile->m_posY + value1, tileZ );
                            glVertex3f( tileX + tileW, tile->m_posY + value2, tileZ );
                            glVertex3f( tileX + tileW, tile->m_posY + value3, tileZ + tileH );
                            glVertex3f( tileX, tile->m_posY + value4, tileZ + tileH );
                        glEnd();

                        glEnable( GL_DEPTH_TEST );
                        glColor4f( 1.0f, 0.0f, 0.0f, 0.8f );
                        glLineWidth( 3.0f );
                        glBegin( GL_LINE_LOOP );
                            glVertex3f( tileX, tile->m_posY + value1, tileZ );
                            glVertex3f( tileX + tileW, tile->m_posY + value2, tileZ );
                            glVertex3f( tileX + tileW, tile->m_posY + value3, tileZ + tileH );
                            glVertex3f( tileX, tile->m_posY + value4, tileZ + tileH );
                        glEnd();

                    }
                }
            }
        }

        glDisable( GL_BLEND );
        glEnable( GL_DEPTH_TEST );
    }

    
	// Render a green box around the currently selected tile (if any)
	if (m_selectionId != -1)
	{
        LandscapeTile *tile = tiles->GetData(m_selectionId);
        float x = tile->m_posX - tile->m_heightMap->m_cellSizeX;
        float y = tile->m_outsideHeight;
        float z = tile->m_posZ - tile->m_heightMap->m_cellSizeY;
        float sX = tile->m_size;
        float sY = tile->m_desiredHeight - tile->m_outsideHeight;
		float sZ = tile->m_size;
		Vector3 centre(x, y, z);
		centre.x += sX * 0.5f;
		centre.y += sY * 0.5f;
		centre.z += sZ * 0.5f;
		RenderCube(centre, sX, sY, sZ, RGBAColour(128,255,128));

        if( m_newLandscapeX != tile->m_posX ||
            m_newLandscapeZ != tile->m_posZ )
        {
            x = m_newLandscapeX;
			y = 1.0f;
            z = m_newLandscapeZ;
            glColor3ub( 0, 0, 255 );
            glBegin( GL_LINE_LOOP );
                glVertex3f( x, y, z );
                glVertex3f( x + sX, y, z );
                glVertex3f( x + sX, y, z + sZ );
                glVertex3f( x, y, z + sZ );
            glEnd();
        }
	}
    
	CHECK_OPENGL_STATE();
}


void LocationEditor::RenderModeLandFlat()
{
	Vector3 mousePos3D = g_app->m_userInput->GetMousePos3d();
	Landscape *land = &g_app->m_location->m_landscape;

	// Highlight any flatten area under our mouse cursor
	LList<LandscapeFlattenArea *> *areas = &g_app->m_location->m_levelFile->m_landscape.m_flattenAreas;
	for (int i = 0; i < areas->Size(); ++i)
	{
		if (i == m_selectionId) continue;

		LandscapeFlattenArea *area = areas->GetData(i);
		float worldX = area->m_centre.x;
		float worldZ = area->m_centre.z;
		float sizeX = area->m_size;
		float sizeY = area->m_centre.y;
		float sizeZ = area->m_size;
		float x = area->m_centre.x;
		float y = area->m_centre.y;
		float z = area->m_centre.z;
		float s = area->m_size * 2.0f;
		Vector3 centre(x, y, z);

		RenderCube(centre, s, y + 20, s, RGBAColour(128,255,128,99));
	}

	if (m_selectionId != -1)
	{
		LandscapeDef *landscapeDef = &(g_app->m_location->m_levelFile->m_landscape);
		LandscapeFlattenArea *areaDef = g_app->m_location->m_levelFile->m_landscape.m_flattenAreas.GetData(m_selectionId);
		float x = areaDef->m_centre.x;
		float y = areaDef->m_centre.y;
		float z = areaDef->m_centre.z;
		float s = areaDef->m_size * 2.0f;
		Vector3 centre(x, y, z);

		RenderCube(centre, s, y + 20, s, RGBAColour(128,255,128,255));
	}
	
	CHECK_OPENGL_STATE();
}


void LocationEditor::RenderModeBuilding()
{
	if (m_selectionId != -1)
	{
		Building *building = g_app->m_location->GetBuilding(m_selectionId);

        if( building->m_shape )
        {
            Matrix34 mat( building->m_front, g_upVector, building->m_pos );
            Vector3 centrePos = building->m_shape->CalculateCentre( mat );
            float radius = building->m_shape->CalculateRadius( mat, centrePos );
            RenderSphere( centrePos, radius, RGBAColour(255,255,255,255) );
        }
        else
        {
            building->RenderHitCheck();
        }

		if (m_tool == ToolLink)
		{
			Vector3     height(0,10,0);
			Vector3     mousePos(g_app->m_userInput->GetMousePos3d());
			Vector3     arrowDir(mousePos - building->m_pos);
			Vector3     arrowSize(0,3,0);

			glEnable    ( GL_LINE_SMOOTH );
			glDisable   ( GL_DEPTH_TEST );
			glLineWidth ( 1.0f );
			glColor3f   ( 1.0f, 0.5f, 0.5f );
			glBegin( GL_LINES );
				glVertex3fv( (building->m_pos+height).GetData() );
				glVertex3fv( (mousePos).GetData() );

				glVertex3fv( (mousePos).GetData() );
				glVertex3fv( (mousePos-arrowDir*0.1f+arrowSize).GetData() );

				glVertex3fv( (mousePos).GetData() );
				glVertex3fv( (mousePos-arrowDir*0.1f-arrowSize).GetData() );
			glEnd();
			glDisable   ( GL_LINE_SMOOTH );
			glEnable    ( GL_DEPTH_TEST );
		}
		
		CHECK_OPENGL_STATE();
    }
}


void LocationEditor::RenderModeInstantUnit()
{
	if (m_selectionId != -1)
	{
		InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(m_selectionId);
		Vector3 pos(iu->m_posX, 0, iu->m_posZ);
		pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
		RenderSphere(pos, sqrtf(iu->m_number) * INSTANT_UNIT_SIZE_FACTOR);	
	}
}


void LocationEditor::RenderModeCameraMount()
{
	RGBAColour bright(255,255,0);
	RGBAColour dim(90,90,0);

	LList <CameraAnimation*> *list = &g_app->m_location->m_levelFile->m_cameraAnimations;
	for (int i = 0; i < list->Size(); ++i)
	{
		CameraAnimation *anim = list->GetData(i);
		CamAnimNode *lastNode = anim->m_nodes.GetData(0);
		for (int j = 1; j < anim->m_nodes.Size(); ++j)
		{
			CamAnimNode *node = anim->m_nodes.GetData(j);
			if (stricmp(node->m_mountName, MAGIC_MOUNT_NAME_START_POS) == 0 ||
				stricmp(lastNode->m_mountName, MAGIC_MOUNT_NAME_START_POS) == 0)
			{
				continue;
			}

			CameraMount *mount1 = g_app->m_location->m_levelFile->GetCameraMount(lastNode->m_mountName);
			CameraMount *mount2 = g_app->m_location->m_levelFile->GetCameraMount(node->m_mountName);
			if (i == m_selectionId)
			{
				RenderArrow(mount1->m_pos, mount2->m_pos, 1.0f, bright);
			}
			else
			{
				RenderArrow(mount1->m_pos, mount2->m_pos, 1.0f, dim);
			}

			lastNode = node;
		}
	}
}


void LocationEditor::Render()
{
    //
    // Render our buildings
	
	g_app->m_renderer->SetObjectLighting();
    LevelFile *levelFile = g_app->m_location->m_levelFile;
    for( int i = 0; i < levelFile->m_buildings.Size(); ++i )
    {
        if( levelFile->m_buildings.ValidIndex(i) )
        {
            Building *b = levelFile->m_buildings.GetData(i);
            b->Render(0.0f);
        }
    } 
    g_app->m_renderer->UnsetObjectLighting();
	if (m_mode == ModeBuilding)
	{
		for( int i = 0; i < levelFile->m_buildings.Size(); ++i )
		{
			if( levelFile->m_buildings.ValidIndex(i) )
			{
				Building *b = levelFile->m_buildings.GetData(i);
				b->RenderAlphas(0.0f);
				b->RenderLink();
			}
		}
	}


	// 
	// Render camera mounts

	{
		g_app->m_renderer->SetObjectLighting();
		Shape *camShape = g_app->m_resource->GetShape("camera.shp");
		Matrix34 mat;
		
		for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
		{
			CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
			Vector3 camToMount = g_app->m_camera->GetPos() - mount->m_pos;
			if (camToMount.Mag() < 20.0f) continue;
			mat.OrientFU(mount->m_front, mount->m_up);
			mat.pos = mount->m_pos;
			camShape->Render(0.0f, mat);
		}
	    g_app->m_renderer->UnsetObjectLighting();
	}


    g_app->m_renderer->UnsetObjectLighting();


	// 
	// Render our instant units
	
	for (int i = 0; i < levelFile->m_instantUnits.Size(); ++i)
	{
		InstantUnit *iu = levelFile->m_instantUnits.GetData(i);
		RenderUnit(iu);
	}

	switch (m_mode)
	{
		case ModeLandTile:			RenderModeLandTile();				break;
		case ModeLandFlat:			RenderModeLandFlat();				break;
		case ModeBuilding:			RenderModeBuilding();				break;
		case ModeLight:													break;
		case ModeInstantUnit:		RenderModeInstantUnit();			break;
		case ModeCameraMount:		RenderModeCameraMount();			break;
	}

	
	//
	// Render axes

	float sizeX = g_app->m_location->m_landscape.GetWorldSizeX() / 2;
	float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ() / 2;
	RenderArrow(Vector3(10, 200, 0), Vector3(sizeX, 200, 0), 4.0f);
	glBegin(GL_LINES);
		glVertex3f(sizeX,      250, 0);
		glVertex3f(sizeX + 90, 150, 0);
		glVertex3f(sizeX,      150, 0);
		glVertex3f(sizeX + 90, 250, 0);
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);

	RenderArrow(Vector3(0, 200, 10), Vector3(0, 200, sizeZ), 4.0f);
	glBegin(GL_LINES);
		glVertex3f(0, 250, sizeZ);
		glVertex3f(0, 250, sizeZ + 90);
		glVertex3f(0, 250, sizeZ);
		glVertex3f(0, 150, sizeZ + 90);
		glVertex3f(0, 150, sizeZ);
		glVertex3f(0, 150, sizeZ + 90);
	glEnd();
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);


    //
    // Render targetting crosshair

    g_app->m_renderer->SetupMatricesFor2D();
	glColor3ub(255,255,255);
    glLineWidth( 1.0f );
    glBegin( GL_LINES );
        glVertex2i( g_app->m_renderer->ScreenW()/2, g_app->m_renderer->ScreenH()/2 - 30 );
        glVertex2i( g_app->m_renderer->ScreenW()/2, g_app->m_renderer->ScreenH()/2 + 30 );

        glVertex2i( g_app->m_renderer->ScreenW()/2 - 30, g_app->m_renderer->ScreenH()/2 );
        glVertex2i( g_app->m_renderer->ScreenW()/2 + 30, g_app->m_renderer->ScreenH()/2 );
    glEnd();
    g_app->m_renderer->SetupMatricesFor3D();
    

	CHECK_OPENGL_STATE();
}

#endif

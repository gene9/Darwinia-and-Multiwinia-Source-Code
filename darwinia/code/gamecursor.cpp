#include "lib/universal_include.h"
#include "lib/bitmap.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/binary_stream_readers.h"

#include <eclipse.h>

#include "app.h"
#include "global_world.h"
#include "main.h"
#include "gamecursor.h"
#include "renderer.h"
#include "user_input.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "camera.h"
#include "location_input.h"
#include "entity_grid.h"
#include "sepulveda.h"

#include "worldobject/armour.h"
#include "worldobject/radardish.h"
#include "worldobject/insertion_squad.h"


GameCursor::GameCursor()
:	m_selectionArrowBoost(0.0f),
	m_highlightingSomething(false),
	m_validPlacementOpportunity(false),
	m_moveableEntitySelected(false)
{
    m_cursorStandard = new MouseCursor( "icons/mouse_main.bmp" );
	m_cursorStandard->SetHotspot(0.055f, 0.070f);
    m_cursorStandard->SetSize(25.0f);

    m_cursorPlacement = new MouseCursor( "icons/mouse_placement.bmp" );
	m_cursorPlacement->SetHotspot(0.5f, 0.5f);
	m_cursorPlacement->SetSize(40.0f);

    m_cursorDisabled = new MouseCursor( "icons/mouse_disabled.bmp" );
	m_cursorDisabled->SetHotspot(0.5f, 0.5f);
	m_cursorDisabled->SetSize(60.0f);
    m_cursorDisabled->SetColour(RGBAColour(255,0,0,255) );

    m_cursorMoveHere = new MouseCursor( "icons/mouse_movehere.bmp" );
    m_cursorMoveHere->SetHotspot( 0.5f, 0.5f);
    m_cursorMoveHere->SetSize(30.0f);
    m_cursorMoveHere->SetAnimation( true );
    m_cursorMoveHere->SetColour(RGBAColour(255,255,150,255) );

    m_cursorHighlight = new MouseCursor( "icons/mouse_highlight.bmp" );
    m_cursorHighlight->SetHotspot( 0.5f, 0.5f );
    m_cursorHighlight->SetAnimation( true );

    m_cursorTurretTarget = new MouseCursor( "icons/mouse_turrettarget.bmp" );
    m_cursorTurretTarget->SetHotspot( 0.5f, 0.5f );
    m_cursorTurretTarget->SetColour(RGBAColour(255,255,255,255) );
    m_cursorTurretTarget->SetShadowed(false);

    m_cursorSelection = new MouseCursor( "icons/mouse_selection.bmp" );
    m_cursorSelection->SetHotspot( 0.5f, 0.5f );
    
	m_cursorMissile = NULL;

    //
    // Load selection arrow graphic
    
	sprintf( m_selectionArrowFilename, "icons/selectionarrow.bmp" );

    BinaryReader *binReader = g_app->m_resource->GetBinaryReader( m_selectionArrowFilename );
	DarwiniaReleaseAssert(binReader, "Failed to open mouse cursor resource %s", m_selectionArrowFilename ); 
    BitmapRGBA bmp( binReader, "bmp" );
	SAFE_DELETE(binReader);

	g_app->m_resource->AddBitmap(m_selectionArrowFilename, bmp);
	
	sprintf(m_selectionArrowShadowFilename, "shadow_%s", m_selectionArrowFilename);
    bmp.ApplyBlurFilter( 10.0f );
	g_app->m_resource->AddBitmap(m_selectionArrowShadowFilename, bmp);

}

GameCursor::~GameCursor()
{
	SAFE_DELETE(m_cursorStandard);
	SAFE_DELETE(m_cursorPlacement);
	SAFE_DELETE(m_cursorDisabled);
	SAFE_DELETE(m_cursorMoveHere);
	SAFE_DELETE(m_cursorHighlight);
	SAFE_DELETE(m_cursorTurretTarget);
	SAFE_DELETE(m_cursorSelection);
	SAFE_DELETE(m_cursorMissile);
}

bool GameCursor::GetSelectedObject( WorldObjectId &_id, Vector3 &_pos )
{
    Team *team = g_app->m_location->GetMyTeam();

    if( team )
    {
        Entity *selectedEnt = team->GetMyEntity();
        if( selectedEnt )
        {
            _pos = selectedEnt->m_pos + selectedEnt->m_centrePos + selectedEnt->m_vel * g_predictionTime;
            _id = selectedEnt->m_id;
            return true;
        }
        else if( team->m_currentBuildingId != -1 )
        {
            Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
            if( building )
            {
                _pos = building->m_centrePos;
                _id = building->m_id;
                return true;
            }
        }
        else
        {
            Unit *selected = team->GetMyUnit();
            if( selected )
            {       
                _pos = selected->m_centrePos + selected->m_vel * g_predictionTime;                
                _id.Set( selected->m_teamId, selected->m_unitId, -1, -1 );

                // Add the centre pos
                for( int i = 0; i < selected->m_entities.Size(); ++i )
                {
                    if( selected->m_entities.ValidIndex(i) )
                    {
                        Entity *ent = selected->m_entities[i];
                        _pos += ent->m_centrePos;
                        break;
                    }
                }

                return true;
            }
        }
    }

    return false;
}


bool GameCursor::GetHighlightedObject( WorldObjectId &_id, Vector3 &_pos, float &_radius )
{    
    WorldObjectId id;
    bool somethingHighlighted = false;
    bool found = false;

    if( !g_app->m_taskManagerInterface->m_visible )
    {
        somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
    }
    else
    {
        Task *task = g_app->m_taskManager->GetTask( g_app->m_taskManagerInterface->m_highlightedTaskId );
        if( task && task->m_objId.IsValid() )
        {
            id = task->m_objId;
            somethingHighlighted = true;
        }
    }

    
    if( somethingHighlighted )
    {
        if( id.GetUnitId() == UNIT_BUILDINGS )
        {
            // Found a building
            Building *building = g_app->m_location->GetBuilding( id.GetUniqueId() );
            if( building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeBridge ||
                building->m_type == Building::TypeGunTurret ||
                building->m_type == Building::TypeFenceSwitch )
            {
                _id = id;
                _pos = building->m_centrePos;
                _radius = building->m_radius;
                found = true;
                if( building->m_type == Building::TypeGunTurret )
                {
                    _pos.y += _radius * 0.5f;
                }
            }
        }
        else if( id.GetIndex() == -1 )
        {
            // Found a unit
            Unit *unit = g_app->m_location->GetUnit( id );
            if( unit )
            {
                _id = id;
                _pos = unit->m_centrePos + unit->m_vel * g_predictionTime;
                _radius = unit->m_radius;
                found = true;

                // Add the centre pos
                for( int i = 0; i < unit->m_entities.Size(); ++i )
                {
                    if( unit->m_entities.ValidIndex(i) )
                    {
                        Entity *ent = unit->m_entities[i];
                        _pos += ent->m_centrePos;
                        break;
                    }
                }
            }
        }
        else
        {
            // Found an entity
            Entity *entity = g_app->m_location->GetEntity(id);
            _id = id;
            _pos = entity->m_pos + entity->m_vel * g_predictionTime + entity->m_centrePos;
            _radius = entity->m_radius*1.5f;
            if( entity->m_type == Entity::TypeDarwinian ) _radius = entity->m_radius*2.0f;
            found = true;
        }
    }    

    return found;
}


void GameCursor::CreateMarker( Vector3 const &_pos )
{
    Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(_pos.x, _pos.z);
    Vector3 front = (landNormal ^ g_upVector).Normalise();

    MouseCursorMarker *marker = new MouseCursorMarker();
    marker->m_pos = _pos;
    marker->m_front = front;
    marker->m_up = landNormal;
    marker->m_startTime = GetHighResTime();

    m_markers.PutData( marker );
}


void GameCursor::BoostSelectionArrows( float _seconds )
{
	m_selectionArrowBoost = max( m_selectionArrowBoost, _seconds );
}


void GameCursor::RenderMarkers()
{
    for( int i = 0; i < m_markers.Size(); ++i )
    {
        MouseCursorMarker *marker = m_markers[i];
        float timeSync = GetHighResTime() - marker->m_startTime;
        if( timeSync > 0.5f )
        {
            m_markers.RemoveData(i);
            delete marker;
            --i;
        }
        else
        {
            m_cursorPlacement->SetSize( 20.0f - timeSync * 40.0f );
            m_cursorPlacement->SetAnimation( false );
            m_cursorPlacement->Render3D( marker->m_pos, marker->m_front, marker->m_up, false );
        }
    }
}


void GameCursor::Render()
{        
    START_PROFILE( g_app->m_profiler, "Render GameCursor" );

	float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f,
							 			   g_app->m_renderer->GetFarPlane());

	int screenX = g_target->X();
	int screenY = g_target->Y();
    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
    mousePos.y = max( 1.0f, mousePos.y );
        
    bool cursorRendered = false;
	bool chatLog = g_app->m_sepulveda->ChatLogVisible();

	m_highlightingSomething = false;
	m_validPlacementOpportunity = false;
	m_moveableEntitySelected = false;

	// Set mip mapping for game cursor
	glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    
    if( g_app->m_editing || EclGetWindows()->Size() > 0 )
    {
        // Editing
    }
    else if( chatLog )
    {
        // Reading Sepulveda's history
    }
    else if( !g_app->m_camera->IsInteractive() )
    {
        // Cut scene of some sort, no player control, so no cursor
        cursorRendered = true;
    }
    else if( !g_app->m_location )
    {
        // We are in the global world        
        GlobalLocation *highlightedLocation = g_app->m_globalWorld->GetHighlightedLocation();
        bool locAvailable = highlightedLocation && 
                            strcmp(highlightedLocation->m_missionFilename, "null" ) != 0 &&
                            highlightedLocation->m_available;
        g_app->m_renderer->SetupMatricesFor2D();
        m_cursorPlacement->SetAnimation( locAvailable );
        m_cursorPlacement->SetSize( 40.0f );
        m_cursorPlacement->Render(screenX,screenY);
        if( !locAvailable ) m_cursorDisabled->Render(screenX,screenY);
        g_app->m_renderer->SetupMatricesFor3D();
        cursorRendered = true;
    }
    else if( g_app->m_location )
    {
        // We are at a location
	    Task *task = g_app->m_taskManager->GetCurrentTask();

        WorldObjectId selectedId;
        Vector3 selectedWorldPos;
        Vector3 highlightedWorldPos;
        WorldObjectId highlightedId;
        float highlightedRadius;

        bool somethingSelected = GetSelectedObject( selectedId, selectedWorldPos );                        
        bool somethingHighlighted = GetHighlightedObject( highlightedId, highlightedWorldPos, highlightedRadius ); 

        if( g_app->m_taskManagerInterface->m_visible )
        {
            // Looking at the task manager
            if( somethingSelected && selectedId.GetUnitId() != UNIT_BUILDINGS )
            {
                RenderSelectionArrows( selectedId, selectedWorldPos );
            }
            
            if( somethingHighlighted )
            {
                float camDist = ( g_app->m_camera->GetPos() - highlightedWorldPos ).Mag();
                float posX, posY;
                g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &posX, &posY );
                m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                m_cursorSelection->SetColour( RGBAColour(255,255,100,255) );
                m_cursorSelection->SetAnimation( false );
                g_app->m_renderer->SetupMatricesFor2D();
                m_cursorSelection->Render( posX, g_app->m_renderer->ScreenH() - posY );
                g_app->m_renderer->SetupMatricesFor3D();
            }

        }
        else if( task && 
                task->m_state == Task::StateStarted && 
                task->m_type != GlobalResearch::TypeOfficer &&
                !somethingHighlighted )        
        {
            // The player is placing a task
            bool validPlacement = g_app->m_taskManager->IsValidTargetArea(task->m_id, mousePos);
            m_cursorPlacement->SetAnimation( validPlacement );
            m_cursorPlacement->SetSize( 40.0f );
            Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
            Vector3 front = (landNormal ^ g_upVector).Normalise();
            m_cursorPlacement->Render3D( mousePos, front, landNormal );
            if( !validPlacement) 
				m_cursorDisabled->Render3D(mousePos, front, landNormal);
			else
				m_validPlacementOpportunity = true;
			
            cursorRendered = true;
        }
		else if( g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) )
		{
            if (false) {
			    if( task &&
				    task->m_type == GlobalResearch::TypeSquad &&
				    task->m_state == Task::StateRunning )
			    {
				    if( g_inputManager->controlEvent( ControlUnitPrimaryFireDirected /* ControlUnitStartSecondaryFireDirected */ ) )
				    {
					    InputDetails details;
					    g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

					    InsertionSquad *squad = ( InsertionSquad *)g_app->m_location->GetMyTeam()->GetMyUnit();
					    Squadie *pointMan = (Squadie *)squad->GetPointMan();

					    Vector3 t = pointMan->GetSecondaryWeaponTarget();

					    Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(t.x, t.z);
					    Vector3 front = (landNormal ^ g_upVector).Normalise();

					    RenderWeaponMarker( t, front, landNormal );
				    }
			    }
            }
		    cursorRendered = true;
		}
        else
        {            
            if( somethingHighlighted &&
                !(somethingSelected && highlightedId.GetUnitId() == UNIT_BUILDINGS) )
            {
                float camDist = ( g_app->m_camera->GetPos() - highlightedWorldPos ).Mag();
                if( camDist > 100 || !somethingSelected || selectedId != highlightedId )
                {
                    float posX, posY;
                    g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &posX, &posY );
                    m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                    m_cursorSelection->SetColour( RGBAColour(255,255,100,255) );
                    m_cursorSelection->SetAnimation( false );
                    g_app->m_renderer->SetupMatricesFor2D();
                    m_cursorSelection->Render( posX, g_app->m_renderer->ScreenH() - posY );
                    g_app->m_renderer->SetupMatricesFor3D();

					m_highlightingSomething = true;
                }
            }

            if( somethingSelected && selectedId.GetUnitId() != UNIT_BUILDINGS )
            {
                int entityType = Entity::TypeInvalid;
                if( selectedId.GetIndex() == -1 )   entityType = g_app->m_location->GetUnit(selectedId)->m_troopType;
                else                                entityType = g_app->m_location->GetEntity(selectedId)->m_type;             

                RenderSelectionArrows( selectedId, selectedWorldPos );
				m_moveableEntitySelected = true;

                bool highlightedBuilding = ( somethingHighlighted && highlightedId.GetUnitId() == UNIT_BUILDINGS );
                
                if( (entityType == Entity::TypeInsertionSquadie ||
                     entityType == Entity::TypeOfficer)
                     && highlightedBuilding )
                {
                    Building *building = g_app->m_location->GetBuilding( highlightedId.GetUniqueId() );

                    if( building && building->m_type == Building::TypeRadarDish)
                    {
                        // Squadies/officer trying to get into a teleport
                        RadarDish *dish = (RadarDish *) building;
                        if( dish->Connected() )
                        {
                            Vector3 entrancePos, entranceFront;
                            dish->GetEntrance( entrancePos, entranceFront );
                            float posX, posY;
                            g_app->m_camera->Get2DScreenPos( entrancePos, &posX, &posY );
                            g_app->m_renderer->SetupMatricesFor2D();
                            m_cursorPlacement->SetSize( 60.0f );
                            m_cursorPlacement->SetAnimation( true );
                            m_cursorPlacement->Render( posX, g_app->m_renderer->ScreenH() - posY );
                            g_app->m_renderer->SetupMatricesFor3D();
                        }
                    }
                }

                // Selected a unit OR an entity
                if( !somethingHighlighted || highlightedId.GetUnitId() == UNIT_BUILDINGS )
                {
                    Vector3 targetFront = (mousePos - selectedWorldPos).Normalise();
                    Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
                    Vector3 targetRight = (targetFront ^ landNormal).Normalise();
                    targetFront = (targetRight ^ landNormal).Normalise();
                    m_cursorMoveHere->SetSize( 30.0f );
                    m_cursorMoveHere->Render3D( mousePos, targetFront, landNormal );
                    cursorRendered = true;
                }
            }

            if( somethingSelected && selectedId.GetUnitId() == UNIT_BUILDINGS )
            {
                // Selected a building - render a targetting crosshair
                g_app->m_renderer->SetupMatricesFor2D();
                m_cursorTurretTarget->SetSize( 200.0f );
                m_cursorTurretTarget->Render( screenX, screenY );
                g_app->m_renderer->SetupMatricesFor3D();
            }

            if( !somethingSelected && !somethingHighlighted )
            {
                // Looking at empty landscape
                Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
                Vector3 front = (landNormal ^ g_upVector).Normalise();
                m_cursorHighlight->SetAnimation( false );
                m_cursorHighlight->SetSize( 30.0f );
                m_cursorHighlight->Render3D( mousePos, front, landNormal );                                
                cursorRendered = true;
            }
        }
    }
                    
    if( !cursorRendered &&
        g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD )
    {
        // Nobody has drawn a cursor yet
        // So give us the default
        g_app->m_renderer->SetupMatricesFor2D();
        RenderStandardCursor(screenX,screenY);
        g_app->m_renderer->SetupMatricesFor3D();
    }

    if( g_app->m_location && g_app->m_location->GetMyTeam() )
    {
        //RenderSphere( g_app->m_location->GetMyTeam()->m_currentMousePos, 10 );
    }
    
    RenderMarkers();
    
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());

    END_PROFILE( g_app->m_profiler, "Render GameCursor" );
}


void GameCursor::RenderStandardCursor( float _screenX, float _screenY )
{
    m_cursorStandard->Render(_screenX,_screenY);
}


void LeftArrow(int x, int y, int size)
{
	glBegin( GL_TRIANGLES );
		glTexCoord2f(0.0f, 0.0f);	glVertex2i( x - size, y - size/2 );
		glTexCoord2f(1.0f, 0.5f);	glVertex2i( x,        y );
		glTexCoord2f(0.0f, 1.0f);	glVertex2i( x - size, y + size/2 );
	glEnd();
}


void RightArrow(int x, int y, int size)
{
	glBegin( GL_TRIANGLES );
		glTexCoord2f(0.0f, 0.0f);	glVertex2i( x + size, y - size/2 );
		glTexCoord2f(1.0f, 0.5f);	glVertex2i( x,        y );
		glTexCoord2f(0.0f, 1.0f);	glVertex2i( x + size, y + size/2 );
	glEnd();
}


void TopArrow(int x, int y, int size)
{
	glBegin( GL_TRIANGLES );
		glTexCoord2f(0.0f, 1.0f);	glVertex2i( x - size/2, y - size );
		glTexCoord2f(0.0f, 0.0f);	glVertex2i( x + size/2, y - size );
		glTexCoord2f(1.0f, 0.5f);	glVertex2i( x,          y );
	glEnd();
}


void BottomArrow(int x, int y, int size)
{
	glBegin( GL_TRIANGLES );
		glTexCoord2f(1.0f, 0.5f);	glVertex2i( x, y );
		glTexCoord2f(0.0f, 1.0f);	glVertex2i( x + size/2, y + size );
		glTexCoord2f(0.0f, 0.0f);	glVertex2i( x - size/2, y + size );
	glEnd();
}


void GameCursor::FindScreenEdge( Vector2 const &_line, float *_posX, float *_posY )
{
//    y = mx + c
//    c = y - mx
//    x = (y - c) / m

    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();
    
    float m = _line.y / _line.x;
    float c = ( screenH / 2.0f ) - m * ( screenW / 2.0f );
    
    if( _line.y < 0 )
    {
        // Intersect with top view plane
        float x = ( 0 - c ) / m;
        if( x >= 0 && x <= screenW )
        {
            *_posX = x;
            *_posY = 0;
            return;
        }
    }
    else
    {
        // Intersect with the bottom view plane
        float x = ( screenH - c ) / m;
        if( x >= 0 && x <= screenW )
        {
            *_posX = x;
            *_posY = screenH;
            return;
        }        
    }

    if( _line.x < 0 )
    {
        // Intersect with left view plane 
        float y = m * 0 + c;
        if( y >= 0 && y <= screenH ) 
        {
            *_posX = 0;
            *_posY = y;
            return;
        }
    }
    else
    {
        // Intersect with right view plane
        float y = m * screenW + c;
        if( y >= 0 && y <= screenH ) 
        {
            *_posX = screenW;
            *_posY = y;
            return;
        }        
    }

    // We should never ever get here
    DarwiniaDebugAssert( false );
    *_posX = 0;
    *_posY = 0;
}


void GameCursor::RenderSelectionArrow( float _screenX, float _screenY, float _screenDX, float _screenDY, float _size, float _alpha )
{
    Vector2 pos( _screenX, _screenY );
    Vector2 gradient( _screenDX, _screenDY );
    Vector2 rightAngle = gradient;
    float tempX = rightAngle.x;
    rightAngle.x = rightAngle.y;
    rightAngle.y = tempX * -1;

    glEnable        ( GL_BLEND );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    glColor4f       ( _alpha, _alpha, _alpha, 0.0f );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_selectionArrowShadowFilename ) );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2fv( (pos - rightAngle * _size / 2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2fv( (pos - rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2fv( (pos + rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2fv( (pos + rightAngle * _size / 2.0f).GetData() );
    glEnd();
    
    glColor4f       ( 1.0f, 1.0f, 0.3f, _alpha );
	glBlendFunc		( GL_SRC_ALPHA, GL_ONE );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_selectionArrowFilename ) );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2fv( (pos - rightAngle * _size / 2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2fv( (pos - rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2fv( (pos + rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2fv( (pos + rightAngle * _size / 2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

}


void GameCursor::RenderSelectionArrows( WorldObjectId _id, Vector3 const &_pos )
{
	Entity *ent = g_app->m_location->GetEntity( _id );
	Unit *unit = g_app->m_location->GetUnit( _id );
	if( !ent && !unit ) return;
	if( ent && ent->m_dead ) return;
	if( unit && unit->NumAliveEntities() == 0 ) return;

    float triSize = 40.0f;

    static bool onScreen = true;
    
    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();

    //
    // Project worldTarget into screen co-ordinates
    // Is the _pos on screen or not?

    float screenX, screenY;
    g_app->m_camera->Get2DScreenPos( _pos, &screenX, &screenY );
    screenY = screenH - screenY;

	Vector3 toCam = g_app->m_camera->GetPos() - _pos;
	float angle = toCam * g_app->m_camera->GetFront();
	Vector3 rotationVector = toCam ^ g_app->m_camera->GetFront();

    if( angle <= 0.0f &&
        screenX >= 0 && screenX < screenW &&
        screenY >= 0 && screenY < screenH )
    {
        // _pos is onscreen
        float camDist = toCam.Mag();
	    m_selectionArrowBoost -= g_advanceTime * 0.4f;

        float distanceOut = 1000 / sqrtf( camDist );
        float alpha = min( m_selectionArrowBoost, 0.9f );

        if( camDist > 200.0f )
        {
            alpha = max( min( ( camDist - 200.0f ) / 200.0f, 0.9f ), alpha );
        }
        g_app->m_renderer->SetupMatricesFor2D();  
        RenderSelectionArrow( screenX, screenY - distanceOut, 0, -1, triSize, alpha );
        RenderSelectionArrow( screenX, screenY + distanceOut, 0, 1, triSize, alpha );
        RenderSelectionArrow( screenX - distanceOut, screenY, -1, 0, triSize, alpha );
        RenderSelectionArrow( screenX + distanceOut, screenY, 1, 0, triSize, alpha );
        g_app->m_renderer->SetupMatricesFor3D();  

        if( !onScreen ) BoostSelectionArrows(2.0f);
        onScreen = true;
    }
    else
    {
        // _pos is offscreen
        Vector3 camPos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * 1000;
        Vector3 camToTarget = ( _pos - camPos ).SetLength( 100 );

        float camX = screenW / 2.0f;
        float camY = screenH / 2.0f;
        float posX, posY;
        g_app->m_camera->Get2DScreenPos( camPos + camToTarget, &posX, &posY );

        Vector2 lineNormal( posX - camX, posY - camY );
        lineNormal.Normalise();
        
        float edgeX, edgeY;
        FindScreenEdge( lineNormal, &edgeX, &edgeY );

        lineNormal.x *= -1;

        g_app->m_renderer->SetupMatricesFor2D();  
        RenderSelectionArrow( edgeX, screenH - edgeY, lineNormal.x, lineNormal.y, triSize * 1.5f, 0.9f );       
        g_app->m_renderer->SetupMatricesFor3D();  

        onScreen = false;
    }
}


/*
void GameCursor::RenderSelectionArrows( WorldObjectId _id, Vector3 const &_pos )
{
	float triSize = 36.0f;          // + fabs(sinf(g_gameTime*4)) * 10;
	float xOut = 24.0f;
	float yOut = 24.0f;

    static bool onScreen = true;
    
	// Project worldTarget into screen co-ordinates
    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();
    float screenX, screenY;
    g_app->m_camera->Get2DScreenPos( _pos, &screenX, &screenY );
    screenY = screenH - screenY;

	// Calculate alpha
	Vector3 toCam = g_app->m_camera->GetPos() - _pos;
	float distance = toCam.Mag();
	
    if( distance < 400.0f )
    {
        xOut += ( 100.0f - distance/4.0f );
        yOut += ( 100.0f - distance/4.0f );
    }
    
    float alpha = 0.0f;
	if (distance > 350.0f) 
	{
		distance -= 350.0f;
		alpha = distance / 300.0f;
		alpha = min(alpha, SELECTION_ARROWS_MAX_ALPHA);
	}

	if (m_selectionArrowBoost > alpha)
	{
		alpha = min(m_selectionArrowBoost, SELECTION_ARROWS_MAX_ALPHA);
	}
	static double lastTime = g_gameTime;
	double deltaTime = g_gameTime - lastTime;
	lastTime = g_gameTime;
	m_selectionArrowBoost -= g_advanceTime * BOOST_FADE_RATE;

	// Deal with the selected unit not being on screen
	float angle = toCam * g_app->m_camera->GetFront();
	Vector3 rotationVector = toCam ^ g_app->m_camera->GetFront();
	if (angle > 0.0f)
	{
		// Unit is behind camera
		
		alpha = SELECTION_ARROWS_MAX_ALPHA;

		if (rotationVector.y < 0.0f)
		{
			screenX = screenW + xOut - triSize / 3.0f;
		}
		else
		{
			screenX = -xOut + triSize / 3.0f;
		}
		screenY = screenH / 2;

        triSize *= 1.5f;
        onScreen = false;
	}
	else
	{
		// Unit is infront of camera

        if( !onScreen )
        {
            BoostSelectionArrows( 2.0f );
        }
        
		if (screenX > screenW + xOut)
		{
			alpha = SELECTION_ARROWS_MAX_ALPHA;
			screenX = screenW + xOut - triSize / 3.0f;
			screenY = screenH / 2.0f;
            //screenY = min( screenY, screenH );
            //screenY = max( screenY, 0 );
            onScreen = false;
            triSize *= 1.5f;
		}
		else if (screenX < -xOut)
		{
			alpha = SELECTION_ARROWS_MAX_ALPHA;
			screenX = -xOut + triSize / 3.0f;
			screenY = screenH / 2.0f;
            //screenY = min( screenY, screenH );
            //screenY = max( screenY, 0 );
            triSize *= 1.5f;
            onScreen = false;
		}
		else if (screenY > screenH + yOut)
		{
			alpha = SELECTION_ARROWS_MAX_ALPHA;
			screenX = screenW / 2.0f;
			screenY = screenH + yOut - triSize / 3.0f;
            triSize *= 1.5f;
            onScreen = false;
		}
		else if (screenY < -yOut)
		{
			alpha = SELECTION_ARROWS_MAX_ALPHA;
			screenX = screenW / 2.0f;
			screenY = -yOut + triSize / 3.0f;
            triSize *= 1.5f;
            onScreen = false;
		}
        else
        {
            onScreen = true;
        }
   	}

    if( !onScreen )
    {
        Vector3 camPos = g_app->m_camera->GetPos() + g_app->m_camera->GetFront() * 1000;
        Vector3 camToTarget = ( _pos - camPos ).SetLength( 100 );

        float camX = screenW / 2.0f;
        float camY = screenH / 2.0f;
        float posX, posY;
        g_app->m_camera->Get2DScreenPos( camPos + camToTarget, &posX, &posY );

        Vector2 lineNormal( posX - camX, posY - camY );
        lineNormal.Normalise();
        
        float edgeX, edgeY;
        FindScreenEdge( lineNormal, &edgeX, &edgeY );

        lineNormal.x *= -1;

        g_app->m_renderer->SetupMatricesFor2D();  

        RenderSelectionArrow( edgeX, screenH - edgeY, lineNormal.x, lineNormal.y, triSize, 1.0f );
        
        g_app->m_renderer->SetupMatricesFor3D();  
    }
    else
    {
	    // Get ready to render
        g_app->m_renderer->SetupMatricesFor2D();  
        glEnable        ( GL_BLEND );
        glDisable       ( GL_CULL_FACE );
    //	glEnable		( GL_TEXTURE_2D );
    //	glBindTexture	( GL_TEXTURE_2D, g_app->m_resource->GetTexture("selection_arrow") );

	    // Do subtractive pass 
	    {
		    glColor4f       ( 0.9f, 0.9f, 0.1f, alpha/2.0f );
		    glBlendFunc		( GL_ZERO, GL_ONE_MINUS_SRC_ALPHA );
		    
		    LeftArrow ( screenX - xOut, screenY, triSize );
		    RightArrow( screenX + xOut, screenY, triSize );
		    TopArrow  ( screenX, screenY - yOut, triSize );
		    BottomArrow(screenX, screenY + yOut, triSize );
	    }

	    // Do additive pass
	    {
		    float myTriSize = triSize - 8.0f;
		    float myXOut = xOut + 5.0f;            
		    float myYOut = yOut + 5.0f;

		    glColor4f       ( 0.9f, 0.9f, 0.1f, alpha );
		    glBlendFunc		( GL_SRC_ALPHA, GL_ONE );

		    LeftArrow ( screenX - myXOut, screenY, myTriSize );
		    RightArrow( screenX + myXOut, screenY, myTriSize );
		    TopArrow  ( screenX, screenY - myYOut, myTriSize );
		    BottomArrow(screenX, screenY + myYOut, myTriSize );
	    }

        glDisable       ( GL_BLEND );
        glEnable        ( GL_CULL_FACE );
    //	glDisable		( GL_TEXTURE_2D );

        g_app->m_renderer->SetupMatricesFor3D();  
    }
}*/

void GameCursor::RenderWeaponMarker ( Vector3 _pos, Vector3 _front, Vector3 _up )
{
    m_cursorPlacement->SetSize(40.0f);    
    m_cursorPlacement->SetShadowed( true );
    m_cursorPlacement->SetAnimation( true );
    m_cursorPlacement->Render3D( _pos, _front, _up );
}



// ****************************************************************************
//  Class MouseCursor
// ****************************************************************************

MouseCursor::MouseCursor( char const *_filename )
:   m_hotspotX(0.0f),
    m_hotspotY(0.0f),
    m_size(20.0f),
    m_animating(false),
    m_shadowed(true)
{
    char fullFilename[512];
	sprintf( fullFilename, "%s", _filename);
	m_mainFilename = strdup(fullFilename);

    BinaryReader *binReader = g_app->m_resource->GetBinaryReader( m_mainFilename );
	DarwiniaReleaseAssert(binReader, "Failed to open mouse cursor resource %s", _filename); 
    BitmapRGBA bmp( binReader, "bmp" );
	SAFE_DELETE(binReader);

	g_app->m_resource->AddBitmap(m_mainFilename, bmp);
	
	sprintf(fullFilename, "shadow_%s", _filename);
	m_shadowFilename = strdup(fullFilename);
    bmp.ApplyBlurFilter( 10.0f );
	g_app->m_resource->AddBitmap(m_shadowFilename, bmp);

    m_colour.Set(255,255,255,255);
}

MouseCursor::~MouseCursor()
{
	free(m_mainFilename);
	free(m_shadowFilename);
}

float MouseCursor::GetSize()
{
	float size = m_size;

	if (m_animating)
	{
		size += fabs(sinf(g_gameTime * 4.0f)) * size * 0.6f;
	}

	return size;
}


void MouseCursor::SetSize(float _size)
{
	m_size = _size;
}


void MouseCursor::SetAnimation(bool _onOrOff)
{
	m_animating = _onOrOff;
}


void MouseCursor::SetShadowed(bool _onOrOff)
{
    m_shadowed = _onOrOff;
}


void MouseCursor::SetHotspot(float x, float y)
{
	m_hotspotX = x;
	m_hotspotY = y;
}


void MouseCursor::SetColour(RGBAColour &_col )
{
    m_colour = _col;
}


void MouseCursor::Render(float _x, float _y)
{
	float s = GetSize();	
	float x = (float)_x - s * m_hotspotX;
	float y = (float)_y - s * m_hotspotY;

	glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE);
    glDepthMask     (false);

    if( m_shadowed )
    {
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_shadowFilename));
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.0f );
	    glBegin         (GL_QUADS);
		    glTexCoord2i(0,1);			glVertex2f(x, y);
		    glTexCoord2i(1,1);			glVertex2f(x + s, y);
		    glTexCoord2i(1,0);			glVertex2f(x + s, y + s);
		    glTexCoord2i(0,0);			glVertex2f(x, y + s);
	    glEnd();
    }
    
    glColor4ubv     (m_colour.GetData() );
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							
	glBegin         (GL_QUADS);
		glTexCoord2i(0,1);			glVertex2f(x, y);
		glTexCoord2i(1,1);			glVertex2f(x + s, y);
		glTexCoord2i(1,0);			glVertex2f(x + s, y + s);
		glTexCoord2i(0,0);			glVertex2f(x, y + s);
	glEnd();

    glDepthMask     (true);
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


void MouseCursor::Render3D( Vector3 const &_pos, Vector3 const &_front, Vector3 const &_up, bool _cameraScale)
{
    Vector3 rightAngle = (_front ^ _up).Normalise();
    
	glEnable        (GL_TEXTURE_2D);
    
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE );
    //glDisable       (GL_DEPTH_TEST );
    glDepthMask     (false);
    
    float scale = GetSize();
    if( _cameraScale )
    {
        float camDist = ( g_app->m_camera->GetPos() - _pos ).Mag();
        scale *= sqrtf(camDist) / 40.0f;
    }

    Vector3 pos = _pos;
    pos -= m_hotspotX * rightAngle * scale;
    pos += m_hotspotY * _front * scale;

    if( m_shadowed )
    {
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_shadowFilename));
	    glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
        glColor4f       (1.0f, 1.0f, 1.0f, 0.0f);

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex3fv( (pos).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + rightAngle * scale).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos - _front * scale + rightAngle * scale).GetData() );
            glTexCoord2i(0,0);      glVertex3fv( (pos - _front * scale).GetData() );
        glEnd();
    }

    glColor4ubv     (m_colour.GetData());
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3fv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (pos - _front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3fv( (pos - _front * scale).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );    
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


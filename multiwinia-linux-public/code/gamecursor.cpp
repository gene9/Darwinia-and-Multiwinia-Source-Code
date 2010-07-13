#include "lib/universal_include.h"
#include "lib/bitmap.h"
#include "lib/targetcursor.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"

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
#include "multiwinia.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "particle_system.h"
#include "level_file.h"
#include "markersystem.h"
#include "sound/soundsystem.h"

#include "worldobject/armour.h"
#include "worldobject/radardish.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/darwinian.h"
#include "worldobject/gunturret.h"



GameCursor::GameCursor()
:	m_selectionArrowBoost(0.0f),
	m_highlightingSomething(false),
	m_validPlacementOpportunity(false),
	m_moveableEntitySelected(false)
{
    m_cursorStandard = new MouseCursor( "icons/mouse_game.bmp" );
	m_cursorStandard->SetHotspot(0.055f, 0.070f);
    m_cursorStandard->SetSize(35.0f);

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
    m_cursorTurretTarget->m_additive = false;
    m_cursorTurretTarget->SetShadowed(false);

    m_cursorSelection = new MouseCursor( "icons/mouse_selection.bmp" );
    m_cursorSelection->SetHotspot( 0.5f, 0.5f );

    m_cursorEnter = new MouseCursor( "icons/mouse_enter.bmp" );
    m_cursorEnter->SetHotspot( 0.5f, 1.0f );
    m_cursorEnter->SetAnimation( true );
    m_cursorEnter->SetColour(RGBAColour(255,255,255,255) );
    m_cursorEnter->SetSize( 50.0f );
    
	m_cursorMissile = NULL;

    //
    // Load selection arrow graphic
    
	sprintf( m_selectionArrowFilename, "icons/selectionarrow.bmp" );

    BinaryReader *binReader = g_app->m_resource->GetBinaryReader( m_selectionArrowFilename );
	AppReleaseAssert(binReader, "Failed to open mouse cursor resource %s", m_selectionArrowFilename ); 
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


Vector3 GetPredictedPosition( Entity *entity )
{
    Vector3 pos = entity->m_pos + entity->m_centrePos;       

    Team *team = g_app->m_location->m_teams[ entity->m_id.GetTeamId() ];
    int lastUpdated = team->m_others.GetLastUpdated();

    float predictionTime = g_predictionTime;

    if( entity->m_id.GetIndex() > lastUpdated )
    {
        predictionTime += SERVER_ADVANCE_PERIOD;
    }

    pos += entity->m_vel * predictionTime;

    return pos;
}


bool GameCursor::GetSelectedObject( WorldObjectId &_id, Vector3 &_pos, float &_radius )
{
    Team *team = g_app->m_location->GetMyTeam();

	if(!team)
	{
		return false;
	}


    int lastUpdated = team->m_others.GetLastUpdated();
    
    if( team )
    {
        Entity *selectedEnt = team->GetMyEntity();
        if( selectedEnt )
        {
            _pos = GetPredictedPosition( selectedEnt );
            _id = selectedEnt->m_id;
            _radius = selectedEnt->m_radius;
            return true;
        }
        else if( team->m_currentBuildingId != -1 )
        {
            Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
            if( building )
            {
                _pos = building->m_centrePos;
                _id = building->m_id;
                _radius = building->m_radius;
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
                _radius = selected->m_radius;

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

    if( !g_app->m_taskManagerInterface->IsVisible() )
    {
        somethingHighlighted = g_app->m_locationInput->GetObjectUnderMouse( id, g_app->m_globalWorld->m_myTeamId );
    }
    else
    {
        Team *team = g_app->m_location->GetMyTeam();
        if( team )
        {
            Task *task = team->m_taskManager->GetTask( g_app->m_taskManagerInterface->m_highlightedTaskId );
            if( task && task->m_objId.IsValid() )
            {
                id = task->m_objId;
                somethingHighlighted = true;
            }
        }
    }

     
    if( somethingHighlighted )
    {
        if( id.GetUnitId() == UNIT_BUILDINGS )
        {
            // Found a building
            Team *team = g_app->m_location->GetMyTeam();
            Building *building = g_app->m_location->GetBuilding( id.GetUniqueId() );

            if( building &&
			   (building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeBridge ||
                 (building->m_type == Building::TypeGunTurret && 
                  building->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId) ||
                building->m_type == Building::TypeFenceSwitch ) )
                //building->m_id.GetTeamId() == team->m_teamId )
            {
                _id = id;
                _pos = building->m_centrePos;
                _radius = building->m_radius * 0.8f;
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
            if( unit &&
                unit->IsSelectable() )
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
            if( entity && entity->IsSelectable() )
            {
                _id = id;
                _pos = GetPredictedPosition( entity );
                _radius = entity->m_radius;
                if( entity->m_type == Entity::TypeDarwinian ) _radius = entity->m_radius*2.0f;
                found = true;
            }
        }
    }    


    //
    // Send an audio event when something new is highlighted

    static WorldObjectId s_previousHighlight;
    static double s_lastTick = 0.0f;

    if( found )
    {
        if( id != s_previousHighlight )
        {
            s_previousHighlight = id;

            WorldObject *wobj = g_app->m_location->GetWorldObject(id);
            Vector3 pos = ( wobj ? wobj->m_pos : g_zeroVector );

            double timeNow = GetHighResTime();

            if( timeNow > s_lastTick + 0.2f )
            {
                g_app->m_soundSystem->TriggerOtherEvent( pos, "UnitHighlight", SoundSourceBlueprint::TypeMultiwiniaInterface, 255 );
                s_lastTick = timeNow;
            }
        }
    }
    else
    {
        s_previousHighlight.SetInvalid();
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
        double timeSync = GetHighResTime() - marker->m_startTime;
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


void GameCursor::RenderSelectionCircle()
{
    Team *team = g_app->m_location->GetMyTeam();

    if( g_app->m_location && 
        team &&
        team->m_selectionCircleCentre != g_zeroVector )
    {

        for( int i = 0; i < 30; ++i )
        {
            float thisAngle = i/30.0f * 2.0f * M_PI;
            float predictedCircleRadius = team->m_selectionCircleRadius;
            thisAngle += GetHighResTime() * 0.5f;
            Vector3 pos = team->m_selectionCircleCentre;
            Vector3 angle( cosf(thisAngle) * predictedCircleRadius, 0, 
                           sinf(thisAngle) * predictedCircleRadius );

            Vector3 movement = angle ^ g_upVector;            
            movement.SetLength( 10 );
            Vector3 finalPos = pos + angle;
            finalPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( finalPos.x, finalPos.z );
            finalPos.y += 10;

            g_app->m_particleSystem->CreateParticle( finalPos, g_zeroVector, Particle::TypeControlFlash, predictedCircleRadius );            
        }
    }
}


void GameCursor::RenderSelectedIcon( char *filename )
{
	if( g_app->m_multiwinia->GameOver() ) return;
    int screenX = g_target->X();
    int screenY = g_target->Y();

    Vector3 lineNormal(0.85f,1,0);
    lineNormal.Normalise();

    screenX += lineNormal.x * 5;
    screenY += lineNormal.y * 5;

    Team *team = g_app->m_location->GetMyTeam();
    RGBAColour colour = team->m_colour;


    float iconSize = g_app->m_renderer->ScreenH() / 24.0f; 
    float shadowSize = iconSize+3;

    g_app->m_markerSystem->RenderShadow        ( screenX, screenY, lineNormal.x, lineNormal.y, shadowSize, 0.9f );
    g_app->m_markerSystem->RenderColourCircle  ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, colour );    
    g_app->m_markerSystem->RenderIcon          ( screenX, screenY, lineNormal.x, lineNormal.y, iconSize, filename, RGBAColour(255,255,255,255) );
}


void CalculateCentreOfSelectionIcon( float &centreX, float &centreY )
{
    int screenX = g_target->X();
    int screenY = g_target->Y();
    float iconSize = g_app->m_renderer->ScreenH() / 24.0f; 

    Vector3 lineNormal(0.85f,1,0);
    lineNormal.Normalise();

    screenX += lineNormal.x * 5;
    screenY += lineNormal.y * 5;

    Vector2 pos( screenX, screenY );
    Vector2 gradient( lineNormal.x, lineNormal.y );
    pos += gradient * 50;
    
    centreX = pos.x;
    centreY = pos.y;
}


void GameCursor::RenderSelectedUnitIcon( Entity *entity )
{  
	if( g_app->m_multiwinia->GameOver() ) return;
    switch( entity->m_type )
    {
        case Entity::TypeOfficer:
            RenderSelectedIcon( "icons/icon_officer.bmp" );
            break;
    }
}


void RenderSprite( char *filename, float scaleFactor )
{
    float centreX, centreY;
    float iconSize = scaleFactor * 0.5f * g_app->m_renderer->ScreenH() / 24.0f; 
    CalculateCentreOfSelectionIcon( centreX, centreY );

    int texId = g_app->m_resource->GetTexture( filename );
    glBindTexture( GL_TEXTURE_2D, texId );
    glEnable( GL_TEXTURE_2D );
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );

    glBegin         (GL_QUADS);
        glTexCoord2i(0,1);			glVertex2f(centreX-iconSize, centreY-iconSize);
        glTexCoord2i(1,1);			glVertex2f(centreX+iconSize, centreY-iconSize);
        glTexCoord2i(1,0);			glVertex2f(centreX+iconSize, centreY+iconSize);
        glTexCoord2i(0,0);			glVertex2f(centreX-iconSize, centreY+iconSize);
    glEnd();

    glDisable( GL_TEXTURE_2D );
}


void GameCursor::RenderSelectedTaskIcon ( Task *task )
{
	if( g_app->m_multiwinia->GameOver() ) return;
    char bmpFilename[256];
    sprintf( bmpFilename, "icons/icon_%s.bmp", Task::GetTaskName(task->m_type) );

    if( task->m_state != Task::StateStarted ||
        fmodf( GetHighResTime(), 0.4f ) < 0.25f )
    {
        RenderSelectedIcon( bmpFilename );
    }



    if( task->m_state == Task::StateStarted )
    {
        Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
        mousePos.y = max( 1.0, mousePos.y );
        
    }   


    //
    // Some tasks require extra rendering to show their effect

    if( task->m_state == Task::StateStarted )
    {
        Vector3 decentMousePos = g_app->m_userInput->GetMousePos3d();
        Team *myTeam = g_app->m_location->GetMyTeam();

        // make sure it's a valid location
        // and show a red cross if not

        if( !g_app->m_location->GetMyTaskManager()->IsValidTargetArea(task->m_id, decentMousePos) )
        {
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDepthMask(false);
            RenderSprite( "icons/mouse_disabled.bmp", 1.2f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        }


        g_app->m_renderer->SetupMatricesFor3D();

        if( task->m_type == GlobalResearch::TypeGunTurret ||
            task->m_type == GlobalResearch::TypeAntsNest )
        {
            Shape *shape = NULL;
            if      ( task->m_type == GlobalResearch::TypeGunTurret ) shape = g_app->m_resource->GetShape( "battlecannonturret.shp" );
            else if ( task->m_type == GlobalResearch::TypeAntsNest ) shape = g_app->m_resource->GetShape( "anthill.shp" );

            if( shape )
            {
                Matrix34 mat(Vector3(0,0,1), g_upVector, decentMousePos);

                glEnable( GL_BLEND );    
                glDisable( GL_CULL_FACE );
                glDepthMask( false );
                glBlendFunc( GL_ONE, GL_SRC_COLOR );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                shape->Render( 0.0f, mat ); 
            }
        }
        else if( task->m_type == GlobalResearch::TypeHotFeet ||
                task->m_type == GlobalResearch::TypeBooster ||
                task->m_type == GlobalResearch::TypeSubversion ||
                task->m_type == GlobalResearch::TypePlague ||
                task->m_type == GlobalResearch::TypeExtinguisher ||
                task->m_type == GlobalResearch::TypeMeteorShower ||
                task->m_type == GlobalResearch::TypeRage ||
                task->m_type == GlobalResearch::TypeNuke )
        {
            float area = POWERUP_EFFECT_RANGE;
            float height = POWERUP_EFFECT_RANGE * 0.6f;

            if( task->m_type == GlobalResearch::TypeMeteorShower ) 
            {
                area *= 4.0f;
                height = POWERUP_EFFECT_RANGE;
            }

            if( task->m_type == GlobalResearch::TypeNuke )
            {
                area *= 3.0f;
                height = POWERUP_EFFECT_RANGE;
            }

            RGBAColour colour = myTeam->m_colour;
            if( fmodf( GetHighResTime(), 0.4f ) > 0.25f ) colour.a *= 0.5f;

            glDisable( GL_CULL_FACE );
            RenderCyliner( decentMousePos, area, height, 100, colour, true );
        }

        g_app->m_renderer->SetupMatricesFor2D();
    }
}
 

bool GameCursor::RenderCrosshair()
{
	if( EclGetWindows()->Size() > 0 ) return false;

	Team *myTeam = g_app->m_location->GetMyTeam();
    if( !myTeam ) return false;

	Building* building = g_app->m_location->GetBuilding(myTeam->m_currentBuildingId);
	Unit* unit = g_app->m_location->GetMyTeam()->GetMyUnit();
	if( !building && !unit ) return false;

	float crosshairSize = -1.0f;
	if( building)
	{
		if( building->m_type == Building::TypeGunTurret)
		{
			crosshairSize =g_app->m_renderer->ScreenH() / 10.0f;
		}
		else if( building->m_type = Building::TypeRadarDish )
		{
			crosshairSize =g_app->m_renderer->ScreenH() / 10.0f;
		}
	}
	else if( unit )
	{
		if( unit->m_troopType == Entity::TypeInsertionSquadie &&
			INPUT_MODE_GAMEPAD == g_inputManager->getInputMode())
		{
			crosshairSize = g_app->m_renderer->ScreenW() / 25.6f;
		}
		else
		{
			return false;
		}
	}


    float posX, posY;

    //
    // Extra aiming info for gun turrets first

    if( building && building->m_type == Building::TypeGunTurret)
    {
        GunTurret *turret = (GunTurret *) building;

        if( turret->m_actualTargetPos != g_zeroVector )
        {           
            g_app->m_renderer->SetupMatricesFor3D();
            g_app->m_camera->Get2DScreenPos( turret->m_actualTargetPos, &posX, &posY );
            posY = g_app->m_renderer->ScreenH() - posY;
            g_app->m_renderer->SetupMatricesFor2D();

            static float s_posX = 0;
            static float s_posY = 0;
            
            double timeFactor = g_advanceTime * 10;
            s_posX = s_posX * (1-timeFactor) + posX * timeFactor;
            s_posY = s_posY * (1-timeFactor) + posY * timeFactor;

            m_cursorTurretTarget->SetSize(crosshairSize * 1.5f);
            m_cursorTurretTarget->Render( s_posX, s_posY );
        }
    }
	else if(	unit && 
				unit->m_troopType == Entity::TypeInsertionSquadie &&
				INPUT_MODE_GAMEPAD == g_inputManager->getInputMode() &&
				g_app->m_camera->m_trackingEntity )	
	{
		// Squaddies
		// LEANDER : This doesn't work. Too close to launch. Needs fixing.
		return true;
		//Vector3 temp(g_app->m_camera->m_targetVector.x, g_app->m_camera->m_targetVector.z, 0);
		g_app->m_camera->Get2DScreenPos( g_app->m_camera->m_trackingEntity->m_pos + g_app->m_camera->m_targetVector, &posX, &posY );
		g_app->m_renderer->SetupMatricesFor2D();
		m_cursorTurretTarget->SetSize( crosshairSize );
		m_cursorTurretTarget->Render( posX, g_app->m_renderer->ScreenH() - posY );   
		g_app->m_renderer->SetupMatricesFor3D();
	}
    else
    {
        //
        // Ordinary crosshair

        g_app->m_renderer->SetupMatricesFor3D();
	    g_app->m_camera->Get2DScreenPos( g_app->m_userInput->GetMousePos3d(), &posX, &posY );
        posY = g_app->m_renderer->ScreenH() - posY;
        g_app->m_renderer->SetupMatricesFor2D();

	    m_cursorTurretTarget->SetSize(crosshairSize);
	    m_cursorTurretTarget->Render( posX, posY );
    }

	return true;
}

void GameCursor::RenderTryingToEnterEffect()
{
    if( m_objUnderMouse.IsValid() )
    {   
        bool tryingToEnter = false;
		bool validEnter = true;
        Vector3 entrancePos;

        Team *myTeam = g_app->m_location->GetMyTeam();
        if( !myTeam ) return;

        Entity *entity = g_app->m_location->GetEntity(m_objUnderMouse);

        //
        // Darwinians trying to enter armour

        if( entity && 
            entity->m_type == Entity::TypeArmour &&
            myTeam->m_numDarwiniansSelected > 0 )
        {
            tryingToEnter = true;
            entrancePos = entity->m_pos;
			validEnter = ((Armour*)entity)->GetNumPassengers() < ((Armour*)entity)->Capacity();
        }

        //
        // Darwinians or officer trying to get into radar dish

        if( m_objUnderMouse.GetUnitId() == UNIT_BUILDINGS )
        {
            Building *building = g_app->m_location->GetBuilding( m_objUnderMouse.GetUniqueId() );
            if( building && building->m_type == Building::TypeRadarDish )
            {
                Entity *selected = myTeam->GetMyEntity();

                if( myTeam->m_numDarwiniansSelected > 0 ||
                    (selected && selected->m_type == Entity::TypeOfficer) )
                {               
                    tryingToEnter = true;
                    Vector3 front;
                    ((RadarDish *)building)->GetEntrance( entrancePos, front );
					validEnter = ((RadarDish*)building)->Connected();
                }
            }
        }

        //
        // Do the rendering

        if( tryingToEnter )
        {
            float posX, posY;

            g_app->m_renderer->SetupMatricesFor3D();
            g_app->m_camera->Get2DScreenPos( entrancePos, &posX, &posY );
            posY = g_app->m_renderer->ScreenH() - posY;
            g_app->m_renderer->SetupMatricesFor2D();

            m_cursorPlacement->SetAnimation(validEnter);

            m_cursorPlacement->Render( posX, posY );

			if( !validEnter )
			{
				m_cursorDisabled->Render( posX, posY );
			}
        }
    }
}


void GameCursor::RenderSelectedDGsIcon( int count )
{    
	if( g_app->m_multiwinia->GameOver() ) return;
    RenderSelectedIcon( "icons/icon_darwinian.bmp" );

    float centreX, centreY;
    CalculateCentreOfSelectionIcon( centreX, centreY );
    
    float iconSize = g_app->m_renderer->ScreenH() / 24.0f; 
    centreX += 2;
    centreY += iconSize * 0.4f;

    glDisable( GL_CULL_FACE );

	if( g_app->m_location->m_routingSystem.m_walkableRoute )
	{
		g_gameFont.SetRenderOutline(true);
		glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		g_gameFont.DrawText2DCentre( centreX, centreY, 15, "%d", count);

		g_gameFont.SetRenderOutline(false);
		glColor4f( 1.0f, 1.0f, 1.0f, 0.9f );
		g_gameFont.DrawText2DCentre( centreX, centreY, 15, "%d", count );
	}
	else
	{
		glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        RenderSprite( "icons/mouse_disabled.bmp", 1.2f );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
	}
}


void GameCursor::RenderMultiwinia()
{
    if (!g_app->m_atMainMenu &&
        g_app->m_location &&
        !g_app->m_location->GetMyTeam() &&
        !g_app->m_editing )
    {
        // Teams have not been set up yet, most likely due to Darwinia having
        // not created a Team before this is called. Avoid crashes, return now!
#ifndef SPECTATOR_ONLY
        return;
#endif
    }

    if( !g_app->m_location ) return;
    if( EclGetWindows()->Size() > 0 ) return;

    g_app->m_renderer->SetupMatricesFor3D();

    RenderSelectionCircle();


    //
    // Get our team and colour

    Team *myTeam = g_app->m_location->GetMyTeam();

    RGBAColour teamCol = myTeam ? myTeam->m_colour : RGBAColour(255,255,100,255);
    teamCol.AddWithClamp( RGBAColour(100,100,100,0) );        
    teamCol.a = 150;



    //
    // Render any highlighted object under the mouse

    WorldObjectId highlightedId;
    Vector3 highlightedWorldPos;
    float highlightedRadius;
    bool somethingHighlighted = GetHighlightedObject( highlightedId, highlightedWorldPos, highlightedRadius ); 

    if( somethingHighlighted )
    {
        float highlightedScreenX, highlightedScreenY;
        g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &highlightedScreenX, &highlightedScreenY );

        RenderUnitHighlightEffect( highlightedWorldPos, highlightedRadius, teamCol );

        Entity *ent = g_app->m_location->GetEntity( highlightedId );
        if( ent )
        {
            if( ent->m_type == Entity::TypeArmour )
            {
                g_app->m_location->m_routingSystem.RenderRouteLine( ent->m_pos, ((Armour *)ent)->m_wayPoint, 0.5f );
            }
            else if( ent->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *)ent;

                if( officer->IsInFormationMode() && 
                    myTeam->GetMyEntity() != officer &&
                    officer->m_state == Officer::StateToWaypoint )
                {
                    Vector3 front = ( officer->m_wayPoint - officer->m_pos ).Normalise();
                    g_app->m_location->m_routingSystem.RenderRouteLine( officer->m_pos, officer->m_wayPoint, 0.5f );
                    RenderFormation( officer->m_wayPoint, front );
                }
            }
        }
    }

    m_objUnderMouse = highlightedId;



    //
    // Render any selected object 

    WorldObjectId selectedId;
    Vector3 selectedWorldPos;
    float selectedRadius;
    bool somethingSelected = GetSelectedObject( selectedId, selectedWorldPos, selectedRadius );                        

    if( somethingSelected )
    {
        bool skipSelectionEffect = false;

        // Don't render the selection effect if this is a gun turret

        if( selectedId.GetUnitId() == UNIT_BUILDINGS )
        {
            Building *building = g_app->m_location->GetBuilding( selectedId.GetUniqueId() );
            if( building && building->m_type == Building::TypeGunTurret )
            {
                skipSelectionEffect = true;
            }
        }

		Unit* u = NULL;
		Entity* e = NULL;
		bool check = false;

		if( (u = g_app->m_location->GetUnit(selectedId)) && u->m_troopType == Entity::TypeInsertionSquadie )
		{
			check = true;
		}
		else if( (e = g_app->m_location->GetEntity(selectedId)) && e->m_type == Entity::TypeInsertionSquadie )
		{
			check = true;
		}

		for( int i = 0; check && i < g_app->m_location->m_buildings.Size(); i++ )
		{
			if( g_app->m_location->m_buildings.ValidIndex(i) )
			{
				Building *b = g_app->m_location->m_buildings[i];
				if( b && b->m_type == Building::TypeRadarDish )
				{
					RadarDish *rd = (RadarDish*)b;
					for( int j = 0; check && j < rd->m_inTransit.Size(); j++ )
					{
						if( rd->m_inTransit.ValidIndex(j) )
						{
							WorldObjectId* id = rd->m_inTransit.GetPointer(j);

							if( id->GetUniqueId() == selectedId.GetUniqueId() ||
								id->GetUnitId() == selectedId.GetUnitId() )
							{
								skipSelectionEffect = true;
								check = false;
							}
						}
					}
				}
			}
		}

        if( !skipSelectionEffect )
        {
            float selectedScreenX, selectedScreenY;
            g_app->m_camera->Get2DScreenPos( selectedWorldPos, &selectedScreenX, &selectedScreenY );
        
            teamCol.AddWithClamp( RGBAColour(50,50,50,255) );
            teamCol.a = 255;

            RenderUnitHighlightEffect( selectedWorldPos, selectedRadius, teamCol );
        }
    }
   

    
    //
    // Are we highlighting a Darwinian?

    if( !somethingHighlighted && !somethingSelected )
    {
        Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
        WorldObjectId darwinianId = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );

        if( darwinianId.IsValid() &&
            myTeam->m_numDarwiniansSelected == 0 &&
            myTeam->m_selectionCircleCentre == g_zeroVector )
        {
            // Highlighting a darwinian, ready to promote
            Entity *entity = g_app->m_location->GetEntity( darwinianId );

            if( entity->IsSelectable() )
            {
                Vector3 predictedPos = GetPredictedPosition( entity );
                RenderUnitHighlightEffect( predictedPos, entity->m_radius*2.0f, teamCol );
            }
        }
    }


    //
    // Special case : officers toggling formation mode

    if( myTeam )
    {
        Entity *selected = myTeam->GetMyEntity();            
        if( selected && selected->m_type == Entity::TypeOfficer )
        {
            Vector3 decentMousePos = g_app->m_userInput->GetMousePos3d();

            Officer *officer = (Officer *)selected;
            if( m_objUnderMouse != officer->m_id )
            {
                if( officer->IsFormationToggle(decentMousePos) )
                {
                    Vector3 front = ( decentMousePos - officer->m_pos ).Normalise();
                    RenderFormation( officer->m_pos, front );
                }
                else if( officer->IsInFormationMode() )
                {
                    Vector3 front = ( decentMousePos - officer->m_pos ).Normalise();
                    RenderFormation( decentMousePos, front );
                }
            }
        }
    }      

}


void GameCursor::RenderRing( Vector3 const &centrePos, float innerRadius, float outerRadius, int numSteps, bool forceAttachToGround )
{
    Vector3 groundPos = centrePos;
    groundPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(centrePos.x, centrePos.z);

    Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(centrePos.x, centrePos.z);

    Vector3 offset1 = normal ^ Vector3(1,0,0);
    Vector3 offset2 = normal ^ Vector3(1,0,0);
    offset1.SetLength( innerRadius );
    offset2.SetLength( outerRadius );

    float rotation = 2.0f * M_PI * 1.0f/(float)numSteps;
    
    float amountToFollowLand = 75.0f / ( centrePos - g_app->m_camera->GetPos() ).Mag();
    amountToFollowLand = min( amountToFollowLand, 1.0f );
    if( forceAttachToGround ) amountToFollowLand = 1.0f;

    glBegin( GL_QUAD_STRIP );

    Vector3 firstP1, firstP2;

    for( int i = 0; i <= numSteps; ++i )
    {
        Vector3 p1 = groundPos + offset1;
        Vector3 p2 = groundPos + offset2;

        if( amountToFollowLand > 0.1f )
        {
            float landH1 = g_app->m_location->m_landscape.m_heightMap->GetValue(p1.x, p1.z);
            float landH2 = g_app->m_location->m_landscape.m_heightMap->GetValue(p2.x, p2.z);

            p1.y = ( p1.y * (1-amountToFollowLand) ) + landH1 * amountToFollowLand;
            p2.y = ( p2.y * (1-amountToFollowLand) ) + landH2 * amountToFollowLand;
        }

        if( i == 0 )
        {
            firstP1 = p1;
            firstP2 = p2;
        }

        if( i == numSteps )
        {
            p1 = firstP1;
            p2 = firstP2;
        }

        glVertex3dv( p1.GetData() );
        glVertex3dv( p2.GetData() );

        offset1.RotateAround( normal * rotation );
        offset2.RotateAround( normal * rotation );
    }

    glEnd();
}


void GameCursor::RenderCyliner( Vector3 const &centrePos, float radius, float height, int numSteps, RGBAColour colour, bool forceAttachToGround )
{
	if( g_app->m_multiwinia->GameOver() ) return;
    Vector3 groundPos = centrePos;
    groundPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(centrePos.x, centrePos.z);

    Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(centrePos.x, centrePos.z);
    if( forceAttachToGround ) normal = g_upVector;
    
    Vector3 offset1 = normal ^ Vector3(1,0,0);
    offset1.SetLength( radius );

    float rotation = 2.0f * M_PI * 1.0f/(float)numSteps;

    float amountToFollowLand = 75.0f / ( centrePos - g_app->m_camera->GetPos() ).Mag();
    amountToFollowLand = min( amountToFollowLand, 1.0f );
    if( forceAttachToGround ) amountToFollowLand = 1.0f;

    glEnable        ( GL_BLEND );
	glDisable( GL_CULL_FACE );
    glShadeModel( GL_SMOOTH );
    glBegin( GL_QUAD_STRIP );

    Vector3 upVector = g_upVector * 0.5f + normal * 0.5f;
    upVector.Normalise();

    Vector3 firstP1;

    for( int i = 0; i <= numSteps; ++i )
    {
        Vector3 p1 = groundPos + offset1;

        if( amountToFollowLand > 0.1f )
        {
            float landH1 = g_app->m_location->m_landscape.m_heightMap->GetValue(p1.x, p1.z);
            p1.y = ( p1.y * (1-amountToFollowLand) ) + landH1 * amountToFollowLand;
        }

        if( i == 0 )            firstP1 = p1;        
        if( i == numSteps )     p1 = firstP1;
        
        glColor4ub( colour.r, colour.g, colour.b, colour.a );
        glVertex3dv( p1.GetData() );

        glColor4ub( colour.r, colour.g, colour.b, 0 );
        glVertex3dv( (p1 + upVector * height).GetData() );

        offset1.RotateAround( normal * rotation );
    }

    glEnd();

    glShadeModel( GL_FLAT );
}


void GameCursor::RenderUnitHighlightEffect( Vector3 const &pos, float radius, RGBAColour colour )
{
    int numSteps = radius * 2.0f;
    if( numSteps < 25 ) numSteps = 25;

    float mainRingRadius = radius * 0.2f;
    float lineWidth = radius * 0.075f;

    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDepthMask( false );
    glColor4ubv( colour.GetData() );

    float nearPlaneStart = g_app->m_renderer->GetNearPlane();
    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.03f,
                                           g_app->m_renderer->GetFarPlane());


    RenderRing( pos, radius-mainRingRadius, radius, numSteps );

    glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
    RenderRing( pos, radius-mainRingRadius-lineWidth, radius-mainRingRadius, numSteps );
    RenderRing( pos, radius, radius+lineWidth, numSteps );

    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
                                           g_app->m_renderer->GetFarPlane());

    //
    // Glowing cylinder

    float heightF = ( g_app->m_camera->GetPos() - pos ).Mag() / 300.0f;
    heightF = max( heightF, 1.0f );
    glEnable( GL_CULL_FACE );
    RenderCyliner( pos, radius+lineWidth, radius * heightF, numSteps, colour );

    glDepthMask( true );
}

void GameCursor::RenderSelectedObjectNearCursor()
{
    //
    // If we have a unit or entity selected,
    // Render its icon near the mouse

    glDisable( GL_CULL_FACE );

    Team *myTeam = g_app->m_location->GetMyTeam();
    if( myTeam )
    {
        bool done = false;

        TaskManager *taskM = g_app->m_location->GetMyTaskManager();
        if( taskM )
        {
            Task *task = taskM->GetCurrentTask();
            if( task )
            {
                RenderSelectedTaskIcon( task );
                done = true;
            }
        }

        if( !done )
        {
            Entity *entity = myTeam->GetMyEntity();
            if( entity ) 
            {
                RenderSelectedUnitIcon( entity );
                done = true;
            }
        }

        if( !done )
        {
            if( myTeam->m_numDarwiniansSelected > 0 )
            {
                RenderSelectedDGsIcon( myTeam->m_numDarwiniansSelected );
            }
        }
    }
}


void GameCursor::RenderBasicCursorAlwaysOnTop()
{
	if( g_app->m_hideInterface && EclGetWindows()->Size() <= 0 ) return;

    if( g_app->Multiplayer() )
    {
        //
        // Do our 2d cursor rendering

        g_app->m_renderer->SetupMatricesFor2D();


        int screenX = g_target->X();
        int screenY = g_target->Y();


        if( EclGetWindows()->Size() == 0 )
        {
            RenderSelectedObjectNearCursor();
            RenderTryingToEnterEffect();
        }


        //
        // Render basic cursor last (top)

		if( !RenderCrosshair() )
		{
			RenderStandardCursor(screenX,screenY);
		}


        g_app->m_renderer->SetupMatricesFor3D();
    }
}


void GameCursor::RenderFormation( Vector3 const &pos, Vector3 const &front )
{
    float nearPlaneStart = g_app->m_renderer->GetNearPlane();
    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.03f,
                                           g_app->m_renderer->GetFarPlane());


    Team *myTeam = g_app->m_location->GetMyTeam();
    int formationSize = myTeam->m_evolutions[Team::EvolutionFormationSize];

    Vector3 camUp = g_app->m_camera->GetUp() * 2.0f;
    Vector3 camRight = g_app->m_camera->GetRight() * 2.0f;

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/particle.bmp" ) );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for( int i = 0; i < formationSize; ++i )
    {
        Vector3 up = g_upVector;
        Vector3 formationPos = Officer::GetFormationPositionFromIndex(pos, i, front, formationSize);

        glColor4ubv( myTeam->m_colour.GetData() );

        glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (formationPos - camUp).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (formationPos + camRight).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (formationPos + camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (formationPos - camRight).GetData() );
        glEnd();
    }

    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );

    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
                                            g_app->m_renderer->GetFarPlane());

}


void GameCursor::Render()
{    
    if( g_app->m_hideInterface && EclGetWindows()->Size() == 0 ) return;
	if (!g_app->m_atMainMenu &&
		g_app->m_location &&
		!g_app->m_location->GetMyTeam() &&
        !g_app->m_editing )
	{
		// Teams have not been set up yet, most likely due to Darwinia having
		// not created a Team before this is called. Avoid crashes, return now!
		// However, we still need the cursor at the main menu and 
#ifndef SPECTATOR_ONLY
        return;
#endif
	}


    if( g_app->Multiplayer() )
    {
        // Like i'm really going to work with this function.
        // This is such a mess im just gonna start again.
        RenderMultiwinia();
        return;
    }


    START_PROFILE( "Render GameCursor" );

	float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f,
							 			   g_app->m_renderer->GetFarPlane());

	int screenX = g_target->X();
	int screenY = g_target->Y();
    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
    mousePos.y = max( 1.0, mousePos.y );
        
    bool cursorRendered = false;
	bool chatLog = g_app->m_camera->ChatLogVisible();

	m_highlightingSomething = false;
	m_validPlacementOpportunity = false;
	m_moveableEntitySelected = false;
    
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
	    Task *task = g_app->m_location->GetMyTaskManager() ? 
                     g_app->m_location->GetMyTaskManager()->GetCurrentTask() : NULL;

        WorldObjectId selectedId;
        Vector3 selectedWorldPos;
        Vector3 highlightedWorldPos;
        WorldObjectId highlightedId;
        float highlightedRadius, selectedRadius;

        bool somethingSelected = GetSelectedObject( selectedId, selectedWorldPos, selectedRadius );                        
        bool somethingHighlighted = GetHighlightedObject( highlightedId, highlightedWorldPos, highlightedRadius ); 
		bool placingTask = false;
        Entity *e = g_app->m_location->GetEntity( highlightedId );
        if( e )
        {
			if( e->m_type == Entity::TypeDarwinian && g_app->IsSinglePlayer())
            {
                if( !task ||
                    task->m_state != Task::StateStarted ||
                    task->m_type != GlobalResearch::TypeOfficer )
                {
                    //somethingHighlighted = false;
                }
            }
        }

        if( g_app->m_taskManagerInterface->IsVisible() )
		{
            // Looking at the task manager

            if( somethingSelected && selectedId.GetUnitId() != UNIT_BUILDINGS )
            {
                RenderSelectionArrows( selectedId, selectedWorldPos );
            }
            
            if( somethingHighlighted )
            {
                RGBAColour teamCol = g_app->m_location->GetMyTeam() ? 
                                        g_app->m_location->GetMyTeam()->m_colour :
                                        RGBAColour(255,255,100,255);

                teamCol.AddWithClamp( RGBAColour(50,50,50,50) );

                float camDist = ( g_app->m_camera->GetPos() - highlightedWorldPos ).Mag();
                float posX, posY;
                g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &posX, &posY );
                m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                m_cursorSelection->SetColour( teamCol );
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
			placingTask = true;
			bool validPlacement = g_app->m_location->GetMyTaskManager()->IsValidTargetArea(task->m_id, mousePos);
			Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
			m_cursorPlacement->SetAnimation( validPlacement );
#ifdef RENDER_CURSOR_3D
            m_cursorPlacement->SetSize( 40.0f );
            Vector3 front = (landNormal ^ g_upVector).Normalise();
            m_cursorPlacement->Render3D( mousePos, front, landNormal );
            if( !validPlacement) 
				m_cursorDisabled->Render3D(mousePos, front, landNormal);
			else
				m_validPlacementOpportunity = true;
            
#else
			float posX, posY;
			g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
			g_app->m_renderer->SetupMatricesFor2D();
			m_cursorPlacement->SetSize( 40.0f );
			m_cursorPlacement->Render( posX, g_app->m_renderer->ScreenH() - posY );   
			if( !validPlacement) 
				m_cursorDisabled->Render( posX, g_app->m_renderer->ScreenH() - posY ); 
			else
				m_validPlacementOpportunity = true;
			g_app->m_renderer->SetupMatricesFor3D();
#endif
			cursorRendered = true;

            if( task->m_type == GlobalResearch::TypeHotFeet ||
                task->m_type == GlobalResearch::TypeBooster ||
                task->m_type == GlobalResearch::TypeSubversion ||
                task->m_type == GlobalResearch::TypePlague ||
                task->m_type == GlobalResearch::TypeExtinguisher ||
                task->m_type == GlobalResearch::TypeMeteorShower ||
                task->m_type == GlobalResearch::TypeRage )
            {
                float area = POWERUP_EFFECT_RANGE;
                if( task->m_type == GlobalResearch::TypeExtinguisher ) area *= 2.0f;
                if( task->m_type == GlobalResearch::TypeMeteorShower ) area *= 6.0f;
                float angle = g_gameTime * 3.0f;
                int steps = 10;
                float size = 60.0f;
                if( task->m_type == GlobalResearch::TypeMeteorShower )
                {
                    steps = 30;
                    size = 180.0f;
                }
                float degreesPerStep = (2 * M_PI) / steps;

                for( int i = 0; i < steps; ++i )
                {
                    Vector3 pos = mousePos;
                    pos.x += sin( i * degreesPerStep + angle ) * area;
                    pos.z += cos( i * degreesPerStep + angle ) * area;
                    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
                    g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeSelectionMarker, size );
                }
            }
            else if( task->m_type == GlobalResearch::TypeGunTurret ||
                     task->m_type == GlobalResearch::TypeAntsNest )
            {
                Shape *shape = NULL;
                if( task->m_type == GlobalResearch::TypeGunTurret ) shape = g_app->m_resource->GetShape( "battlecannonturret.shp" );
                else if( task->m_type == GlobalResearch::TypeAntsNest ) shape = g_app->m_resource->GetShape( "anthill.shp" );

                if( shape )
                {
                    Matrix34 mat(Vector3(1,0,0), g_upVector, mousePos);

                    glEnable( GL_BLEND );    
                    glDisable( GL_CULL_FACE );
                    glDepthMask( false );
                    glColor4f( 0.9f, 0.9f, 0.9f, 0.5f );
                    glBlendFunc( GL_ONE, GL_SRC_COLOR );
                    shape->Render( 0.0f, mat ); 
                }
            }
		}
		else if( g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) )
		{
			static double startTime = GetHighResTime();
			bool done = GetHighResTime() - startTime > 5;
			/*if( g_app->m_camera->m_targettingEntity )
			{
				m_cursorPlacement->SetAnimation( true );
				float posX, posY;
				g_app->m_camera->Get2DScreenPos( g_app->m_camera->m_targettingEntity->m_pos, &posX, &posY );
				g_app->m_renderer->SetupMatricesFor2D();
				m_cursorPlacement->SetSize( 40.0f );
				m_cursorPlacement->Render( posX, g_app->m_renderer->ScreenH() - posY );   
				g_app->m_renderer->SetupMatricesFor3D();
			}
			else*/ if( g_app->m_camera->m_trackingEntity && g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) )
			{
				float posX, posY;
				g_app->m_camera->Get2DScreenPos( g_app->m_camera->m_trackingEntity->m_pos + g_app->m_camera->m_targetVector, &posX, &posY );
				
				g_app->m_renderer->SetupMatricesFor2D();
				m_cursorTurretTarget->SetSize( 40.0f );
				m_cursorTurretTarget->Render( posX, g_app->m_renderer->ScreenH() - posY );   
				g_app->m_renderer->SetupMatricesFor3D();
			}
		    cursorRendered = true;
		}
        else
		{  
            bool darwiniansSelected = (g_app->m_location->GetMyTeam() && g_app->m_location->GetMyTeam()->m_numDarwiniansSelected > 0);
            if( somethingHighlighted &&
                !((somethingSelected || darwiniansSelected ) && highlightedId.GetUnitId() == UNIT_BUILDINGS ) )
			{
                float camDist = ( g_app->m_camera->GetPos() - highlightedWorldPos ).Mag();
                if( camDist > 100 || !somethingSelected || selectedId != highlightedId )
                {
                    bool show = true;
                    if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionNoRadarControl] == 1 )
                    {
                        Building *b = g_app->m_location->GetBuilding( highlightedId.GetUniqueId() );
                        if( b && b->m_type == Building::TypeRadarDish ) show = false;
                    }

                    if( show )
                    {
                        RGBAColour teamCol = g_app->m_location->GetMyTeam() ? 
                                             g_app->m_location->GetMyTeam()->m_colour :
                                             RGBAColour(255,255,100,255);

                        teamCol.AddWithClamp( RGBAColour(50,50,50,50) );

                        float posX, posY;
                        g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &posX, &posY );
                        m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                        m_cursorSelection->SetColour( teamCol );
                        m_cursorSelection->SetAnimation( false );
                        g_app->m_renderer->SetupMatricesFor2D();
                        m_cursorSelection->Render( posX, g_app->m_renderer->ScreenH() - posY );
                        g_app->m_renderer->SetupMatricesFor3D();

					    m_highlightingSomething = true;
                    }
                }
            }

            if( darwiniansSelected && somethingHighlighted &&
                highlightedId.GetUnitId() != UNIT_BUILDINGS )
            {
                Entity *e = g_app->m_location->GetEntity( highlightedId );
                if( e )
                {
                    if( e->m_type == Entity::TypeArmour )
                    {
                        Armour *a = (Armour *)e;
                        if( a->GetNumPassengers() < a->Capacity() )
                        {
                            float posX, posY;
                            g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
                            //m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                            g_app->m_renderer->SetupMatricesFor2D();
                            m_cursorEnter->Render( posX, g_app->m_renderer->ScreenH() - posY );
                            g_app->m_renderer->SetupMatricesFor3D();

                            cursorRendered = true;

                        }
                    }
                }
            }
            else if((somethingSelected || darwiniansSelected )
                && selectedId.GetUnitId() != UNIT_BUILDINGS )
            {
                int entityType = Entity::TypeInvalid;
                if( somethingSelected )
                {
                    if( selectedId.GetIndex() == -1 )   entityType = g_app->m_location->GetUnit(selectedId)->m_troopType;
                    else                                entityType = g_app->m_location->GetEntity(selectedId)->m_type;             

                    RenderSelectionArrows( selectedId, selectedWorldPos );
				    m_moveableEntitySelected = true;
                }

                bool highlightedBuilding = ( somethingHighlighted && highlightedId.GetUnitId() == UNIT_BUILDINGS );
                
                if( (entityType == Entity::TypeInsertionSquadie ||
                     entityType == Entity::TypeOfficer ||
                     g_app->m_location->GetMyTeam()->m_numDarwiniansSelected > 0 )
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
                if( (!somethingHighlighted || highlightedId.GetUnitId() == UNIT_BUILDINGS ) &&
                    g_app->m_location->m_teams[ g_app->m_globalWorld->m_myTeamId ]->m_numDarwiniansSelected == 0 &&
					!g_app->m_camera->m_camLookAround )
                {
					Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
#ifdef RENDER_CURSOR_3D
                    Vector3 targetFront = (mousePos - selectedWorldPos).Normalise();
                    Vector3 targetRight = (targetFront ^ landNormal).Normalise();
                    targetFront = (targetRight ^ landNormal).Normalise();
                    m_cursorMoveHere->SetSize( 30.0f );
                    m_cursorMoveHere->Render3D( mousePos, targetFront, landNormal );
#else
					float posX, posY, pointAtX, pointAtY;
					g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
					g_app->m_camera->Get2DScreenPos( selectedWorldPos, &pointAtX, &pointAtY );
					g_app->m_renderer->SetupMatricesFor2D();
					float rotationAngle = (atanf((posX-pointAtX)/(posY-pointAtY)))* (180/M_PI);
					if( posY > pointAtY )
					{
						rotationAngle += 180.0f;
					}
					m_cursorMoveHere->SetSize( 30.0f );
					m_cursorMoveHere->Render( posX, g_app->m_renderer->ScreenH() - posY, rotationAngle );   
					g_app->m_renderer->SetupMatricesFor3D();
#endif
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
                Team *team = g_app->m_location->GetMyTeam();
                if( team )
                {
                    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
                    WorldObjectId darwinianId = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );

                    if( darwinianId.IsValid() &&
                        team->m_numDarwiniansSelected == 0 &&
                        team->m_selectionCircleCentre == g_zeroVector )
					{
                        // Highlighting a darwinian, ready to promote
                        Entity *entity = g_app->m_location->GetEntity( darwinianId );
						if( entity->IsSelectable() || 
							(task && task->m_state == Task::StateStarted && task->m_type == GlobalResearch::TypeOfficer))
                        {
                            highlightedWorldPos = entity->m_pos + entity->m_vel * g_predictionTime + entity->m_centrePos;
                            highlightedRadius = entity->m_radius * 2.0f;

                            float camDist = ( g_app->m_camera->GetPos() - entity->m_pos ).Mag();
                            
                            RGBAColour teamCol = g_app->m_location->GetMyTeam()->m_colour;
                            teamCol.AddWithClamp( RGBAColour(50,50,50,50) );

                            float posX, posY;
                            g_app->m_camera->Get2DScreenPos( highlightedWorldPos, &posX, &posY );
                            m_cursorSelection->SetSize( highlightedRadius * 100 / sqrt(camDist) );
                            m_cursorSelection->SetColour( teamCol );
                            m_cursorSelection->SetAnimation( false );
                            g_app->m_renderer->SetupMatricesFor2D();
                            m_cursorSelection->Render( posX, g_app->m_renderer->ScreenH() - posY );
                            g_app->m_renderer->SetupMatricesFor3D();

                            m_highlightingSomething = true;
                        }
						else 
						{
							// Looking at empty landscape
							// Added as Darwinian::IsSelectable() has changed so that it returns false if in single player mode
							Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
#ifdef RENDER_CURSOR_3D
							Vector3 front = (landNormal ^ g_upVector).Normalise();
							m_cursorHighlight->SetAnimation( false );
							m_cursorHighlight->SetSize( 30.0f );
							m_cursorHighlight->Render3D( mousePos, front, landNormal );                                
#else
							float posX, posY;
							g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
							g_app->m_renderer->SetupMatricesFor2D();
							m_cursorHighlight->SetSize( 30.0f );
							m_cursorHighlight->Render( posX, g_app->m_renderer->ScreenH() - posY );   
							g_app->m_renderer->SetupMatricesFor3D();
#endif
							cursorRendered = true;
						}
                    }

                    else if( team->m_numDarwiniansSelected > 0 )
                    {
                        RenderSelectionArrows( WorldObjectId(), team->m_selectedDarwinianCentre, true );

                        // Group of darwinians selected
						Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
#ifdef RENDER_CURSOR_3D
                        Vector3 targetFront = (mousePos - team->m_selectedDarwinianCentre).Normalise();
                        Vector3 targetRight = (targetFront ^ landNormal).Normalise();
                        targetFront = (targetRight ^ landNormal).Normalise();
                        m_cursorMoveHere->SetSize( 30.0f );
                        m_cursorMoveHere->Render3D( mousePos, targetFront, landNormal );
#else
						float posX, posY, pointAtX, pointAtY;
                        g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
						g_app->m_camera->Get2DScreenPos( team->m_selectedDarwinianCentre, &pointAtX, &pointAtY );
						g_app->m_renderer->SetupMatricesFor2D();
                        m_cursorMoveHere->SetSize( 30.0f );
						float rotationAngle = (atanf((posX-pointAtX)/(posY-pointAtY)))* (180/M_PI);
						if( posY > pointAtY )
						{
							rotationAngle += 180.0f;
						}
                        m_cursorMoveHere->Render( posX, g_app->m_renderer->ScreenH() - posY,rotationAngle );   
						g_app->m_renderer->SetupMatricesFor3D();
#endif
                        cursorRendered = true;
                    }                
                    else
                    {
                        // Looking at empty landscape
						Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
#ifdef RENDER_CURSOR_3D
                        Vector3 front = (landNormal ^ g_upVector).Normalise();
                        m_cursorHighlight->SetAnimation( false );
                        m_cursorHighlight->SetSize( 30.0f );
                        m_cursorHighlight->Render3D( mousePos, front, landNormal );                                
#else
						float posX, posY;
                        g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
						g_app->m_renderer->SetupMatricesFor2D();
                        m_cursorHighlight->SetAnimation( false );
                        m_cursorHighlight->SetSize( 30.0f );
                        m_cursorHighlight->Render( posX, g_app->m_renderer->ScreenH() - posY );   
						g_app->m_renderer->SetupMatricesFor3D();
#endif
                        cursorRendered = true;
                    }
                }
                else
                {
                    // We don't have a team, Looking at empty landscape
					Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(mousePos.x, mousePos.z);
#ifdef RENDER_CURSOR_3D
                    Vector3 front = (landNormal ^ g_upVector).Normalise();
                    m_cursorHighlight->SetAnimation( false );
                    m_cursorHighlight->SetSize( 30.0f );
                    m_cursorHighlight->Render3D( mousePos, front, landNormal );                                
#else
					float posX, posY;
                    g_app->m_camera->Get2DScreenPos( mousePos, &posX, &posY );
					g_app->m_renderer->SetupMatricesFor2D();
                    m_cursorHighlight->SetAnimation( false );
                    m_cursorHighlight->SetSize( 30.0f );
                    m_cursorHighlight->Render( posX, g_app->m_renderer->ScreenH() - posY );   
					g_app->m_renderer->SetupMatricesFor3D();
#endif
                    cursorRendered = true;
                }
            }
        }
    }
                  
    if( !cursorRendered &&
        (g_inputManager->getInputMode() != INPUT_MODE_GAMEPAD 
        //|| g_inputManager->controlEvent( ControlShowMouseCursor )
         ) )
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
    

    //
    // Special case : officers toggling formation mode

	if( g_app->m_location )
    {
        Team *myTeam = g_app->m_location->GetMyTeam();
        if( myTeam )
        {
            Entity *selected = myTeam->GetMyEntity();            
            if( selected && selected->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *)selected;

                if( !g_app->IsSinglePlayer() && officer->IsFormationToggle( myTeam->m_currentMousePos ) )
                {
                    Vector3 front = ( myTeam->m_currentMousePos - officer->m_pos ).Normalise();
                    RenderFormation( officer->m_pos, front );
                }
            }
        }
    }


    //
    // Particle effect for selection circle

    if( g_app->m_location && 
        g_app->m_location->GetMyTeam() &&
        g_app->m_location->GetMyTeam()->m_selectionCircleCentre != g_zeroVector )
    {
        Team *team = g_app->m_location->GetMyTeam();

        for( int i = 0; i < 30; ++i )
        {
            float thisAngle = i/30.0f * 2.0f * M_PI;
            float predictedCircleRadius = team->m_selectionCircleRadius;
            thisAngle += GetHighResTime() * 0.5f;
            Vector3 pos = team->m_selectionCircleCentre;
            Vector3 angle( cosf(thisAngle) * predictedCircleRadius, 0, 
                           sinf(thisAngle) * predictedCircleRadius );

            Vector3 movement = angle ^ g_upVector;            
            movement.SetLength( 10 );
            Vector3 finalPos = pos + angle;
            finalPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( finalPos.x, finalPos.z );
            finalPos.y += 10;

            g_app->m_particleSystem->CreateParticle( finalPos, g_zeroVector, Particle::TypeControlFlash, predictedCircleRadius );            
        }
    }



    RenderMarkers();
    
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());

    END_PROFILE(  "Render GameCursor" );
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
    AppDebugAssert( false );
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
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f).GetData() );
    glEnd();
    
    RGBAColour colour;
    if( !g_app->m_location->GetMyTeam() || g_app->IsSinglePlayer() )
    {
        colour.Set( 255, 255, 75, 255 * _alpha );
    }
    else
    {
        colour = g_app->m_location->GetMyTeam()->m_colour;
        colour.a = 255 * _alpha;
    }

    glColor4ubv     ( colour.GetData() );
	glBlendFunc		( GL_SRC_ALPHA, GL_ONE );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_selectionArrowFilename ) );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 1 );       glVertex2dv( (pos - rightAngle * _size / 2.0f).GetData() );
        glTexCoord2i( 0, 0 );       glVertex2dv( (pos - rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex2dv( (pos + rightAngle * _size / 2.0f + gradient * _size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex2dv( (pos + rightAngle * _size / 2.0f).GetData() );
    glEnd();

    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

}


void GameCursor::RenderOffscreenArrow( Vector3 &fromPos, Vector3 &toPos, float alpha )
{
    if( g_app->Multiplayer() ) return;
	if( !g_app->m_location->m_landscape.IsInLandscape( toPos ) ) return;

    Vector3 midpoint = ( fromPos + toPos ) / 2.0f;
    float camDistance = ( g_app->m_camera->GetPos() - midpoint ).Mag();

    float spotSize = camDistance / 100.0f;
    float stepSize = spotSize * 4.0f;

    RGBAColour colour = g_app->m_location->GetMyTeam()->m_colour;

    if( !g_app->m_location->IsWalkable( fromPos, toPos ) )
    {
        colour.r *= 0.2f;
        colour.g *= 0.2f;
        colour.b *= 0.2f;        
    }

    Vector3 pos = toPos;
    float distance = ( fromPos - toPos ).Mag();
    int numSteps = distance / stepSize;
    Vector3 step = ( fromPos - toPos ) / numSteps;

    Vector3 camUp = g_app->m_camera->GetUp() * spotSize;
    Vector3 camRight = g_app->m_camera->GetRight() * spotSize;

    float thisAlpha = ( distance / 500.0f );       
    thisAlpha *= alpha;
    thisAlpha = max( thisAlpha, 0.0f );
    thisAlpha = min( thisAlpha, 1.0f );

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/particle.bmp" ) );
    glEnable        ( GL_BLEND );

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for( int i = 0; i < numSteps; ++i )
    {
        float thisAlpha2 = thisAlpha * ( 0.5f + sinf(GetHighResTime() * 10 + i * 0.7f) * 0.5f );
        
        colour.a = thisAlpha2 * 255;
        glColor4ubv( colour.GetData() );
        
        pos += step;
        Vector3 thisPos = pos;
        thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        if( thisPos.y < 0 ) thisPos.y = 0;
        thisPos.y += 1;
        
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (thisPos - camUp).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (thisPos + camRight).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (thisPos + camUp).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (thisPos - camRight).GetData() );
        glEnd();
    }

    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );
}


void GameCursor::RenderSelectionArrows( WorldObjectId _id, Vector3 &_pos, bool darwinians )
{
	Entity *ent = g_app->m_location->GetEntity( _id );
	Unit *unit = g_app->m_location->GetUnit( _id );
	
	if( ent && ent->m_dead ) return;
	if( unit && unit->NumAliveEntities() == 0 ) return;
    if( unit && unit->m_troopType == Entity::TypeInsertionSquadie &&
        g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD) return;

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

    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();

    if( angle <= 0.0f &&
        screenX >= 0 && screenX < screenW &&
        screenY >= 0 && screenY < screenH )
    {
        RenderOffscreenArrow( _pos, mousePos, 0.5f );

        // _pos is onscreen
        if( !darwinians )
        {
            float camDist = toCam.Mag();
	        m_selectionArrowBoost -= g_advanceTime * 0.4f;

            float distanceOut = 1000 / sqrtf( camDist );
            float alpha = min( m_selectionArrowBoost, 0.9f );

            if( camDist > 200.0f )
            {
                alpha = max( min( ( camDist - 200.0f ) / 200.0f, 0.9f ), alpha );
            }

		    alpha = max( alpha, 0.1f );
			g_app->m_renderer->SetupMatricesFor2D();
			RenderSelectionArrow( screenX, screenY - distanceOut, 0, -1, triSize, alpha );
            RenderSelectionArrow( screenX, screenY + distanceOut, 0, 1, triSize, alpha );
            RenderSelectionArrow( screenX - distanceOut, screenY, -1, 0, triSize, alpha );
            RenderSelectionArrow( screenX + distanceOut, screenY, 1, 0, triSize, alpha );
            g_app->m_renderer->SetupMatricesFor3D();  

            if( !onScreen ) BoostSelectionArrows(2.0f);
        }
        onScreen = true;
    }
    else
    {
        RenderOffscreenArrow( _pos, mousePos, 0.9f );

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
    m_shadowed(true),
    m_additive(true)
{
    char fullFilename[512];
	sprintf( fullFilename, "%s", _filename);
	m_mainFilename = strdup(fullFilename);

    BinaryReader *binReader = g_app->m_resource->GetBinaryReader( m_mainFilename );
	AppReleaseAssert(binReader, "Failed to open mouse cursor resource %s", _filename); 
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
		size += iv_abs(sinf(g_gameTime * 4.0f)) * size * 0.6f;
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


void MouseCursor::SetColour(const RGBAColour &_col )
{
    m_colour = _col;
}


void MouseCursor::Render(float _x, float _y, float _rotateAngle)
{
	float s = GetSize();	
	float x = (float)_x - s * m_hotspotX;
	float y = (float)_y - s * m_hotspotY;

	glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE);
    glDepthMask     (false);

	glPushMatrix();
	glTranslatef(x+(s/2),y+(s/2),0);
	glRotatef(_rotateAngle, 0.0f, 0.0f, 1.0f);

    if( m_shadowed )
    {
		glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_shadowFilename));
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.0f );
	    glBegin         (GL_QUADS);
		    glTexCoord2i(0,1);			glVertex2f(-s/2, -s/2);
		    glTexCoord2i(1,1);			glVertex2f(s/2, -s/2);
		    glTexCoord2i(1,0);			glVertex2f(s/2, s/2);
		    glTexCoord2i(0,0);			glVertex2f(-s/2, s/2);
	    glEnd();
    }
    
    glColor4ubv     (m_colour.GetData() );
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	
    if( m_additive )    glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							
    else                glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							

	glBegin         (GL_QUADS);
		glTexCoord2i(0,1);			glVertex2f(-s/2, -s/2);
	    glTexCoord2i(1,1);			glVertex2f(s/2, -s/2);
	    glTexCoord2i(1,0);			glVertex2f(s/2, s/2);
	    glTexCoord2i(0,0);			glVertex2f(-s/2, s/2);
	glEnd();

	glPopMatrix();

    glDepthMask     (true);
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


void MouseCursor::Render3D(Vector3 const &_pos, bool _cameraScale )
{
    Vector3 up = g_app->m_location->m_landscape.m_normalMap->GetValue(_pos.x, _pos.y);

    Vector3 front( 1, 0, 0 );
    Vector3 rightAngle = (front ^ up).Normalise();   
    front = up ^ rightAngle;

    Render3D( _pos, front, up, _cameraScale );
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

        if( camDist > 300 )
        {
            scale += (camDist-300)/20.0f;
        }
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
            glTexCoord2i(0,1);      glVertex3dv( (pos).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos - _front * scale + rightAngle * scale).GetData() );
            glTexCoord2i(0,0);      glVertex3dv( (pos - _front * scale).GetData() );
        glEnd();
    }

    glColor4ubv     (m_colour.GetData());
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(m_mainFilename));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos - _front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (pos - _front * scale).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );    
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}








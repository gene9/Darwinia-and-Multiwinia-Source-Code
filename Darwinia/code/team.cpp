#include "lib/universal_include.h"

#include <math.h>

#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/text_renderer.h"
#include "lib/shape.h"
#include "lib/preferences.h"
#include "lib/debug_render.h"
#include "lib/bitmap.h"
#include "lib/binary_stream_readers.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "level_file.h"
#include "global_world.h"
#include "main.h"
#include "entity_grid.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "gamecursor.h"
#include "taskmanager.h"
#include "control_help.h"

#include "worldobject/engineer.h"
#include "worldobject/entity.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/virii.h"
#include "worldobject/airstrike.h"
#include "worldobject/worldobject.h"
#include "worldobject/darwinian.h"


// ****************************************************************************
//  Class Team
// ****************************************************************************

// *** Constructor
Team::Team()
:   m_teamId(-1),
	m_teamType(TeamTypeUnused),
    m_currentUnitId(-1),
    m_currentEntityId(-1),
    m_currentBuildingId(-1)
{
    m_others.SetTotalNumSlices( NUM_SLICES_PER_FRAME );
	m_others.SetStepSize(100);
	m_units.SetStepSize(5);
	m_name = new char[256];
}


void Team::Initialise(int _teamId)
{
    m_teamId = _teamId;

	sprintf(m_name, "%s", g_app->m_location->m_levelFile->m_teamNames[_teamId]);
	//strcpy(m_name,g_app->m_location->m_levelFile->m_teamNames[_teamId]);

    //
    // Generate the ViriiFull bmp

    if( !g_app->m_resource->DoesTextureExist( "sprites/viriifull.bmp" ) )
    {
	    BinaryReader *reader = g_app->m_resource->GetBinaryReader("sprites/virii.bmp");
		BitmapRGBA little(reader, "bmp");
		delete reader;
	    BitmapRGBA big(32+128, 512);
        big.Clear( RGBAColour(0,0,0) );
        int destY = 0;
        float viriiWidth = 32.0f;

	    for (int i = 0; i < 32; ++i)
	    {
            int destWidth = 32 - i/2;
            int destHeight = 32 - i/2;

            if( destY + destHeight < 512 )
            {
                int targetX = viriiWidth/2 - destWidth/2;
                big.Blit(0,little.m_height,little.m_width, -little.m_height, &little,
                         targetX,destY,destWidth,destHeight, true);
                destY += destHeight;
            }
	    }

		reader = g_app->m_resource->GetBinaryReader("textures/glow.bmp");
        BitmapRGBA glow(reader, "bmp");
		delete reader;
        big.Blit( 0, 0, 128, 128, &glow, 32, 0, 128, 128, true );

	    g_app->m_resource->AddBitmap("sprites/viriifull.bmp", big, true);
    }
}


void Team::SetTeamType(int _teamType)
{
    m_teamType = _teamType;
}


void Team::RegisterSpecial( WorldObjectId _id )
{
    m_specials.PutData( _id );
}


void Team::UnRegisterSpecial( WorldObjectId _id )
{
    for( int i = 0; i < m_specials.Size(); ++i )
    {
        WorldObjectId id = *m_specials.GetPointer(i);
        if( id == _id )
        {
            m_specials.RemoveData(i);
            break;
        }
    }
}


void Team::SelectUnit(int _unitId, int _entityId, int _buildingId )
{
    m_currentBuildingId = _buildingId;
    m_currentUnitId = _unitId;
    m_currentEntityId = _entityId;

    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_gameCursor->BoostSelectionArrows(2.0f);
    }

    if( m_currentUnitId == -1 && m_currentBuildingId == -1 &&
        m_others.ValidIndex(m_currentEntityId) )
    {
        Entity *entity = m_others[m_currentEntityId];
        if( entity && entity->m_type == Entity::TypeOfficer )
        {
            g_app->m_taskManager->SelectTask(-1);
        }
    }

    if( _unitId == -1 && _entityId == -1 && _buildingId == -1 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "TaskManagerDeselectTask", SoundSourceBlueprint::TypeInterface );
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "TaskManagerSelectTask", SoundSourceBlueprint::TypeInterface );
    }

//    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
//    {
//        Vector3 worldpos;
//        if( m_units.ValidIndex(_unitId) )
//        {
//            Unit *unit = m_units[_unitId];
//            worldpos = unit->m_centrePos - g_app->m_camera->GetFront() * 200.0f;
//        }
//        else if( m_others.ValidIndex(_entityId) )
//        {
//            Entity *entity = m_others[_entityId];
//            worldpos = entity->m_pos - g_app->m_camera->GetFront() * 200.0f;
//        }
//    }
}


Unit *Team::GetMyUnit()
{
    if( m_currentUnitId == -1 || !m_units.ValidIndex(m_currentUnitId))
    {
        return NULL;
    }
    else if( m_units.ValidIndex( m_currentUnitId ) )
    {
        return m_units[ m_currentUnitId ];
    }
    else
    {
        return NULL;
    }
}


Entity *Team::RayHitEntity(Vector3 const &_rayStart, Vector3 const &_rayEnd)
{
	// Hit against Units
	for (unsigned int i = 0; i < m_units.Size(); ++i)
	{
		if (m_units.ValidIndex(i))
		{
			Entity *result = m_units[i]->RayHit(_rayStart, _rayEnd);
			if (result)
			{
				return result;
			}
		}
	}

	// Hit against Others
	for (unsigned int i = 0; i < m_others.Size(); ++i)
	{
		if (m_others.ValidIndex(i))
		{
			if (m_others[i]->RayHit(_rayStart, _rayEnd))
			{
				return m_others[i];
			}
		}
	}

	return NULL;
}


Entity *Team::GetMyEntity()
{
    if( m_currentEntityId == -1 )
    {
        return NULL;
    }
    else if( m_others.ValidIndex( m_currentEntityId ) )
    {
        return m_others[ m_currentEntityId ];
    }
    else
    {
        return NULL;
    }
}


Unit *Team::NewUnit(int _troopType, int _numEntities, int *_unitId, Vector3 const &_pos)
{
    *_unitId = m_units.GetNextFree();
    Unit *unit = NULL;

	if (_troopType == Entity::TypeInsertionSquadie)
	{
		unit = new InsertionSquad( m_teamId, *_unitId, _numEntities, _pos );
	}
    else if(_troopType == Entity::TypeSpaceInvader)
    {
        unit = new AirstrikeUnit( m_teamId, *_unitId, _numEntities, _pos );
    }
    else if(_troopType == Entity::TypeVirii)
    {
        unit = new ViriiUnit( m_teamId, *_unitId, _numEntities, _pos );
    }
	else
	{
		unit = new Unit( _troopType, m_teamId, *_unitId, _numEntities, _pos );
	}

    m_units.PutData( unit, *_unitId );
    unit->Begin();
    return unit;
}

Entity *Team::NewEntity(int _troopType, int _unitId, int *_index)
{
	if( _unitId == -1 )
    {
        Entity *entity = Entity::NewEntity( _troopType );
        DarwiniaDebugAssert( entity );
        *_index = m_others.PutData( entity );
        return entity;
    }
    else
    {
        if( m_units.ValidIndex(_unitId) )
        {
            Unit *unit = m_units.GetData(_unitId);
            return unit->NewEntity( _index );
        }
    }

    return NULL;
}

int Team::NumEntities( int _troopType)
{
    int result = 0;
    int i;

    for( i = 0; i < m_units.Size(); ++i )
    {
        if( m_units.ValidIndex(i) )
        {
            Unit *unit = m_units[i];
            if( unit->m_troopType == _troopType )
            {
                result += unit->NumEntities();
            }
        }
    }

    for( i = 0; i < m_others.Size(); ++i )
    {
        if( m_others.ValidIndex(i) )
        {
            Entity *ent = m_others[i];
            if( ent->m_type == _troopType )
            {
                ++result;
            }
        }
    }

    return result;
}


void Team::Advance(int _slice)
{
    //
    // Advance all Units

    if( m_teamType > TeamTypeUnused )
    {
        START_PROFILE(g_app->m_profiler, "Advance Unit Entities");
        for( int unit = 0; unit < m_units.Size(); ++unit )
        {
            if( m_units.ValidIndex(unit) )
            {
                Unit *theUnit = m_units.GetData(unit);
                theUnit->AdvanceEntities(_slice);
            }
        }
        END_PROFILE(g_app->m_profiler, "Advance Unit Entities");

        if( _slice == 0 )
        {
            START_PROFILE(g_app->m_profiler, "Advance Units");
            for( int unit = 0; unit < m_units.Size(); ++unit )
            {
                if( m_units.ValidIndex(unit) )
                {
                    Unit *theUnit = m_units.GetData(unit);
                    bool amIDead = theUnit->Advance( unit );
                    if( amIDead )
                    {
                        m_units.MarkNotUsed(unit);
                        delete theUnit;
                    }
                }
            }
            END_PROFILE(g_app->m_profiler, "Advance Units");
        }
    }


    //
    // Advance all Other entities

    if( m_teamType > TeamTypeUnused )
    {
        START_PROFILE(g_app->m_profiler, "Advance Others");
        int startIndex, endIndex;
        m_others.GetNextSliceBounds(_slice, &startIndex, &endIndex);

        for (int i = startIndex; i <= endIndex; i++)
        {
            if( m_others.ValidIndex(i) )
            {
                Entity *ent = m_others[i];
                if( ent->m_enabled )
                {
                    Vector3 oldPos( ent->m_pos );
                    WorldObjectId myId( m_teamId, -1, i, ent->m_id.GetUniqueId() );

                    char *entityName = Entity::GetTypeName( ent->m_type );
                    START_PROFILE( g_app->m_profiler, entityName );
                    bool amIdead = ent->Advance(NULL);
                    END_PROFILE( g_app->m_profiler, entityName );

#ifdef PROFILER_ENABLED
                    DarwiniaDebugAssert( strcmp(g_app->m_profiler->m_currentElement->m_name, "Advance Others") == 0 );
#endif

                    if( amIdead )
                    {
                        g_app->m_location->m_entityGrid->RemoveObject( myId, oldPos.x, oldPos.z, ent->m_radius );
                        m_others.MarkNotUsed(i);
                        delete ent;
                    }
                    else if( !ent->m_enabled )
                    {
                        g_app->m_location->m_entityGrid->RemoveObject( myId, oldPos.x, oldPos.z, ent->m_radius );
                    }
                    else
                    {
                        g_app->m_location->m_entityGrid->UpdateObject( myId, oldPos.x, oldPos.z, ent->m_pos.x, ent->m_pos.z, ent->m_radius );
                    }
                }
            }
        }

		END_PROFILE(g_app->m_profiler, "Advance Others");
    }
}

void Team::Render()
{
	//
	// Render Units

    START_PROFILE(g_app->m_profiler, "Render Units");

	float timeSinceAdvance = g_predictionTime;

    glEnable        ( GL_TEXTURE_2D );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glEnable        ( GL_BLEND );
	glEnable        ( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.02f );

    for(int i = 0; i < m_units.Size(); ++i )
    {
        if( m_units.ValidIndex(i) )
        {
            Unit *unit = m_units[i];
            if( unit->IsInView() )
            {
                START_PROFILE( g_app->m_profiler, Entity::GetTypeName( unit->m_troopType ) );
                unit->Render(timeSinceAdvance);
                END_PROFILE( g_app->m_profiler, Entity::GetTypeName( unit->m_troopType ) );
            }
        }
    }

	glDisable		( GL_TEXTURE_2D );
	glDisable       ( GL_ALPHA_TEST );
    glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glAlphaFunc     ( GL_GREATER, 0.01f );

    END_PROFILE(g_app->m_profiler, "Render Units");


	//
	// Render Others

	CHECK_OPENGL_STATE();
    START_PROFILE(g_app->m_profiler, "Render Others");
    RenderOthers(timeSinceAdvance);
    END_PROFILE(g_app->m_profiler, "Render Others");
	CHECK_OPENGL_STATE();


    //
    // Render Virii

	CHECK_OPENGL_STATE();
    //if( m_teamId == 1 && m_teamType == TeamTypeCPU )
    //{
        START_PROFILE(g_app->m_profiler, "Render Virii");
        RenderVirii(timeSinceAdvance);
        END_PROFILE(g_app->m_profiler, "Render Virii");
    //}
	CHECK_OPENGL_STATE();


    //
    // Render Darwinians

	CHECK_OPENGL_STATE();
    START_PROFILE(g_app->m_profiler, "Render Darwinians");
    RenderDarwinians(timeSinceAdvance);
    END_PROFILE(g_app->m_profiler, "Render Darwinians");
	CHECK_OPENGL_STATE();
}


void Team::RenderVirii(float _predictionTime)
{
	if (m_others.Size() == 0) return;

    int lastUpdated = m_others.GetLastUpdated();

	float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05f,
							 			   g_app->m_renderer->GetFarPlane());

    //
    // Render Red Virii shapes

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTextureWithAlpha( "sprites/viriifull.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

	glEnable		( GL_BLEND );
    //if( max(max(m_colour.r,m_colour.g),m_colour.b) < 128 )
    //{
    //    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    //}
    //else
    //{
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    //}
//	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );
	glDisable		( GL_CULL_FACE );
    glBegin         ( GL_QUADS );

    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail" );

    for (int i = 0; i <= m_others.Size(); i++)
    {
        if( m_others.ValidIndex(i) )
        {
            Entity *entity = m_others.GetData(i);
            if( entity->m_type == Entity::TypeVirii )
            {
                Virii *virii = (Virii *) entity;
                if( virii->IsInView() )
                {
                    float rangeToCam = ( virii->m_pos - g_app->m_camera->GetPos() ).Mag();
                    int viriiDetail = 1;
                    if      ( entityDetail == 1 && rangeToCam > 1000.0f )        viriiDetail = 2;
                    else if ( entityDetail == 2 && rangeToCam > 1000.0f )        viriiDetail = 3;
                    else if ( entityDetail == 2 && rangeToCam > 500.0f )         viriiDetail = 2;
                    else if ( entityDetail == 3 && rangeToCam > 1000.0f )        viriiDetail = 4;
                    else if ( entityDetail == 3 && rangeToCam > 600.0f )         viriiDetail = 3;
                    else if ( entityDetail == 3 && rangeToCam > 300.0f )         viriiDetail = 2;

                    if( i <= lastUpdated )
                    {
                        virii->Render ( _predictionTime, m_teamId, viriiDetail );
                    }
                    else
                    {
                        virii->Render  ( _predictionTime+SERVER_ADVANCE_PERIOD, m_teamId, viriiDetail );
                    }
                }
            }
        }
    }

    glEnd           ();
    glDisable       ( GL_TEXTURE_2D );
	glDisable		( GL_BLEND );
	glEnable		( GL_CULL_FACE );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		   g_app->m_renderer->GetFarPlane());
}


void Team::RenderDarwinians(float _predictionTime)
{
	if (m_others.Size() == 0) return;

    int lastUpdated = m_others.GetLastUpdated();

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glEnable		( GL_BLEND );
	glEnable        ( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.04f );
	glDisable		( GL_CULL_FACE );

    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
    float highDetailDistanceSqd = 0.0f;
    if      ( entityDetail <= 1 )   highDetailDistanceSqd = 99999.9f;
    else if ( entityDetail == 2 )   highDetailDistanceSqd = 1000.0f;
    else if ( entityDetail >= 3 )   highDetailDistanceSqd = 500.0f;

    highDetailDistanceSqd *= highDetailDistanceSqd;

    for (int i = 0; i <= m_others.Size(); i++)
    {
        if( m_others.ValidIndex(i) )
        {
            Entity *entity = m_others.GetData(i);
            if( entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                if( darwinian->IsInView() )
                {
                    float camDistSqd = ( darwinian->m_pos - g_app->m_camera->GetPos() ).MagSquared();
                    float highDetail = 1.0f - ( camDistSqd / highDetailDistanceSqd );
                    highDetail = max( highDetail, 0.0f );
                    highDetail = min( highDetail, 1.0f );

                    if( i <= lastUpdated )
                    {
                        darwinian->Render ( _predictionTime, highDetail );
                    }
                    else
                    {
                        darwinian->Render  ( _predictionTime+SERVER_ADVANCE_PERIOD, highDetail );
                    }
                }
            }
        }
    }

	glDisable		( GL_ALPHA_TEST);
	glAlphaFunc		( GL_GREATER, 0.01);
    glDisable       ( GL_TEXTURE_2D );
	glDisable		( GL_BLEND );
	glEnable		( GL_CULL_FACE );
}


void Team::RenderOthers(float _predictionTime)
{
    int lastUpdated = m_others.GetLastUpdated();

    for (int i = 0; i <= lastUpdated; i++)
    {
        if( m_others.ValidIndex(i) )
        {
            Entity *entity = m_others.GetData(i);
            if( entity->m_type != Entity::TypeVirii &&
                entity->m_type != Entity::TypeDarwinian &&
                entity->IsInView() )
            {
                START_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );
                entity->Render( _predictionTime );
                END_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );
            }
        }
    }

	int size = m_others.Size();
	_predictionTime += SERVER_ADVANCE_PERIOD;
	for (int i = lastUpdated + 1; i < size; i++)
    {
        if( m_others.ValidIndex(i) )
        {
            Entity *entity = m_others.GetData(i);
            if( entity->m_type != Entity::TypeVirii &&
                entity->m_type != Entity::TypeDarwinian &&
                entity->IsInView() )
            {
                START_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );
                entity->Render( _predictionTime );
                END_PROFILE( g_app->m_profiler, Entity::GetTypeName( entity->m_type ) );
            }
        }
    }

}

// ****************************************************************************
//  Class TeamControls
// ****************************************************************************

#include "lib/input/input.h"

TeamControls::TeamControls()
{
	Clear();
}

unsigned short TeamControls::GetFlags() const
{
	return
		(m_unitMove					? 0x0001 : 0) |
		(m_directUnitMove			? 0x0002 : 0) |
		(m_primaryFireTarget		? 0x0004 : 0) |
		(m_secondaryFireTarget		? 0x0008 : 0) |
		(m_primaryFireDirected		? 0x0010 : 0) |
		(m_secondaryFireDirected	? 0x0020 : 0) |
		(m_cameraEntityTracking		? 0x0040 : 0) |
		(m_unitSecondaryMode        ? 0x0080 : 0) |
		(m_endSetTarget             ? 0x0100 : 0);
}

void TeamControls::SetFlags( unsigned short _flags )
{
	m_unitMove				= _flags & 0x0001 ? 1 : 0;
	m_directUnitMove		= _flags & 0x0002 ? 1 : 0;
	m_primaryFireTarget		= _flags & 0x0004 ? 1 : 0;
	m_secondaryFireTarget	= _flags & 0x0008 ? 1 : 0;
	m_primaryFireDirected	= _flags & 0x0010 ? 1 : 0;
	m_secondaryFireDirected	= _flags & 0x0020 ? 1 : 0;
	m_cameraEntityTracking	= _flags & 0x0040 ? 1 : 0;
	m_unitSecondaryMode     = _flags & 0x0080 ? 1 : 0;
	m_endSetTarget		    = _flags & 0x0100 ? 1 : 0;
}


void TeamControls::Clear()
{
	memset(this, 0, sizeof(*this));
}

void TeamControls::ClearFlags()
{
	m_unitMove				= 0;
	m_directUnitMove		= 0;
	m_primaryFireTarget		= 0;
	m_secondaryFireTarget	= 0;
	m_primaryFireDirected	= 0;
	m_secondaryFireDirected	= 0;
	m_cameraEntityTracking	= 0;
	m_unitSecondaryMode		= 0;
	m_endSetTarget			= 0;
}

void TeamControls::Advance()
{
	if( g_app->m_camera->IsInMode( Camera::ModeBuildingFocus ) ) return;

	m_mousePos = g_app->m_userInput->GetMousePos3d();

	m_primaryFireTarget |= g_inputManager->controlEvent( ControlUnitPrimaryFireTarget );
	m_secondaryFireTarget |= g_inputManager->controlEvent( ControlUnitSecondaryFireTarget );
    m_primaryFireDirected |= g_inputManager->controlEvent( ControlUnitPrimaryFireDirected ) && !g_inputManager->controlEvent( ControlCameraRotate );
	m_secondaryFireDirected |= g_inputManager->controlEvent( ControlUnitSecondaryFireDirected ) /* && g_inputManager->controlEvent( ControlUnitStartSecondaryFireDirected ) */;
	m_cameraEntityTracking |= g_app->m_camera->IsInMode( Camera::ModeEntityTrack );
    m_unitMove |= g_inputManager->controlEvent( ControlUnitSetTarget ) && !m_secondaryFireTarget;
	m_unitSecondaryMode |= g_inputManager->controlEvent( ControlUnitStartSecondaryFireDirected );
	m_endSetTarget |= g_inputManager->controlEvent( ControlUnitEndSetTarget );

	InputDetails details;
	if (g_inputManager->controlEvent( ControlUnitMove, details )) {

		Vector3 right = g_app->m_camera->GetControlVector();
		Vector3 front = g_upVector ^ -right;

		Vector3 waypoint = right * -details.x;
		waypoint += front * -details.y;

		m_directUnitMove = true;
		m_directUnitMoveDx = waypoint.x;
		m_directUnitMoveDy = waypoint.z;

		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondMoveCameraOrUnit);
	}

    if( g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details ) &&
        !g_inputManager->controlEvent( ControlCameraRotate ))
    {
        m_primaryFireDirected = true;
        m_directUnitFireDx = details.x;
        m_directUnitFireDy = details.y;

		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondSquaddieFire);
    }

	if (m_secondaryFireDirected) {
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireAirstrike);
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireGrenades);
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireRocket);
	}
}
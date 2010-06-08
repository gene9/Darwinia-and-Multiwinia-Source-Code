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
#include "lib/filesys/binary_stream_readers.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"
#include "lib/network_stream.h"

#include "network/clienttoserver.h"
#include "network/syncdiff.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
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
#include "main.h"
#include "level_file.h"
#include "multiwinia.h"

#include "worldobject/engineer.h"
#include "worldobject/entity.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/virii.h"
#include "worldobject/airstrike.h"
#include "worldobject/worldobject.h"
#include "worldobject/darwinian.h"
#include "worldobject/gunturret.h"


// ****************************************************************************
//  Class LobbyTeam
// ****************************************************************************

LobbyTeam::LobbyTeam()
:	m_teamId(-1),
	m_clientId(-1),
	m_teamType(TeamTypeUnused),
	m_score(0),
	m_prevScore(0),
	m_iframe(NULL),
    m_rocketId(-1),
    m_colourId(-1),
    m_coopGroupPosition(0),
	m_accurateCPUTakeOverTime(-1.0f),
	m_achievementMask(0),
    m_disconnected(true),
    m_ready(false),
    m_demoPlayer(false)
{
}

LobbyTeam::~LobbyTeam()
{
}


// ****************************************************************************
//  Class Team
// ****************************************************************************

// *** Constructor
Team::Team( LobbyTeam &_lobbyTeam )
:	m_teamId( _lobbyTeam.m_teamId ),
	m_clientId( _lobbyTeam.m_clientId ),
	m_teamType( _lobbyTeam.m_teamType ),
	m_colour( _lobbyTeam.m_colour ),
	m_lobbyTeam( &_lobbyTeam ),
    m_currentUnitId(-1),
    m_currentEntityId(-1),
    m_currentBuildingId(-1),
	//m_currentMouseStatus(0),
	//m_mouseDeltas(0),
    m_taskManager(NULL),
    m_monsterTeam(false),
    m_numDarwiniansSelected(0),
	m_newNumDarwiniansSelected(0),
    m_selectionCircleRadius(0.0),
    m_lastOrdersSet(0.0),
    m_futurewinianTeam(false),
    m_ready(false),
    m_teamKills(0),
    m_oldKills(0),
    m_blockUnitChange(false)
{
    m_others.SetTotalNumSlices( NUM_SLICES_PER_FRAME );
	m_others.SetStepSize(100);
	m_units.SetStepSize(5);

    memset( m_evolutions, 0, sizeof(int) * NumEvolutions );
    m_evolutions[EvolutionFormationSize] = 90;
}

Team::~Team()
{
    m_others.EmptyAndDelete();
    m_units.EmptyAndDelete();
	delete m_taskManager; m_taskManager = NULL;
}


void Team::Initialise()
{
    m_taskManager = new TaskManager(m_teamId);

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
        double viriiWidth = 32.0;

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
        
	    g_app->m_resource->AddBitmap("sprites/viriifull.bmp", big);
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
    if( m_blockUnitChange ) return;

    m_currentBuildingId = _buildingId;
    m_currentUnitId = _unitId;
    m_currentEntityId = _entityId;

    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_gameCursor->BoostSelectionArrows(2.0);
    }

    if( m_currentUnitId == -1 && 
        m_currentBuildingId == -1 &&
        m_others.ValidIndex(m_currentEntityId) )
    {
        Entity *entity = m_others[m_currentEntityId];
        if( entity && entity->m_type == Entity::TypeOfficer  )
        {
            m_taskManager->SelectTask(-1);
        }

        if( entity && m_teamId == g_app->m_globalWorld->m_myTeamId ) 
        {
            g_app->m_soundSystem->TriggerEntityEvent( entity, "Selected" );
        }
    }


    if( //_unitId == -1 && _entityId == -1 && _buildingId == -1 &&
        m_numDarwiniansSelected > 0 )
    {
        for( int i = 0; i < m_others.Size(); ++i )
        {
            if( m_others.ValidIndex(i) )
            {
                Entity *entity = m_others[i];
                if( entity->m_type == Entity::TypeDarwinian )
                {
                    Darwinian *darwinian = (Darwinian *)entity;
                    darwinian->m_underPlayerControl = false;
                }
            }
        }       

		m_newNumDarwiniansSelected = 0;
		m_numDarwiniansSelected = 0;
    }

    

    if( _unitId == -1 && _entityId == -1 && _buildingId == -1 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "TaskManagerDeselectTask", SoundSourceBlueprint::TypeInterface );       
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "TaskManagerSelectTask", SoundSourceBlueprint::TypeInterface );
    }

    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        Building *b = g_app->m_location->GetBuilding( m_currentBuildingId );
        
        if(b && 
           b->m_type == Building::TypeGunTurret && 
           !g_app->m_camera->IsInMode( Camera::ModeTurretAim ) )
        {
            g_app->m_camera->RequestTurretAimMode(b);
        }
    }

    Building *b = g_app->m_location->GetBuilding( m_currentBuildingId );
    if( b && b->m_type == Building::TypeGunTurret )
    {
        ((GunTurret *)b)->m_fireTimer = 0.5f;
             
    }
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
        AppDebugAssert( entity );        
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
            if( unit->m_troopType == _troopType || _troopType == 255 )
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
            if( ent->m_type == _troopType || _troopType == 255 )
            {
                ++result;
            }
        }
    }

    return result;
}

void Team::Advance(int _slice)
{	
    if( m_teamType > TeamTypeUnused )    
    {
        //
        // Advance our task manager
        
        if( _slice == 1 )
        {
            m_taskManager->Advance();
            if( m_teamKills != m_oldKills )
            {
                if( m_teamId == g_app->m_globalWorld->m_myTeamId )
                {
                    if( m_teamKills >= 500 &&
                        m_oldKills < 500 )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_500") );
                    }
                    else if( m_teamKills >= 1000  &&
                        m_oldKills < 1000 )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_1000") );
                    }
                    else if( m_teamKills >= 2000  &&
                        m_oldKills < 2000 )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_2000") );
                    }
                    else if( m_teamKills >= 3000  &&
                        m_oldKills < 3000 )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_3000") );
                    }
                    else if( m_teamKills >= 4000 &&
                        m_oldKills < 4000  )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_4000") );
                    }
                    else if( m_teamKills >= 5000 &&
                        m_oldKills < 5000  )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_kills_5000") );
                    }
                }

                m_oldKills = m_teamKills;
            }
        }

        //
        // Advance the entities in all units

        START_PROFILE( "Advance Unit Entities");
        for( int unit = 0; unit < m_units.Size(); ++unit )
        {
            if( m_units.ValidIndex(unit) )
            {
                Unit *theUnit = m_units.GetData(unit);
                theUnit->AdvanceEntities(_slice);
            }
        }
        END_PROFILE( "Advance Unit Entities");

        //
        // Advance all units

        if( _slice == 0 )
        {
            START_PROFILE( "Advance Units");
            for( int unit = 0; unit < m_units.Size(); ++unit )
            {
                if( m_units.ValidIndex(unit) )
                {
                    Unit *theUnit = m_units.GetData(unit);                    
                    bool amIDead = theUnit->Advance( unit );
#ifdef TRACK_SYNC_RAND
					bool WatchEntities();
					if( WatchEntities() )
						SyncRandLog( " when advancing unit %d\n", unit );
#endif
                    if( amIDead )
                    {
                        m_units.RemoveData(unit);
                        delete theUnit;
                    }
                }
            }
            END_PROFILE( "Advance Units");
        }
    

        //
        // Advance all Other entities

		if( _slice == 0 )
		{
			m_numDarwiniansSelected = m_newNumDarwiniansSelected;
			m_selectedDarwinianCentre = m_newSelectedDarwinianCentre / (double)m_numDarwiniansSelected;

			m_newNumDarwiniansSelected = 0;
			m_newSelectedDarwinianCentre.Zero();
		}

        START_PROFILE( "Advance Others");
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
					int uniqueId = ent->m_id.GetUniqueId();
                    WorldObjectId myId( m_teamId, -1, i, uniqueId );
                    
                    char *entityName = Entity::GetTypeName( ent->m_type );
                    START_PROFILE( entityName );
#ifdef TRACK_SYNC_RAND
                    SyncRandLogText( ent->LogState( "ENTITY BEFORE: " ) );
#endif
                    bool amIdead = ent->Advance(NULL);
#ifdef TRACK_SYNC_RAND
                    SyncRandLogText( ent->LogState( "ENTITY AFTER: " ) );

					bool WatchEntities();
					WatchEntities();
#endif
                    END_PROFILE(  entityName );                    

                    
					if( ent->m_type == Entity::TypeDarwinian &&
						!ent->m_dead && !amIdead &&
						((Darwinian *)ent)->m_underPlayerControl )
					{
						m_newNumDarwiniansSelected++;
						m_newSelectedDarwinianCentre += ent->m_pos;
					}

                    if( amIdead )
                    {
                        g_app->m_location->m_entityGrid->RemoveObject( myId, oldPos.x, oldPos.z, ent->m_radius );
                        m_others.RemoveData(i);                                            
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
 
		END_PROFILE( "Advance Others");
    }
}

void Team::Render()
{
	//
	// Render Units

    START_PROFILE( "Render Units");
    
	double timeSinceAdvance = g_predictionTime;

    glEnable        ( GL_TEXTURE_2D );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glEnable        ( GL_BLEND );
	glEnable        ( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.02 );

    for(int i = 0; i < m_units.Size(); ++i )
    {
        if( m_units.ValidIndex(i) )
        {
            Unit *unit = m_units[i];
            if( unit->IsInView() )
            {
                START_PROFILE( Entity::GetTypeName( unit->m_troopType ) );
                unit->Render(timeSinceAdvance);
                END_PROFILE(  Entity::GetTypeName( unit->m_troopType ) );
            }
        }
    }
    
	glDisable		( GL_TEXTURE_2D );
	glDisable       ( GL_ALPHA_TEST );
    glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glAlphaFunc     ( GL_GREATER, 0.01 );

    END_PROFILE( "Render Units");


	//
	// Render Others

	CHECK_OPENGL_STATE();
    START_PROFILE( "Render Others");
    RenderOthers(timeSinceAdvance);
    END_PROFILE( "Render Others");
	CHECK_OPENGL_STATE();

    
    //
    // Render Virii

	CHECK_OPENGL_STATE();
    START_PROFILE( "Render Virii");
    RenderVirii(timeSinceAdvance);
    END_PROFILE( "Render Virii");
	CHECK_OPENGL_STATE();


    //
    // Render Darwinians

	CHECK_OPENGL_STATE();
    START_PROFILE( "Render Darwinians");
    RenderDarwinians(timeSinceAdvance);
    END_PROFILE( "Render Darwinians");
	CHECK_OPENGL_STATE();

	//
	// Render Sync Errors
	CHECK_OPENGL_STATE();
    START_PROFILE( "Render Sync Errors");
    RenderSyncErrors();
    END_PROFILE( "Render Sync Errors");
	CHECK_OPENGL_STATE();
}


void Team::RenderSyncErrors()
{
	if( g_app->m_hideInterface ) return;
	LList<SyncDiff *> &syncDiffs = m_lobbyTeam->m_syncDifferences;
	int syncDiffsSize = syncDiffs.Size();

	for( int i = 0; i < syncDiffsSize; i++ )
	{
		SyncDiff *syncDiff = syncDiffs.GetData( i );
		syncDiff->Render();
	}
}


void Team::RenderVirii(double _predictionTime)
{    
	if (m_others.Size() == 0) return;
    
    int lastUpdated = m_others.GetLastUpdated();

	double nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.05,
							 			   g_app->m_renderer->GetFarPlane());
        
    //
    // Render Red Virii shapes

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/viriifull.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
	glEnable		( GL_BLEND );
    if( m_monsterTeam )
    {
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    }
    else
    {
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    }
    glDepthMask     ( false );   
	glDisable		( GL_CULL_FACE );
    glBegin         ( GL_QUADS );

    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );

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
                    double rangeToCam = ( virii->m_pos - g_app->m_camera->GetPos() ).Mag();
                    int viriiDetail = 1;
                    if      ( entityDetail == 1 && rangeToCam > 1000.0 )        viriiDetail = 2;
                    else if ( entityDetail == 2 && rangeToCam > 1000.0 )        viriiDetail = 3;
                    else if ( entityDetail == 2 && rangeToCam > 500.0 )         viriiDetail = 2;
                    else if ( entityDetail == 3 && rangeToCam > 1000.0 )        viriiDetail = 4;
                    else if ( entityDetail == 3 && rangeToCam > 600.0 )         viriiDetail = 3;
                    else if ( entityDetail == 3 && rangeToCam > 300.0 )         viriiDetail = 2;
                    
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


void Team::RenderDarwinians(double _predictionTime)
{
	if (m_others.Size() == 0) return;

    int lastUpdated = m_others.GetLastUpdated();

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glEnable		( GL_BLEND );
	glEnable        ( GL_ALPHA_TEST );
    glAlphaFunc     ( GL_GREATER, 0.04 );
	glDisable		( GL_CULL_FACE );
	
	CHECK_OPENGL_STATE();
	
    int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
    double highDetailDistanceSqd = 0.0;
    if      ( entityDetail <= 1 )   highDetailDistanceSqd = 99999.9;
    else if ( entityDetail == 2 )   highDetailDistanceSqd = 1000.0;
    else if ( entityDetail >= 3 )   highDetailDistanceSqd = 500.0;

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
                    double camDistSqd = ( darwinian->m_pos - g_app->m_camera->GetPos() ).MagSquared();
                    double highDetail = 1.0 - ( camDistSqd / highDetailDistanceSqd );
                    highDetail = max( highDetail, 0.0 );
                    highDetail = min( highDetail, 1.0 );

                    if( i <= lastUpdated )
                    {
                        darwinian->Render ( _predictionTime, highDetail );
                    }
                    else
                    {
                        darwinian->Render  ( _predictionTime + SERVER_ADVANCE_PERIOD, highDetail );
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
	CHECK_OPENGL_STATE();
}


void Team::RenderOthers(double _predictionTime)
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
                START_PROFILE( Entity::GetTypeName( entity->m_type ) );
                entity->Render( _predictionTime );
                END_PROFILE(  Entity::GetTypeName( entity->m_type ) );
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
                START_PROFILE( Entity::GetTypeName( entity->m_type ) );
                entity->Render( _predictionTime );
                END_PROFILE(  Entity::GetTypeName( entity->m_type ) );
            }
        }
    }    

}

UnicodeString Team::GetTeamName()
{
	UnicodeString name;
    if( m_lobbyTeam->m_teamName != UnicodeString() )
    {
        name = m_lobbyTeam->m_teamName;

        if( m_teamType == TeamTypeCPU ) 
        {
            if( g_app->m_multiwinia->GameOver() )
            {
                UnicodeString left = LANGUAGEPHRASE("teamtype_left");
                left.Trim();
                name = left + UnicodeString(" ") + name;

                // (LEFT) Chris
            }
            else
            {
                UnicodeString cpu = LANGUAGEPHRASE("teamtype_2");
                cpu.Trim();
                name = cpu + UnicodeString(" ") + name;

                // CPU Chris
            }
        }
        else if( m_lobbyTeam->m_demoPlayer )
        {
            UnicodeString demo = LANGUAGEPHRASE("teamtype_demo");
            demo.Trim();
            name = demo + UnicodeString(" ") + name;            
        }

    }
    else
    {
        if( m_monsterTeam )
		{
			name = LANGUAGEPHRASE("multiwinia_team_virus");
		}
        else if( m_futurewinianTeam )
        {
            name = LANGUAGEPHRASE("multiwinia_team_future");
        }
		else
		{
			switch( m_lobbyTeam->m_colourId )
			{
				case 0: name = LANGUAGEPHRASE("multiwinia_team_green");		break;
				case 1: name = LANGUAGEPHRASE("multiwinia_team_red");		break;
				case 2: name = LANGUAGEPHRASE("multiwinia_team_yellow");	break;
				case 3: name = LANGUAGEPHRASE("multiwinia_team_blue");		break;
				case 4: name = LANGUAGEPHRASE("multiwinia_team_orange");	break;
				case 5: name = LANGUAGEPHRASE("multiwinia_team_purple");	break;
				case 6: name = LANGUAGEPHRASE("multiwinia_team_cyan");		break;
				case 7: name = LANGUAGEPHRASE("multiwinia_team_pink");		break;

				default:    name = UnicodeString("BAD TEAM NAME");			break;
			}

            if( m_teamType == TeamTypeCPU )
            {
                UnicodeString cpu = LANGUAGEPHRASE("teamtype_2");
                cpu.Trim();
                name = cpu + UnicodeString(" ") + name;            
            }
		}
    }


	return name;
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
		(m_targetDirected			? 0x0010 : 0) |
		(m_secondaryFireDirected	? 0x0020 : 0) |
		(m_cameraEntityTracking		? 0x0040 : 0) |
		(m_unitSecondaryMode        ? 0x0080 : 0) |
		(m_endSetTarget             ? 0x0100 : 0) |
		(m_consoleController		? 0x0200 : 0) | 
        (m_leftButtonPressed        ? 0x0400 : 0);
}

void TeamControls::SetFlags( unsigned short _flags )
{
	m_unitMove				= _flags & 0x0001 ? 1 : 0;
	m_directUnitMove		= _flags & 0x0002 ? 1 : 0;
	m_primaryFireTarget		= _flags & 0x0004 ? 1 : 0;
	m_secondaryFireTarget	= _flags & 0x0008 ? 1 : 0;
	m_targetDirected		= _flags & 0x0010 ? 1 : 0;
	m_secondaryFireDirected	= _flags & 0x0020 ? 1 : 0;
	m_cameraEntityTracking	= _flags & 0x0040 ? 1 : 0;
	m_unitSecondaryMode     = _flags & 0x0080 ? 1 : 0;
	m_endSetTarget		    = _flags & 0x0100 ? 1 : 0;
	m_consoleController		= _flags & 0x0200 ? 1 : 0;
    m_leftButtonPressed     = _flags & 0x0400 ? 1 : 0;
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
	m_targetDirected		= 0;
	m_secondaryFireDirected	= 0;
	m_cameraEntityTracking	= 0;
	m_unitSecondaryMode		= 0;
	m_endSetTarget			= 0;
	m_consoleController		= 0;
	m_leftButtonPressed    	= 0;
}

void TeamControls::Pack( char *_data, int &_length ) const
{	
	std::ostrstream s( _data, 256 );

	WriteNetworkValue( s, GetFlags() );
	if( m_directUnitMove )
	{
		WriteNetworkValue( s, m_directUnitMoveDx );
		WriteNetworkValue( s, m_directUnitMoveDy );
	}
	
	if( m_targetDirected || m_secondaryFireDirected )
	{
		WriteNetworkValue( s, m_directUnitTargetDx );
		WriteNetworkValue( s, m_directUnitTargetDy );
	}

	WriteNetworkValue( s, m_mousePos );
	_length = s.tellp();
}

void TeamControls::Unpack( const char *_data, int _length ) 
{
	std::istrstream s( _data, _length );

	unsigned short flags;
	ReadNetworkValue( s, flags );
	SetFlags( flags );

	if( m_directUnitMove )
	{
		ReadNetworkValue( s, m_directUnitMoveDx );
		ReadNetworkValue( s, m_directUnitMoveDy );
	}
	
	if( m_targetDirected || m_secondaryFireDirected )
	{
		ReadNetworkValue( s, m_directUnitTargetDx );
		ReadNetworkValue( s, m_directUnitTargetDy );
	}

	ReadNetworkValue( s, m_mousePos );	
}

void TeamControls::Advance()
{
	Unit* unit = NULL;
	bool canSetMove = true;
	if( g_app->m_location &&
		g_app->m_location->GetMyTeam() ) 
	{
		unit = g_app->m_location->GetMyTeam()->GetMyUnit();
	}

	if( g_inputManager && 
        g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD && 
		unit && 
        unit->m_troopType == Entity::TypeInsertionSquadie )
	{
		canSetMove = false;
	}

	m_mousePos = g_app->m_userInput->GetMousePos3d();
	m_consoleController = g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD;

	m_primaryFireTarget |= g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) && !g_app->m_camera->m_camLookAround;
	m_secondaryFireTarget |= g_inputManager->controlEvent( ControlUnitSecondaryFireTarget );
    m_targetDirected |= g_inputManager->controlEvent( ControlTargetMoveSecondary/*ControlUnitTargetDirected*/ );
	m_secondaryFireDirected |= g_inputManager->controlEvent( ControlUnitSecondaryFireDirected ) /* && g_inputManager->controlEvent( ControlUnitStartSecondaryFireDirected ) */;
	m_cameraEntityTracking |= g_app->m_camera->IsInMode( Camera::ModeEntityTrack ); 
    m_unitMove |= g_inputManager->controlEvent( ControlUnitSetTarget ) && !m_secondaryFireTarget && canSetMove;
	m_unitSecondaryMode |= g_inputManager->controlEvent( ControlIssueSpecialOrders );
	m_endSetTarget |= g_inputManager->controlEvent( ControlUnitEndSetTarget );
    m_leftButtonPressed |= g_inputManager->controlEvent(ControlLeftButtonPressed);

	
	InputDetails details;
	if (g_inputManager->controlEvent( ControlUnitMove, details )) {

		Vector3 right = g_app->m_camera->GetControlVector();
		Vector3 front = g_upVector ^ -right;

		Vector3 waypoint = right * -details.x;
		waypoint += front * -details.y;

		// make sure we won't overflow or underflow a short
		AppDebugAssert( SHRT_MIN <= waypoint.x && waypoint.x <= SHRT_MAX );
		AppDebugAssert( SHRT_MIN <= waypoint.y && waypoint.y <= SHRT_MAX );

		m_directUnitMove = true;
		m_directUnitMoveDx = waypoint.x;
		m_directUnitMoveDy = waypoint.z;

		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondMoveCameraOrUnit);
	}

    if( g_inputManager->controlEvent( ControlUnitTargetDirected ))
    {
		g_inputManager->controlEvent( ControlUnitTargetDirected, details );
        Vector3 right = g_app->m_camera->GetControlVector();
		Vector3 front = g_upVector ^ -right;

		Vector3 target = right * -details.x;
		target += front * -details.y;

        m_targetDirected = true;
        m_directUnitTargetDx = target.x;
        m_directUnitTargetDy = target.z;

		int acceleration = 6;
		Vector3 delta(m_directUnitTargetDx, 0, m_directUnitTargetDy);

		if( delta.MagSquared() > 175 )
		{
			acceleration = 256;
		}

		delta.x *= acceleration;
		delta.z *= acceleration;
		g_app->m_camera->m_wantedTargetVector = g_app->m_camera->m_targetVector + delta;
		if( g_app->m_camera->m_wantedTargetVector.MagSquared() > 15625 )
		{
			g_app->m_camera->m_wantedTargetVector.SetLength(125);
		}

		Squadie* pointMan = (Squadie*) g_app->m_camera->m_trackingEntity;
		if( pointMan )
		{
			//AppDebugOut("***** %d , %d\n", g_app->m_camera->m_targetVector.x, g_app->m_camera->m_targetVector.y);
			Vector3 t(pointMan->m_pos + g_app->m_camera->m_targetVector);
			m_directUnitTargetDx = t.x;
			m_directUnitTargetDy = t.z;
		}


		//g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondSquaddieFire);
    }

	if (m_secondaryFireDirected) {
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireAirstrike);
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireGrenades);
		g_app->m_controlHelpSystem->RecordCondUsed(ControlHelpSystem::CondFireRocket);
	}
}


char *Team::GetTeamType( int _teamType )
{
    char* stringId = new char[256];
    sprintf( stringId, "teamtype_%d", _teamType );

    return stringId;
}

bool Team::IsFuturewinianTeam()
{
    bool match = (g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionFuturewinianTeam] == g_app->m_location->GetTeamPosition(m_teamId));
    return match || m_futurewinianTeam;
}

void Team::Eliminate()
{
    for( int u = 0; u < m_units.Size(); ++u )
    {
        if( m_units.ValidIndex(u) )
        {
            Unit *unit = m_units[u];
            for( int e = 0; e < unit->m_entities.Size(); ++e )
            {
                if( unit->m_entities.ValidIndex(e) )
                {
                    Entity *entity = unit->m_entities[e];
                    entity->ChangeHealth( entity->m_stats[Entity::StatHealth] * -2.0f );
                }
            }
        }
    }

    // Kill all ENTITIES
    for( int o = 0; o < m_others.Size(); ++o )
    {
        if( m_others.ValidIndex(o) )
        {
            Entity *entity = m_others[o];
            entity->ChangeHealth( entity->m_stats[Entity::StatHealth] * -2.0f );
        }
    }

    m_taskManager->StopAllTasks();

    //
    // Switch off our trunk ports

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {   
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeTrunkPort &&
                building->m_id.GetTeamId() == m_teamId )
            {
                building->SetTeamId( 255 );
            }
        }
    }
}

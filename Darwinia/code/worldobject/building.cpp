#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/matrix34.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "app.h"
#include "camera.h"
#include "globals.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "particle_system.h"
#include "explosion.h"

#include "sound/soundsystem.h"

#include "network/clienttoserver.h"

#include "worldobject/building.h"
#include "worldobject/cave.h"
#include "worldobject/factory.h"
#include "worldobject/radardish.h"
#include "worldobject/laserfence.h"
#include "worldobject/controltower.h"
#include "worldobject/gunturret.h"
#include "worldobject/bridge.h"
#include "worldobject/powerstation.h"
#include "worldobject/tree.h"
#include "worldobject/wall.h"
#include "worldobject/trunkport.h"
#include "worldobject/library.h"
#include "worldobject/generator.h"
#include "worldobject/mine.h"
#include "worldobject/constructionyard.h"
#include "worldobject/upgrade_port.h"
#include "worldobject/incubator.h"
#include "worldobject/darwinian.h"
#include "worldobject/anthill.h"
#include "worldobject/safearea.h"
#include "worldobject/triffid.h"
#include "worldobject/spiritreceiver.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/blueprintstore.h"
#include "worldobject/ai.h"
#include "worldobject/researchitem.h"
#include "worldobject/scripttrigger.h"
#include "worldobject/spam.h"
#include "worldobject/goddish.h"
#include "worldobject/staticshape.h"
#include "worldobject/rocket.h"
#include "worldobject/switch.h"
#include "worldobject/generichub.h"
#include "worldobject/feedingtube.h"
#include "worldobject/controlstation.h"
#include "worldobject/researchcrate.h"
#include "worldobject/crate.h"



Shape *Building::s_controlPad = NULL;
ShapeMarker *Building::s_controlPadStatus = NULL;



Building::Building()
:   m_front(1,0,0),
    m_radius(13.0f),
	m_timeOfDeath(-1.0f),
	m_shape(NULL),
	m_dynamic(false),
	m_isGlobal(false),
	m_destroyed(false),
	m_minimumPorts(3)
{
    if( !s_controlPad )
    {
        s_controlPad = g_app->m_resource->GetShape( "controlpad.shp" );
        DarwiniaDebugAssert( s_controlPad );

        s_controlPadStatus = s_controlPad->m_rootFragment->LookupMarker( "MarkerStatus" );
        DarwiniaDebugAssert( s_controlPadStatus );
    }

    m_id.SetTeamId(1);

    m_up = g_upVector;
}

void Building::Initialise( Building *_template )
{
    m_id        = _template->m_id;
    m_pos       = _template->m_pos;
    m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    m_front     = _template->m_front;
	m_up        = _template->m_up;
    m_dynamic   = _template->m_dynamic;
    m_isGlobal  = _template->m_isGlobal;
	m_destroyed = _template->m_destroyed;
	m_minimumPorts = _template->m_minimumPorts;

    if( m_shape )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );

        SetShapeLights( m_shape->m_rootFragment );
        SetShapePorts( m_shape->m_rootFragment );
    }
    else
    {
        m_centrePos = m_pos;
        m_radius = 13.0f;
    }

    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_requestedLocationId );
    if( gb ) m_id.SetTeamId( gb->m_teamId );

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Create" );
}


void Building::SetDetail( int _detail )
{
    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if( m_shape )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );

        m_ports.EmptyAndDelete();
        SetShapePorts( m_shape->m_rootFragment );
    }
    else
    {
        m_centrePos = m_pos;
        m_radius = 13.0f;
    }
}


bool Building::Advance()
{
	if( m_destroyed ) return true;

    EvaluatePorts();
    return false;
}

void Building::SetShape( Shape *_shape )
{
    m_shape = _shape;
}

void Building::SetShapeLights ( ShapeFragment *_fragment )
{
    //
    // Enter all lights from this fragment

    int i;

    for( i = 0; i < _fragment->m_childMarkers.Size(); ++i )
    {
        ShapeMarker *marker = _fragment->m_childMarkers[i];
        if( strstr( marker->m_name, "MarkerLight" ) )
        {
            m_lights.PutData( marker );
        }
    }


    //
    // Recurse to all child fragments

    for( i = 0; i < _fragment->m_childFragments.Size(); ++i )
    {
        ShapeFragment *fragment = _fragment->m_childFragments[i];
        SetShapeLights( fragment );
    }
}


void Building::SetShapePorts( ShapeFragment *_fragment )
{
    //
    // Enter all ports from this fragment

    int i;

    Matrix34 buildingMat( m_front, m_up, m_pos );

    for( i = 0; i < _fragment->m_childMarkers.Size(); ++i )
    {
        ShapeMarker *marker = _fragment->m_childMarkers[i];
        if( strstr( marker->m_name, "MarkerPort" ) )
        {
            BuildingPort *port = new BuildingPort();
            port->m_marker = marker;
            port->m_mat = marker->GetWorldMatrix(buildingMat);
            port->m_mat.pos = PushFromBuilding( port->m_mat.pos, 5.0f );
            port->m_mat.pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( port->m_mat.pos.x, port->m_mat.pos.z );

            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                port->m_counter[t] = 0;
            }

            m_ports.PutData( port );
        }
    }



    //
    // Recurse to all child fragments

    for( i = 0; i < _fragment->m_childFragments.Size(); ++i )
    {
        ShapeFragment *fragment = _fragment->m_childFragments[i];
        SetShapePorts( fragment );
    }
}


void Building::Reprogram( float _complete )
{
}


void Building::ReprogramComplete()
{
    g_app->m_soundSystem->TriggerBuildingEvent( this, "ReprogramComplete" );

    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    if( gb )
    {
        //gb->m_online = !gb->m_online;
		gb->m_online = true;
    }

    g_app->m_globalWorld->EvaluateEvents();
}


void Building::SetTeamId( int _teamId )
{
    m_id.SetTeamId( _teamId );

    GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
    if( gb ) gb->m_teamId = _teamId;

    g_app->m_soundSystem->TriggerBuildingEvent( this, "ChangeTeam" );
}


Vector3 Building::PushFromBuilding( Vector3 const &pos, float _radius )
{
    START_PROFILE( g_app->m_profiler, "PushFromBuilding" );

    Vector3 result = pos;

    bool hit = false;
    if( DoesSphereHit( result, _radius ) ) hit = true;

    if( hit )
    {
        Vector3 pushForce = (m_pos - result).SetLength(2.0f);
        while( DoesSphereHit( result, _radius ) )
        {
            result -= pushForce;
        }
    }

    END_PROFILE( g_app->m_profiler, "PushFromBuilding" );

    return result;
}


bool Building::IsInView()
{
    return( g_app->m_camera->SphereInViewFrustum( m_centrePos, m_radius ) );
}


bool Building::PerformDepthSort( Vector3 &_centrePos )
{
    return false;
}


void Building::Render( float predictionTime )
{
#ifdef DEBUG_RENDER_ENABLED
	if (g_app->m_editing)
	{
		Vector3 pos(m_pos);
		pos.y += 5.0f;
		RenderArrow(pos, pos + m_front * 20.0f, 4.0f);
	}
#endif

	if (m_shape)
	{
		Matrix34 mat(m_front, m_up, m_pos);
		m_shape->Render(predictionTime, mat);

        //m_shape->RenderMarkers(mat);
	}
}


void Building::RenderAlphas( float predictionTime )
{
    RenderLights();
    RenderPorts();

    //RenderHitCheck();
    //RenderSphere(m_pos, 300);

//	if (m_shape)
//	{
//		Matrix34 mat(m_front, g_upVector, m_pos);
//		m_shape->RenderMarkers(mat);
//	}
}


void Building::RenderLights()
{
    if( m_id.GetTeamId() != 255 && m_lights.Size() > 0 )
    {
        if( (g_app->m_clientToServer->m_lastValidSequenceIdFromServer % NUM_TEAMS) == m_id.GetTeamId() ||
            g_app->m_editing )
        {
            for( int i = 0; i < m_lights.Size(); ++i )
            {
                ShapeMarker *marker = m_lights[i];
	            Matrix34 rootMat(m_front, m_up, m_pos);
                Matrix34 worldMat = marker->GetWorldMatrix(rootMat);
	            Vector3 lightPos = worldMat.pos;

                float signalSize = 6.0f;
                Vector3 camR = g_app->m_camera->GetRight();
                Vector3 camU = g_app->m_camera->GetUp();

                if( m_id.GetTeamId() == 255 )
	            {
		            glColor3f( 0.5f, 0.5f, 0.5f );
	            }
	            else
	            {
		            glColor3ubv( g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour.GetData() );
	            }

                glEnable        ( GL_TEXTURE_2D );
                glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
	            glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	            glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glDisable       ( GL_CULL_FACE );
                glDepthMask     ( false );

                glEnable        ( GL_BLEND );
                glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

                for( int i = 0; i < 10; ++i )
                {
                    float size = signalSize * (float) i / 10.0f;
                    glBegin( GL_QUADS );
                        glTexCoord2f    ( 0.0f, 0.0f );             glVertex3fv     ( (lightPos - camR * size - camU * size).GetData() );
                        glTexCoord2f    ( 1.0f, 0.0f );             glVertex3fv     ( (lightPos + camR * size - camU * size).GetData() );
                        glTexCoord2f    ( 1.0f, 1.0f );             glVertex3fv     ( (lightPos + camR * size + camU * size).GetData() );
                        glTexCoord2f    ( 0.0f, 1.0f );             glVertex3fv     ( (lightPos - camR * size + camU * size).GetData() );
                    glEnd();
                }

                glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glDisable       ( GL_BLEND );

                glDepthMask     ( true );
                glEnable        ( GL_CULL_FACE );
                glDisable       ( GL_TEXTURE_2D );
            }
        }
    }
}


void Building::EvaluatePorts()
{
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        BuildingPort *port = m_ports[i];
        //port->m_mat = port->m_marker->GetWorldMatrix(buildingMat);
        port->m_occupant.SetInvalid();

        //
        // Look for a valid Darwinian near the port

        int numFound;
        if( g_app->m_location->m_entityGrid )
        {
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetNeighbours( port->m_mat.pos.x, port->m_mat.pos.z, 5.0f, &numFound );
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = ids[i];
                Entity *entity = g_app->m_location->GetEntity( id );
                if( entity && entity->m_type == Entity::TypeDarwinian )
                {
                    Darwinian *darwinian = (Darwinian *) entity;
                    if( darwinian->m_state == Darwinian::StateOperatingPort )
                    {
                        port->m_occupant = id;
                        break;
                    }
                }
            }
        }

        //
        // Update the operation counter

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( port->m_occupant.IsValid() )
            {
                port->m_counter[t] = 0;
            }
            else
            {
                port->m_counter[t]-=4;
                port->m_counter[t] = max( port->m_counter[t], 0 );
            }
        }
    }
}


void Building::RenderPorts()
{
    START_PROFILE( g_app->m_profiler, "RenderPorts" );

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail" );

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        Vector3 portPos;
        Vector3 portFront;
        GetPortPosition( i, portPos, portFront );


        //
        // Render the port shape

        portPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( portPos.x, portPos.z ) + 0.5f;
        Vector3 portUp = g_upVector;
        Matrix34 mat( portFront, portUp, portPos );

        if( buildingDetail < 3 )
        {
            g_app->m_renderer->SetObjectLighting();
            s_controlPad->Render( 0.0f, mat );
            g_app->m_renderer->UnsetObjectLighting();
        }


        //
        // Render the status light

        float size = 6.0f;

        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = s_controlPadStatus->GetWorldMatrix( mat ).pos;

        if( GetPortOccupant(i).IsValid() )      glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );
        else                                    glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );

        glDisable       ( GL_CULL_FACE );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDepthMask     ( false );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex3fv( (statusPos - camR - camU).GetData() );
            glTexCoord2i( 1, 0 );           glVertex3fv( (statusPos + camR - camU).GetData() );
            glTexCoord2i( 1, 1 );           glVertex3fv( (statusPos + camR + camU).GetData() );
            glTexCoord2i( 0, 1 );           glVertex3fv( (statusPos - camR + camU).GetData() );
        glEnd();
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
        glDepthMask     ( true );
        glDisable       ( GL_TEXTURE_2D );
        glEnable        ( GL_CULL_FACE );
    }

    END_PROFILE( g_app->m_profiler, "RenderPorts" );
}


void Building::RenderHitCheck()
{
#ifdef DEBUG_RENDER_ENABLED
	if (m_shape)
	{
		Matrix34 mat(m_front, m_up, m_pos);
		m_shape->RenderHitCheck(mat);
	}
	else
	{
		RenderSphere(m_pos, m_radius);
	}
#endif
}


void Building::RenderLink()
{
#ifdef DEBUG_RENDER_ENABLED
    int buildingId = GetBuildingLink();
    if( buildingId != -1 )
    {
        Building *linkBuilding = g_app->m_location->GetBuilding( buildingId );
        if( linkBuilding )
        {
			Vector3 start = m_pos;
			start.y += 10.0f;
			Vector3 end = linkBuilding->m_pos;
			end.y += 10.0f;
			RenderArrow(start, end, 6.0f, RGBAColour(255,0,255));
        }
    }
#endif
}

void Building::Damage( float _damage )
{
    g_app->m_soundSystem->TriggerBuildingEvent( this, "Damage" );
}

void Building::Destroy( float _intensity )
{
	m_destroyed = true;

	Matrix34 mat( m_front, g_upVector, m_pos );
	for( int i = 0; i < 3; ++i )
	{
		g_explosionManager.AddExplosion( m_shape, mat );
	}
	g_app->m_location->Bang( m_pos, _intensity, _intensity/4.0f );

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );

	for( int i = 0; i < (int)(_intensity/4.0f); ++i )
	{
		Vector3 vel(0.0f,0.0f,0.0f);
		vel.x += syncsfrand(100.0f);
		vel.y += syncsfrand(100.0f);
		vel.z += syncsfrand(100.0f);
		g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeExplosionCore, 100.0f);
	}
}

bool Building::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir,
                          float _rayLen, Vector3 *_pos, Vector3 *norm )
{
	if (m_shape)
	{
		RayPackage ray(_rayStart, _rayDir, _rayLen);
		Matrix34 transform(m_front, m_up, m_pos);
		return m_shape->RayHit(&ray, transform, true);
	}
	else
	{
		return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
	}
}

bool Building::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    if(m_shape)
    {
        SpherePackage sphere(_pos, _radius);
        Matrix34 transform(m_front, m_up, m_pos);
        return m_shape->SphereHit(&sphere, transform);
    }
    else
    {
        float distance = (_pos - m_pos).Mag();
        return( distance <= _radius + m_radius );
    }
}

bool Building::DoesShapeHit(Shape *_shape, Matrix34 _theTransform)
{
    if( m_shape )
    {
        Matrix34 ourTransform(m_front, m_up, m_pos);
        //return m_shape->ShapeHit( _shape, _theTransform, ourTransform, true );
        return _shape->ShapeHit( m_shape, ourTransform, _theTransform, true );
    }
    else
    {
        SpherePackage package( m_pos, m_radius );
        return _shape->SphereHit( &package, _theTransform, true );
    }
}

int Building::GetBuildingLink()
{
    return -1;
}

void Building::SetBuildingLink( int _buildingId )
{
}


int Building::GetNumPorts()
{
    return m_ports.Size();
}


int Building::GetNumPortsOccupied()
{
    int result = 0;
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        if( GetPortOccupant(i).IsValid() ) result++;
    }
    return result;
}


WorldObjectId Building::GetPortOccupant( int _portId )
{
    if( m_ports.ValidIndex( _portId ) )
    {
        BuildingPort *port = m_ports[_portId];
        return port->m_occupant;
    }

    return WorldObjectId();
}


bool Building::GetPortPosition( int _portId, Vector3 &_pos, Vector3 &_front )
{
    if( m_ports.ValidIndex( _portId ) )
    {
        BuildingPort *port = m_ports[_portId];
        _pos = port->m_mat.pos;
        _front = port->m_mat.f;
        return true;
    }

    return false;
}


void Building::OperatePort( int _portId, int _teamId )
{
    if( m_ports.ValidIndex( _portId ) )
    {
        BuildingPort *port = m_ports[_portId];
        port->m_counter[_teamId]++;
        port->m_counter[_teamId] = min( port->m_counter[_teamId], 50 );
    }
}


int Building::GetPortOperatorCount( int _portId, int _teamId )
{
    if( m_ports.ValidIndex( _portId ) )
    {
        BuildingPort *port = m_ports[_portId];
        return port->m_counter[_teamId];
    }

    return -1;
}


char *Building::GetObjectiveCounter()
{
    return "";
}


void Building::Read( TextReader *_in, bool _dynamic )
{
    char *word;

    int buildingId;
    int teamId;

    word = _in->GetNextToken();          buildingId  =        atoi(word);
	word = _in->GetNextToken();          m_pos.x     = (float)atof(word);
	word = _in->GetNextToken();			 m_pos.z     = (float)atof(word);
	word = _in->GetNextToken();			 teamId      =        atoi(word);
    word = _in->GetNextToken();          m_front.x   = (float)atof(word);
    word = _in->GetNextToken();          m_front.z   = (float)atof(word);
	word = _in->GetNextToken();			 m_isGlobal  =  (bool)atoi(word);

	m_front.Normalise();
    m_id.Set( teamId, UNIT_BUILDINGS, -1, buildingId );
	m_dynamic = _dynamic;
}

void Building::Write( FileWriter *_out )
{
    _out->printf( "\t%-20s", GetTypeName(m_type) );

    _out->printf( "%-8d",    m_id.GetUniqueId());
	_out->printf( "%-8.2f",  m_pos.x);
	_out->printf( "%-8.2f",  m_pos.z);
	_out->printf( "%-8d",    m_id.GetTeamId() );
    _out->printf( "%-8.2f",  m_front.x);
    _out->printf( "%-8.2f",  m_front.z);
	_out->printf( "%-8d",	 m_isGlobal);
}

Building *Building::CreateBuilding( char *_name )
{
    for( int i = 0; i < NumBuildingTypes; ++i )
    {
        if( stricmp( _name, GetTypeName(i) ) == 0 )
        {
            return CreateBuilding(i);
        }
    }

    //DarwiniaDebugAssert(false);
	return NULL;
}

Building *Building::CreateBuilding( int _type )
{
    Building *building = NULL;

    switch( _type )
    {
        case TypeFactory :              building = new Factory();               break;
        case TypeCave :                 building = new Cave();                  break;
        case TypeRadarDish :            building = new RadarDish();             break;
        case TypeLaserFence :           building = new LaserFence();            break;
        case TypeControlTower :         building = new ControlTower();          break;
        case TypeGunTurret:             building = new GunTurret();             break;
        case TypeBridge:                building = new Bridge();                break;
		case TypePowerstation:          building = new Powerstation();	        break;
        case TypeTree:                  building = new Tree();                  break;
        case TypeWall:                  building = new Wall();                  break;
        case TypeTrunkPort:             building = new TrunkPort();             break;
        case TypeResearchItem:          building = new ResearchItem();          break;
        case TypeLibrary:               building = new Library();               break;
        case TypeGenerator:             building = new Generator();             break;
        case TypePylon:                 building = new Pylon();                 break;
        case TypePylonStart:            building = new PylonStart();            break;
        case TypePylonEnd:              building = new PylonEnd();              break;
        case TypeSolarPanel:            building = new SolarPanel();            break;
        case TypeTrackLink:             building = new TrackLink();             break;
        case TypeTrackJunction:         building = new TrackJunction();         break;
        case TypeTrackStart:            building = new TrackStart();            break;
        case TypeTrackEnd:              building = new TrackEnd();              break;
        case TypeRefinery:              building = new Refinery();              break;
        case TypeMine:                  building = new Mine();                  break;
        case TypeYard:                  building = new ConstructionYard();      break;
        case TypeDisplayScreen:         building = new DisplayScreen();         break;
		case TypeUpgradePort:	        building = new UpgradePort;			    break;
        case TypePrimaryUpgradePort:    building = new PrimaryUpgradePort();    break;
        case TypeIncubator:             building = new Incubator();             break;
        case TypeAntHill:               building = new AntHill();               break;
        case TypeSafeArea:              building = new SafeArea();              break;
        case TypeTriffid:               building = new Triffid();               break;
        case TypeSpiritReceiver:        building = new SpiritReceiver();        break;
        case TypeReceiverLink:          building = new ReceiverLink();          break;
        case TypeReceiverSpiritSpawner: building = new ReceiverSpiritSpawner(); break;
        case TypeSpiritProcessor:       building = new SpiritProcessor();       break;
        case TypeSpawnPoint:            building = new SpawnPoint();            break;
        case TypeSpawnPopulationLock:   building = new SpawnPopulationLock();   break;
        case TypeSpawnPointMaster:      building = new MasterSpawnPoint();      break;
        case TypeSpawnLink:             building = new SpawnLink();             break;
        case TypeBlueprintStore:        building = new BlueprintStore();        break;
        case TypeBlueprintConsole:      building = new BlueprintConsole();      break;
        case TypeBlueprintRelay:        building = new BlueprintRelay();        break;
        case TypeAITarget:              building = new AITarget();              break;
        case TypeAISpawnPoint:          building = new AISpawnPoint();          break;
        case TypeScriptTrigger:         building = new ScriptTrigger();         break;
        case TypeSpam:                  building = new Spam();                  break;
        case TypeGodDish:               building = new GodDish();               break;
        case TypeStaticShape:           building = new StaticShape();           break;
        case TypeFuelGenerator:         building = new FuelGenerator();         break;
        case TypeFuelPipe:              building = new FuelPipe();              break;
        case TypeFuelStation:           building = new FuelStation();           break;
        case TypeEscapeRocket:          building = new EscapeRocket();          break;
        case TypeFenceSwitch:           building = new FenceSwitch();           break;
        case TypeDynamicHub:            building = new DynamicHub();            break;
        case TypeDynamicNode:           building = new DynamicNode();           break;
        case TypeFeedingTube:           building = new FeedingTube();           break;
        case TypeControlStation:        building = new ControlStation();        break;
        case TypeSpawnPointRandom:      building = new SpawnPointRandom();      break;
		case TypeResearchCrate:         building = new ResearchCrate();         break;
		case TypeCrate:                 building = new Crate();                 break;
    };


    if( _type == TypeRadarDish ||
        _type == TypeControlTower ||
        _type == TypeControlStation ||
        _type == TypeTrunkPort ||
        _type == TypeIncubator ||
        _type == TypeDynamicHub ||
        _type == TypeFenceSwitch )
    {
        building->m_isGlobal = true;
    }

    return building;
}

int Building::GetTypeId( char const *_name )
{
    for( int i = 0; i < NumBuildingTypes; ++i )
    {
        if( stricmp( _name, GetTypeName(i) ) == 0 )
        {
            return i;
        }
    }
    return -1;
}

char *Building::GetTypeName( int _type )
{
    static char *buildingNames[] = {    "Invalid",
                                        "Factory",                              // These must be in the
                                        "Cave",                                 // Same order as defined
                                        "RadarDish",                            // in building.h
                                        "LaserFence",
                                        "ControlTower",
                                        "GunTurret",
                                        "Bridge",
										"Powerstation",
                                        "Tree",
                                        "Wall",
                                        "TrunkPort",
                                        "ResearchItem",
                                        "Library",
                                        "Generator",
                                        "Pylon",
                                        "PylonStart",
                                        "PylonEnd",
                                        "SolarPanel",
                                        "TrackLink",
                                        "TrackJunction",
                                        "TrackStart",
                                        "TrackEnd",
                                        "Refinery",
                                        "Mine",
                                        "Yard",
                                        "DisplayScreen",
										"UpgradePort",
                                        "PrimaryUpgrade",
                                        "Incubator",
                                        "AntHill",
                                        "SafeArea",
                                        "Triffid",
                                        "SpiritReceiver",
                                        "ReceiverLink",
                                        "SpiritSpawner",
                                        "SpiritProcessor",
                                        "SpawnPoint",
                                        "SpawnPopulationLock",
                                        "SpawnPointMaster",
                                        "SpawnLink",
                                        "AITarget",
                                        "AISpawnPoint",
                                        "BlueprintStore",
                                        "BlueprintConsole",
                                        "BlueprintRelay",
                                        "ScriptTrigger",
                                        "Spam",
                                        "GodDish",
                                        "StaticShape",
                                        "FuelGenerator",
                                        "FuelPipe",
                                        "FuelStation",
                                        "EscapeRocket",
                                        "FenceSwitch",
                                        "DynamicHub",
                                        "DynamicNode",
                                        "FeedingTube",
										"ControlStation",
										"SpawnPointRandom",
										"ResearchCrate",
										"Crate"
                                    };

    if( _type >= 0 && _type < NumBuildingTypes )
    {
        return buildingNames[_type];
    }
    else
    {
        DarwiniaDebugAssert(false);
        return NULL;
    }
}


char *Building::GetTypeNameTranslated( int _type )
{
    char *typeName = GetTypeName(_type);

    char stringId[256];
    sprintf( stringId, "buildingname_%s", typeName );

    if( ISLANGUAGEPHRASE( stringId ) )
    {
        return LANGUAGEPHRASE( stringId );
    }
    else
    {
        return typeName;
    }
}


void Building::ListSoundEvents( LList<char *> *_list )
{
    _list->PutData( "Create" );
    _list->PutData( "Reprogramming" );              // Remove me
    _list->PutData( "ReprogramComplete" );
    _list->PutData( "ChangeTeam" );
    _list->PutData( "Damage" );
}


void Building::RecalculateOwnership()
{
	if ( GetNumPorts() == 0 ) { return; }

    int teamCount[NUM_TEAMS];
	bool teamPresent[NUM_TEAMS];
	for ( int i = 0; i < NUM_TEAMS; i++ )
	{
		teamCount[i] = 0;
		teamPresent[i] = false;
	}

	for ( int i = 0; i < GetNumPorts(); i++ )
	{
        WorldObjectId id = GetPortOccupant(i);
		if( id.IsValid() ) {
			teamPresent[id.GetTeamId()] = true;
		}
	}
	/*
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			if ( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS && g_app->m_location->IsFriend(id.GetTeamId(),m_id.GetTeamId()) ) {
	            teamCount[m_id.GetTeamId()] ++;
			} else {
	            teamCount[id.GetTeamId()] ++;
			}
        }
    }
*/

	for ( int i = 0; i < GetNumPorts(); i++ )
	{
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
			int lowestFriendlyTeam = id.GetTeamId();
			for ( int j = 0; j < NUM_TEAMS; j++ )
			{
				if ( teamPresent[j] && j < lowestFriendlyTeam && g_app->m_location->IsFriend(id.GetTeamId(),j) ) { lowestFriendlyTeam = j; }
			}
			teamCount[lowestFriendlyTeam]++;
		}
	}

    int winningTeam = -1;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] >= m_minimumPorts &&
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 &&
                 teamCount[i] > m_minimumPorts &&
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
	else if ( !g_app->m_location->IsFriend(m_id.GetTeamId(),winningTeam) )
    {
        SetTeamId(winningTeam);
    }
}
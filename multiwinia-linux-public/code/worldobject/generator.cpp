#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#include "worldobject/generator.h"
#include "worldobject/constructionyard.h"
#include "worldobject/darwinian.h"
#include "worldobject/controltower.h"
#include "worldobject/rocket.h"
#include "worldobject/switch.h"
#include "worldobject/laserfence.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"
#include "entity_grid.h"
#include "user_input.h"
#include "team.h"
#include "multiwinia.h"

#include "sound/soundsystem.h"

// ****************************************************************************
// Class PowerBuilding
// ****************************************************************************

PowerBuilding::PowerBuilding()
:   Building(),
    m_powerLink(-1),
    m_powerLocation(NULL)
{    
}

PowerBuilding::~PowerBuilding()
{
	m_surges.EmptyAndDelete();
}

void PowerBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_powerLink = ((PowerBuilding *) _template)->m_powerLink;
}

Vector3 PowerBuilding::GetPowerLocation()
{
    if( !m_powerLocation )
    {
        const char powerLocationName[] = "MarkerPowerLocation";
        m_powerLocation = m_shape->m_rootFragment->LookupMarker( powerLocationName );
        AppReleaseAssert( m_powerLocation, "PowerBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", powerLocationName, m_shape->m_name );
    }

    Matrix34 rootMat( m_front, m_up, m_pos );
    Matrix34 worldPos = m_powerLocation->GetWorldMatrix( rootMat );
    return worldPos.pos;
}


bool PowerBuilding::IsInView()
{
    Building *powerLink = g_app->m_location->GetBuilding( m_powerLink);

    if( powerLink )
    {
        Vector3 midPoint = ( powerLink->m_centrePos + m_centrePos ) / 2.0;
        double radius = ( powerLink->m_centrePos - m_centrePos ).Mag() / 2.0;
        radius += m_radius;
                
        return( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) );
    }
    else
    {
        return Building::IsInView();
    }
}


void PowerBuilding::Render( double _predictionTime )
{
    if( m_shape )
    {
	    Matrix34 mat(m_front, m_up, m_pos);
	    m_shape->Render(_predictionTime, mat);
    }
}


void PowerBuilding::RenderAlphas ( double _predictionTime )
{
    Building::RenderAlphas( _predictionTime );
    
    Building *powerLink = g_app->m_location->GetBuilding( m_powerLink );
    if( powerLink )
    {
        //
        // Render the power line itself 
        PowerBuilding *powerBuilding = (PowerBuilding *) powerLink;

        Vector3 ourPos = GetPowerLocation();
        Vector3 theirPos = powerBuilding->GetPowerLocation();

        Vector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
        Vector3 ourPosRight = camToOurPos ^ ( theirPos - ourPos );
        ourPosRight.SetLength( 2.0 );

        Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
        Vector3 theirPosRight = camToTheirPos ^ ( theirPos - ourPos );
        theirPosRight.SetLength( 2.0 );

        glDisable   ( GL_CULL_FACE );
        glEnable    ( GL_BLEND );
        glDepthMask ( false );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glColor4f   ( 1.0, 1.0, 1.0, 0.0 );

        glBegin( GL_QUADS );
            glTexCoord2f(0.0, 0);      glVertex3dv( (ourPos - ourPosRight * 3).GetData() );
            glTexCoord2f(0.0, 1);      glVertex3dv( (ourPos + ourPosRight * 3).GetData() );
            glTexCoord2f(1.0, 1);      glVertex3dv( (theirPos + theirPosRight * 3).GetData() );
            glTexCoord2f(1.0, 0);      glVertex3dv( (theirPos - theirPosRight * 3).GetData() );
        glEnd();           


        glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
        glColor4f   ( 0.9, 0.9, 0.5, 1.0 );

        glBegin( GL_QUADS );
            glTexCoord2f(0.1, 0);      glVertex3dv( (ourPos - ourPosRight).GetData() );
            glTexCoord2f(0.1, 1);      glVertex3dv( (ourPos + ourPosRight).GetData() );
            glTexCoord2f(0.9, 1);      glVertex3dv( (theirPos + theirPosRight).GetData() );
            glTexCoord2f(0.9, 0);      glVertex3dv( (theirPos - theirPosRight).GetData() );
        glEnd();           

        glBegin( GL_QUADS );
            glTexCoord2f(0.1, 0);      glVertex3dv( (ourPos - ourPosRight).GetData() );
            glTexCoord2f(0.1, 1);      glVertex3dv( (ourPos + ourPosRight).GetData() );
            glTexCoord2f(0.9, 1);      glVertex3dv( (theirPos + theirPosRight).GetData() );
            glTexCoord2f(0.9, 0);      glVertex3dv( (theirPos - theirPosRight).GetData() );
        glEnd();           


        //
        // Render any surges

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

        double surgeSize = 20.0;
        Vector3 camUp = g_app->m_camera->GetUp() * surgeSize;
        Vector3 camRight = g_app->m_camera->GetRight() * surgeSize;

        for( int i = 0; i < m_surges.Size(); ++i )
        {                        
            PowerSurge *thisSurge = m_surges[i];
            double predictedPos = thisSurge->m_percent + _predictionTime * 4;
            if( g_app->Multiplayer() ) predictedPos = thisSurge->m_percent + _predictionTime * 2;
            if( predictedPos < 0.0 ) predictedPos = 0.0;
            if( predictedPos > 1.0 ) predictedPos = 1.0;
            Vector3 thisSurgePos = ourPos + (theirPos-ourPos) * predictedPos;

            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            glColor4f( 1.0, 1.0, 1.0, 0.0 );

            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3dv( (thisSurgePos - camUp * 1.5 - camRight * 1.5).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (thisSurgePos - camUp * 1.5 + camRight * 1.5).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (thisSurgePos + camUp * 1.5 + camRight * 1.5).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (thisSurgePos + camUp * 1.5 - camRight * 1.5).GetData() );
            glEnd();

            if( !g_app->Multiplayer() || thisSurge->m_teamId < 0 || thisSurge->m_teamId >= NUM_TEAMS )
            {
                glColor4f( 0.5, 0.5, 1.0, 1.0 );
            }
            else
            {
                Team *team = g_app->m_location->m_teams[thisSurge->m_teamId];
                glColor4ubv( team->m_colour.GetData() );
            }

            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3dv( (thisSurgePos - camUp - camRight).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (thisSurgePos - camUp + camRight).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (thisSurgePos + camUp + camRight).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (thisSurgePos + camUp - camRight).GetData() );
            glEnd();

            glColor4f( 1.0, 1.0, 1.0, 0.8 );
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3dv( (thisSurgePos - camUp - camRight).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (thisSurgePos - camUp + camRight).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (thisSurgePos + camUp + camRight).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (thisSurgePos + camUp - camRight).GetData() );
            glEnd();
        }

        if( g_app->Multiplayer() ) 
        {
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
            glBegin( GL_QUADS );

            for( int i = 0; i < m_surges.Size(); ++i )
            {                        
                PowerSurge *thisSurge = m_surges[i];
                if( thisSurge && thisSurge->m_teamId >= 0 && thisSurge->m_teamId < NUM_TEAMS )
                {
                    Team *team = g_app->m_location->m_teams[thisSurge->m_teamId];
                    if( !team || team->m_monsterTeam || team->m_futurewinianTeam) continue;

                    for( int j = 1; j <= 3; ++j )
                    {
                        RGBAColour colour = team->m_colour;                
                        colour.a *= ( 0.8 * j );
                        glColor4ubv( colour.GetData() );

                        double predictedPos = thisSurge->m_percent + _predictionTime * 4;
                        if( g_app->Multiplayer() ) predictedPos = thisSurge->m_percent + _predictionTime * 2;
                        predictedPos -= 0.05 * j;
                        if( predictedPos < 0.0 ) predictedPos = 0.0;
                        if( predictedPos > 1.0 ) predictedPos = 1.0;
                        Vector3 thisSurgePos = ourPos + (theirPos-ourPos) * predictedPos;
                        
                        glTexCoord2i( 0, 0 );       glVertex3dv( (thisSurgePos - camUp - camRight).GetData() );
                        glTexCoord2i( 1, 0 );       glVertex3dv( (thisSurgePos - camUp + camRight).GetData() );
                        glTexCoord2i( 1, 1 );       glVertex3dv( (thisSurgePos + camUp + camRight).GetData() );
                        glTexCoord2i( 0, 1 );       glVertex3dv( (thisSurgePos + camUp - camRight).GetData() );
                    }
                }
            }

            glEnd();
        }

        glDisable   ( GL_TEXTURE_2D );
        glDepthMask ( true );
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable   ( GL_BLEND );
        glEnable    ( GL_CULL_FACE );
    }
}

bool PowerBuilding::Advance()
{
    for( int i = 0; i < m_surges.Size(); ++i )
    {
        PowerSurge *thisSurge = m_surges[i];
        double amountToAdd = SERVER_ADVANCE_PERIOD * 4;
        if( g_app->Multiplayer() ) amountToAdd = SERVER_ADVANCE_PERIOD * 2;

        thisSurge->m_percent += amountToAdd;

        if( thisSurge->m_percent >= 1.0 )
        {            
            Building *powerLink = g_app->m_location->GetBuilding( m_powerLink );
            if( powerLink )
            {
                PowerBuilding *powerBuilding = (PowerBuilding *) powerLink;
                powerBuilding->TriggerSurge( 0.0, thisSurge->m_teamId );
            }

            delete thisSurge;
            m_surges.RemoveData(i);
            --i;
        }
    }
    return Building::Advance();
}

void PowerBuilding::TriggerSurge ( double _initValue, int teamId )
{
    PowerSurge *surge = new PowerSurge;
    surge->m_teamId = teamId;
    surge->m_percent = _initValue;

    m_surges.PutDataAtStart( surge );

    g_app->m_soundSystem->TriggerBuildingEvent( this, "TriggerSurge" );
}


void PowerBuilding::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "TriggerSurge" );
}


void PowerBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_powerLink = atoi( _in->GetNextToken() );
}

void PowerBuilding::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_powerLink );
}

int PowerBuilding::GetBuildingLink()
{
    return m_powerLink;
}

void PowerBuilding::SetBuildingLink( int _buildingId )
{
    m_powerLink = _buildingId;
}



// ****************************************************************************
// Class Generator
// ****************************************************************************

Generator::Generator()
:   PowerBuilding(),
    m_throughput(0.0),
    m_timerSync(0.0),
    m_numThisSecond(0),
    m_enabled(false)
{
    m_type = TypeGenerator;
    SetShape( g_app->m_resource->GetShape( "generator.shp" ) );

    const char counterName[] = "MarkerCounter";
    m_counter = m_shape->m_rootFragment->LookupMarker( counterName );
    AppReleaseAssert( m_counter, "Generator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", counterName, m_shape->m_name );
}

void Generator::TriggerSurge( double _initValue, int teamId )
{
    if( m_enabled )
    {
        PowerBuilding::TriggerSurge( _initValue, teamId );
        ++m_numThisSecond;
    }
}


void Generator::GetObjectiveCounter(UnicodeString& _dest)
{
    static wchar_t result[256];
	swprintf( result, sizeof(result)/sizeof(wchar_t),
			  L"%ls : %d Gq/s", LANGUAGEPHRASE("objective_output").m_unicodestring,
			  int(m_throughput*10) );
    _dest = UnicodeString(result);
}


void Generator::ReprogramComplete()
{
    m_enabled = true;
    g_app->m_soundSystem->TriggerBuildingEvent( this, "Enable" );
}


void Generator::ListSoundEvents( LList<char *> *_list )
{
    PowerBuilding::ListSoundEvents( _list );

    _list->PutData( "Enable" );
}


bool Generator::Advance()
{
    if( !m_enabled )
    {
        m_surges.Empty();
        m_throughput = 0.0;
        m_numThisSecond = 0;

        //
        // Check to see if our control tower has been captured.
        // This can happen if a user captures the control tower, exits the level and saves,
        // then returns to the level.  The tower is captured and cannot be changed, but
        // the m_enabled state of this building has been lost.

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeControlTower )
                {
                    ControlTower *tower = (ControlTower *) building;
                    if( tower->GetBuildingLink() == m_id.GetUniqueId() &&
                        tower->m_id.GetTeamId() == m_id.GetTeamId() )
                    {                        
                        m_enabled = true;
                        break;
                    }
                }
            }
        }
    }
    else 
    {
        if( GetNetworkTime() >= m_timerSync + 1.0 )
        {
            double newAverage = m_numThisSecond;
            m_numThisSecond = 0;
            m_timerSync = GetNetworkTime();
            m_throughput = m_throughput * 0.8 + newAverage * 0.2;
        }

        if( m_throughput > 6.5 )
        {
            GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
            gb->m_online = true;
        }
    }
	
    return PowerBuilding::Advance();
}


void Generator::Render( double _predictionTime )
{
    PowerBuilding::Render( _predictionTime );

    //g_gameFont.DrawText3DCentre( m_pos + Vector3(0,215,0), 10.0, "NumThisSecond : %d", m_numThisSecond );    
    
    //if( m_enabled ) g_gameFont.DrawText3DCentre( m_pos + Vector3(0,180,0), 10.0, "Enabled" );
    //g_gameFont.DrawText3DCentre( m_pos + Vector3(0,170,0), 10.0, "Output : %d Gq/s", int(m_throughput*10.0) );    

    Matrix34 generatorMat( m_front, g_upVector, m_pos );
    Matrix34 counterMat = m_counter->GetWorldMatrix(generatorMat);

    glColor4f( 0.6, 0.8, 0.9, 1.0 );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 7.0, "%d", int(m_throughput*10.0));
    counterMat.pos += counterMat.f * 0.1;
    counterMat.pos += ( counterMat.f ^ counterMat.u ) * 0.2;
    counterMat.pos += counterMat.u * 0.2;        
    g_gameFont.SetRenderShadow(true);
    glColor4f( 0.6, 0.8, 0.9, 0.0 );
    g_gameFont.DrawText3D( counterMat.pos, counterMat.f, counterMat.u, 7.0, "%d", int(m_throughput*10.0));
    g_gameFont.SetRenderShadow(false);
}



// ****************************************************************************
// Class Pylon
// ****************************************************************************

Pylon::Pylon()
:   PowerBuilding()
{
    m_type = TypePylon;
    SetShape( g_app->m_resource->GetShape( "pylon.shp" ) );    
}


bool Pylon::Advance()
{    
    return PowerBuilding::Advance();
}


// ****************************************************************************
// Class PylonStart
// ****************************************************************************

PylonStart::PylonStart()
:   PowerBuilding(),
    m_reqBuildingId(-1)
{
    m_type = TypePylonStart;
    SetShape( g_app->m_resource->GetShape( "pylon.shp" ) );    
};


void PylonStart::Initialise( Building *_template )
{
    PowerBuilding::Initialise( _template );

    m_reqBuildingId = ((PylonStart *) _template)->m_reqBuildingId;
}


bool PylonStart::Advance()
{
    //
    // Is the Generator online?

    bool generatorOnline = false;

    int generatorLocationId = g_app->m_globalWorld->GetLocationId("generator");
    GlobalBuilding *globalRefinery = NULL;
    for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
    {
        if( g_app->m_globalWorld->m_buildings.ValidIndex(i) )
        {
            GlobalBuilding *gb = g_app->m_globalWorld->m_buildings[i];
            if( gb && gb->m_locationId == generatorLocationId && 
                gb->m_type == TypeGenerator && gb->m_online )
            {
                generatorOnline = true;
                break;
            }
        }
    }

    if( generatorOnline )
    {
        //
        // Is our required building online yet?
        GlobalBuilding *globalBuilding = g_app->m_globalWorld->GetBuilding( m_reqBuildingId, g_app->m_locationId );
        if( globalBuilding && globalBuilding->m_online )
        {
            if( syncfrand(1) > 0.7 )
            {
                TriggerSurge(0.0);
            }
        }
    }

    return PowerBuilding::Advance();
}


void PylonStart::RenderAlphas( double _predictionTime )
{
    PowerBuilding::RenderAlphas( _predictionTime );

#ifdef DEBUG_RENDER_ENABLED
    if( g_app->m_editing )
    {
        Building *req = g_app->m_location->GetBuilding( m_reqBuildingId );
        if( req )
        {
            RenderArrow( m_pos+Vector3(0,50,0), 
                         req->m_pos+Vector3(0,50,0), 
                         2.0, RGBAColour(255,0,0) );
        }
    }
#endif
}


void PylonStart::Read( TextReader *_in, bool _dynamic )
{
    PowerBuilding::Read( _in, _dynamic );

    m_reqBuildingId = atoi( _in->GetNextToken() );    
}


void PylonStart::Write( TextWriter *_out )
{
    PowerBuilding::Write( _out );

    _out->printf( "%-8d", m_reqBuildingId );
}


// ****************************************************************************
// Class PylonEnd
// ****************************************************************************

PylonEnd::PylonEnd()
:   PowerBuilding()
{
    m_type = TypePylonEnd;
    SetShape( g_app->m_resource->GetShape( "pylon.shp" ) );    
};


void PylonEnd::TriggerSurge( double _initValue, int teamId )
{
    Building *building = g_app->m_location->GetBuilding( m_powerLink );
    
    if( building && building->m_type == Building::TypeYard )
    {
        ConstructionYard *yard = (ConstructionYard *) building;
        yard->AddPowerSurge();
    }

    if( building && building->m_type == Building::TypeFuelGenerator )
    {
        FuelGenerator *fuel = (FuelGenerator *) building;
        fuel->ProvideSurge();
    }

    if( building && building->m_type == Building::TypeLaserFence &&
		g_app->m_location->IsFriend( teamId, building->m_id.GetTeamId() ) )
    {
        LaserFence *fence = (LaserFence *)building;
        fence->ProvidePower();
    }
}


void PylonEnd::RenderAlphas( double _predictionTime )
{
    // Do nothing
}


// ****************************************************************************
// Class SolarPanel
// ****************************************************************************

SolarPanel::SolarPanel()
:   PowerBuilding(),
    m_operating(false),
    m_startingTeam(-1)
{
    m_type = TypeSolarPanel;
    SetShape( g_app->m_resource->GetShape( "solarpanel.shp" ) );

    memset( m_glowMarker, 0, SOLARPANEL_NUMGLOWS * sizeof(ShapeMarker *) );

    for( int i = 0; i < SOLARPANEL_NUMGLOWS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerGlow0%d", i+1 );
        m_glowMarker[i] = m_shape->m_rootFragment->LookupMarker( name );
        AppReleaseAssert( m_glowMarker[i], "SolarPanel: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
    }

    for( int i = 0; i < SOLARPANEL_NUMSTATUSMARKERS; ++i )
    {
        char name[64];
        sprintf( name, "MarkerStatus0%d", i+1 );
        m_statusMarkers[i] = m_shape->m_rootFragment->LookupMarker( name );
        AppReleaseAssert( m_statusMarkers[i], "SolarPanel: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
    }
}

bool SolarPanel::IsOperating()
{
	return m_operating;
}


void SolarPanel::Initialise( Building *_template )
{
    _template->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( _template->m_pos.x, _template->m_pos.z );
    Vector3 right = Vector3( 1, 0, 0 );
    _template->m_front = right ^ _template->m_up;

    PowerBuilding::Initialise( _template );

    if( m_id.GetTeamId() != 255 ) 
    {
        m_startingTeam = m_id.GetTeamId();
    }
}


void SolarPanel::RecalculateOwnership()
{
    if( GetNumPorts() == 0 )
    {
        m_id.SetTeamId(255);
        return;
    }

    if( GetNumPortsOccupied() == 0 )
    {
        m_id.SetTeamId(255);
    }


    //
    // Count darwinians from each team operating the building

    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS * sizeof(int) );

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        if( GetPortOccupant(i).IsValid() )
        {
            Entity *entity = g_app->m_location->GetEntity( GetPortOccupant(i) );
            if( entity && entity->m_type == Entity::TypeDarwinian )
            {
                teamCount[ entity->m_id.GetTeamId() ]++;
            }
        }
    }

    //
    // Who was highest?

    int highestTeamId = -1;
    int highest = 0;

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > highest )
        {
            highest = teamCount[i];
            highestTeamId = i;
        }
    }


    SetTeamId( highestTeamId );
}


bool SolarPanel::Advance()
{
    double fractionOccupied = 0.0f;
    if( GetNumPorts() > 0 )
    {
        fractionOccupied = (double) GetNumPortsOccupied() / (double) GetNumPorts();
    }

    if( m_startingTeam != 255 && m_id.GetTeamId() != 255 &&
        m_startingTeam != m_id.GetTeamId() &&
        GetNumPorts() > 0 &&
        g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
    {
        m_ports.EmptyAndDelete();
    }
    
    if( syncfrand(20.0) <= fractionOccupied )
    {
        TriggerSurge(0.0, m_id.GetTeamId() );
    }
    
    if( fractionOccupied > 0.6 )
    {
        if( !m_operating ) g_app->m_soundSystem->TriggerBuildingEvent( this, "Operate" );
        m_operating = true;
    }

    if( fractionOccupied < 0.3 )
    {
        if( m_operating ) g_app->m_soundSystem->StopAllSounds( m_id, "SolarPanel Operate" );
        m_operating = false;
    }

    if( g_app->Multiplayer() )
    {
        RecalculateOwnership();
    }

    return PowerBuilding::Advance();
}


void SolarPanel::RenderPorts()
{
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    for( int i = 0; i < GetNumPorts(); ++i )
    {        
        Matrix34 rootMat(m_front, m_up, m_pos);
        Matrix34 worldMat = m_statusMarkers[i]->GetWorldMatrix(rootMat);

        
        //
        // Render the status light

        double size = 6.0;

        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = worldMat.pos;

        if( GetPortOccupant(i).IsValid() )      glColor4f( 0.3, 1.0, 0.3, 1.0 );        
        else                                    glColor4f( 1.0, 0.3, 0.3, 1.0 );
        
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );           glVertex3dv( (statusPos - camR - camU).GetData() );
            glTexCoord2i( 1, 0 );           glVertex3dv( (statusPos + camR - camU).GetData() );
            glTexCoord2i( 1, 1 );           glVertex3dv( (statusPos + camR + camU).GetData() );
            glTexCoord2i( 0, 1 );           glVertex3dv( (statusPos - camR + camU).GetData() );
        glEnd();
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}


void SolarPanel::Render( double _predictionTime )
{
    if( g_app->m_editing )
    {
        m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        Vector3 right( 1, 0, 0 );
        m_front = right ^ m_up;
    }

    glShadeModel( GL_SMOOTH );
    PowerBuilding::Render( _predictionTime );
    glShadeModel( GL_FLAT );
}


void SolarPanel::RenderAlphas( double _predictionTime )
{
    PowerBuilding::RenderAlphas( _predictionTime );

    double fractionOccupied = (double) GetNumPortsOccupied() / (double) GetNumPorts();

    if( fractionOccupied > 0.0 && m_id.GetTeamId() != 255 )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        double glowWidth = 60.0;
        double glowHeight = 40.0;
        double alphaValue = 0.5 + iv_abs(iv_sin(g_gameTime)) * 0.5;
        
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        
        RGBAColour teamColour = team->m_colour;
		if( g_app->IsSinglePlayer() )
		{
			teamColour = RGBAColour(100, 100, 250);
		}
        teamColour.a *= alphaValue;


        glEnable        ( GL_BLEND );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        glDepthMask     ( false );
        glDisable       ( GL_CULL_FACE );

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
        glColor4f( alphaValue, alphaValue, alphaValue, 0.0 );

        for( int i = 0; i < SOLARPANEL_NUMGLOWS; ++i )
        {
            Matrix34 thisGlow = m_glowMarker[i]->GetWorldMatrix( mat );

            glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );   glVertex3dv( (thisGlow.pos - thisGlow.r * glowHeight * 0.5 + thisGlow.f * glowWidth* 0.5).GetData() );    
            glTexCoord2i( 0, 1 );   glVertex3dv( (thisGlow.pos + thisGlow.r * glowHeight * 0.5+ thisGlow.f * glowWidth* 0.5).GetData() );    
            glTexCoord2i( 1, 1 );   glVertex3dv( (thisGlow.pos + thisGlow.r * glowHeight * 0.5- thisGlow.f * glowWidth* 0.5).GetData() );    
            glTexCoord2i( 1, 0 );   glVertex3dv( (thisGlow.pos - thisGlow.r * glowHeight * 0.5- thisGlow.f * glowWidth* 0.5).GetData() );                
            glEnd();
        }

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glColor4ubv( teamColour.GetData() );

        for( int i = 0; i < SOLARPANEL_NUMGLOWS; ++i )
        {
            Matrix34 thisGlow = m_glowMarker[i]->GetWorldMatrix( mat );
                    
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );   glVertex3dv( (thisGlow.pos - thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData() );    
                glTexCoord2i( 0, 1 );   glVertex3dv( (thisGlow.pos + thisGlow.r * glowHeight + thisGlow.f * glowWidth).GetData() );    
                glTexCoord2i( 1, 1 );   glVertex3dv( (thisGlow.pos + thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData() );    
                glTexCoord2i( 1, 0 );   glVertex3dv( (thisGlow.pos - thisGlow.r * glowHeight - thisGlow.f * glowWidth).GetData() );                
            glEnd();
        }

        glEnable        ( GL_CULL_FACE );
        glDepthMask     ( true );
        glDisable       ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
    }
}


void SolarPanel::ListSoundEvents( LList<char *> *_list )
{
    PowerBuilding::ListSoundEvents( _list );

    _list->PutData( "Operate" );
}



PowerSplitter::PowerSplitter()
:   PowerBuilding()
{
    m_type = TypePowerSplitter;

    SetShape( g_app->m_resource->GetShape( "pylon.shp" ) );    
}


void PowerSplitter::Initialise( Building *_template )
{
    PowerBuilding::Initialise( _template );

    PowerSplitter *splitter = (PowerSplitter *)_template;

    for( int i = 0; i < splitter->m_links.Size(); ++i )
    {
        m_links.PutData( splitter->m_links[i] );
    }
}


void PowerSplitter::TriggerSurge( double _initValue, int teamId )
{
    for( int i = 0; i < m_links.Size(); ++i )
    {
        Building *building = g_app->m_location->GetBuilding(m_links[i] );
        if( building &&
            building->m_type == Building::TypePylon &&
            building->m_id.GetTeamId() == teamId )
        {
            ((PowerBuilding *)building)->TriggerSurge( _initValue, teamId );
            break;
        }
    }
}


void PowerSplitter::Render( double _predictionTime )
{
    if( g_app->m_editing )
    {
        PowerBuilding::Render( _predictionTime );
    }
}


void PowerSplitter::RenderAlphas( double _predictionTime )
{
    PowerBuilding::RenderAlphas( _predictionTime );

    if( g_app->m_editing )
    {
        for( int i = 0; i < m_links.Size(); ++i )
        {
            Building *building = g_app->m_location->GetBuilding(m_links[i] );
            if( building )
            {
                RenderArrow( m_pos + g_upVector * 50, building->m_pos + g_upVector * 50, 1, RGBAColour(255,0,0,255) );
            }
        }
    }
}


void PowerSplitter::SetBuildingLink( int _buildingId )
{
    m_links.PutData( _buildingId );
}


void PowerSplitter::Read( TextReader *_in, bool _dynamic )
{
    PowerBuilding::Read( _in, _dynamic );

    while( _in->TokenAvailable() )
    {
        int link = atoi( _in->GetNextToken() );
        m_links.PutData( link );
    }
}


void PowerSplitter::Write( TextWriter *_out )
{
    PowerBuilding::Write( _out );

    for( int i = 0; i < m_links.Size(); ++i )
    {
        _out->printf( "%-4d", m_links[i] );
    }
}


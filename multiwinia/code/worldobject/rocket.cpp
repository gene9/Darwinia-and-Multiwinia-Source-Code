#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/text_renderer.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/language_table.h"
#include "lib/math/random_number.h"

#ifdef CHEATMENU_ENABLED
#include "lib/input/input.h"
#include "lib/input/input_types.h"
#endif

#include "worldobject/rocket.h"
#include "worldobject/darwinian.h"
#include "worldobject/ai.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "team.h"
#include "main.h"
#include "particle_system.h"
#include "explosion.h"
#include "global_world.h"
#include "script.h"
#include "entity_grid.h"
#include "renderer.h"
#include "multiwiniahelp.h"
#include "multiwinia.h"

#include "sound/soundsystem.h"

#include "worldobject/spawnpoint.h"


Shape *FuelBuilding::s_fuelPipe = NULL;


FuelBuilding::FuelBuilding()
:   Building(),
    m_fuelLink(-1),
    m_currentLevel(0.0f),
    m_fuelMarker(NULL)
{
    if( !s_fuelPipe )
    {
        s_fuelPipe = g_app->m_resource->GetShape( "fuelpipe.shp" );
        AppDebugAssert( s_fuelPipe );
    }
}


void FuelBuilding::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_fuelLink = ((FuelBuilding *) _template)->m_fuelLink;
}


Vector3 FuelBuilding::GetFuelPosition()
{
    if( !m_fuelMarker )
    {
        const char fuelMarkerName[] = "MarkerFuel";
        m_fuelMarker = m_shape->m_rootFragment->LookupMarker( fuelMarkerName );
        AppReleaseAssert( m_fuelMarker, "FuelBuilding: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", fuelMarkerName, m_shape->m_name );
    }   

    Matrix34 mat( m_front, m_up, m_pos );
    return m_fuelMarker->GetWorldMatrix(mat).pos;
}


void FuelBuilding::ProvideFuel( double _level )
{
    double factor2 = 0.2 * SERVER_ADVANCE_PERIOD;
    if( _level < m_currentLevel ) factor2 *= 3.0;          // Decrease faster than increase

    double factor1 = 1.0 - factor2;

    m_currentLevel = m_currentLevel * factor1 +
                            _level * factor2;

    m_currentLevel = min( m_currentLevel, 1.0 );
    m_currentLevel = max( m_currentLevel, 0.0 );
}


FuelBuilding *FuelBuilding::GetLinkedBuilding()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building )
    {
        if( building->m_type == TypeFuelGenerator ||
            building->m_type == TypeFuelPipe ||
            building->m_type == TypeEscapeRocket ||
            building->m_type == TypeFuelStation )
        {
            FuelBuilding *fuelBuilding = (FuelBuilding *) building;
            return fuelBuilding;
        }
    }

    return NULL;
}


bool FuelBuilding::Advance()
{
    FuelBuilding *fuelBuilding = GetLinkedBuilding();
    if( fuelBuilding )
    {
        fuelBuilding->ProvideFuel( m_currentLevel );
    }

    return Building::Advance();
}


bool FuelBuilding::IsInView()
{
    FuelBuilding *fuelBuilding = GetLinkedBuilding();

    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();
        
        Vector3 midPoint = ( theirPipePos + ourPipePos ) / 2.0;
        double radius = ( theirPipePos - ourPipePos ).Mag() / 2.0;
        radius += m_radius;
        
        return( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) );
    }
    else
    {
        return Building::IsInView();
    }
}


void FuelBuilding::Render( double _predictionTime )
{
    Building::Render( _predictionTime );
    
    FuelBuilding *fuelBuilding = GetLinkedBuilding();
    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();

        Vector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
        Vector3 right = pipeVector ^ g_upVector;
        Vector3 up = pipeVector ^ right;

        ourPipePos += pipeVector * 10;
        
        Matrix34 pipeMat( up, pipeVector, ourPipePos );
        AppDebugAssert( s_fuelPipe );
        s_fuelPipe->Render( _predictionTime, pipeMat );
    }
}


void FuelBuilding::RenderAlphas( double _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    if( m_currentLevel > 0.0f )
    {
        FuelBuilding *fuelBuilding = GetLinkedBuilding();
        if( fuelBuilding )
        {
            Vector3 startPos = GetFuelPosition();
            Vector3 endPos = fuelBuilding->GetFuelPosition();

            Vector3 midPos = ( startPos + endPos ) / 2.0f;
            Vector3 rightAngle = ( g_app->m_camera->GetPos() - midPos ) ^ ( startPos - endPos );
            rightAngle.SetLength( 25.0f );

            glBindTexture       ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/fuel.bmp" ) );
            glEnable            ( GL_TEXTURE_2D );
            glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri     ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );    
            glEnable            ( GL_BLEND );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
            glDepthMask         ( false );
        
            float tx = g_gameTime * -0.5f;
            float tw = 1.0f;

            if( g_app->Multiplayer() ) tx *= 2.0f;

            glColor4f( 1.0f, 0.4f, 0.1f, 0.4f * m_currentLevel );

	        float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	        g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.2f,
							 			           g_app->m_renderer->GetFarPlane());

            int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail" );
            float maxLoops = 4 - buildingDetail;
            maxLoops = max( maxLoops, 1.0f );
            maxLoops = min( maxLoops, 3.0f );
            
            for( int i = 0; i < maxLoops; ++i )
            {
                glBegin( GL_QUADS );
                    glTexCoord2f( tx, 0 );      glVertex3dv( (startPos - rightAngle).GetData() );
                    glTexCoord2f( tx, 1 );      glVertex3dv( (startPos + rightAngle).GetData() );
                    glTexCoord2f( tx+tw, 1 );   glVertex3dv( (endPos + rightAngle).GetData() );
                    glTexCoord2f( tx+tw, 0 );   glVertex3dv( (endPos - rightAngle).GetData() );
                glEnd();
                rightAngle *= 0.7;
            }

	        g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
								 		           g_app->m_renderer->GetFarPlane());

            glEnable( GL_DEPTH_TEST );
            glDisable( GL_TEXTURE_2D );
        }
    }
    
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,70,0), 5, "Fuel Pressure : %2.2f", m_currentLevel );
}


void FuelBuilding::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_fuelLink = atoi( _in->GetNextToken() );
}


void FuelBuilding::Write( TextWriter *_out )
{
    Building::Write( _out );
    
    _out->printf( "%6d ", m_fuelLink );
}


int FuelBuilding::GetBuildingLink()
{
    return m_fuelLink;
}


void FuelBuilding::SetBuildingLink( int _buildingId )
{
    m_fuelLink = _buildingId;   
}

void FuelBuilding::Destroy( double _intensity )
{
	Building::Destroy( _intensity );
	FuelBuilding *fuelBuilding = GetLinkedBuilding();

    if( fuelBuilding )
    {
        Vector3 ourPipePos = GetFuelPosition();
        Vector3 theirPipePos = fuelBuilding->GetFuelPosition();

        Vector3 pipeVector = (theirPipePos - ourPipePos).Normalise();
        Vector3 right = pipeVector ^ g_upVector;
        Vector3 up = pipeVector ^ right;

        ourPipePos += pipeVector * 10;
        
        Matrix34 pipeMat( up, pipeVector, ourPipePos );
		g_explosionManager.AddExplosion( s_fuelPipe, pipeMat );
    }
}
// ============================================================================


FuelGenerator::FuelGenerator()
:   FuelBuilding(),
    m_surges(0.0f),
    m_pump(NULL),
    m_pumpTip(NULL),
    m_pumpMovement(0.0f),
    m_previousPumpPos(0.0f)
{
    m_type = TypeFuelGenerator;

    SetShape( g_app->m_resource->GetShape( "fuelgenerator.shp" ) );
    
    m_pump = g_app->m_resource->GetShape( "fuelgeneratorpump.shp" );

    const char pumpTipName[] = "MarkerTip";
    m_pumpTip = m_pump->m_rootFragment->LookupMarker( pumpTipName );
    AppReleaseAssert( m_pumpTip, "FuelGenerator: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", pumpTipName, m_pump->m_name );
}


void FuelGenerator::ProvideSurge()
{
    m_surges++;
}


bool FuelGenerator::Advance()
{   
    //
    // Advance surges

    double maxSurges = 10;
    if( g_app->Multiplayer() ) maxSurges = 20;

    m_surges -= SERVER_ADVANCE_PERIOD * 1.0f;
    m_surges = min( m_surges, maxSurges );
    m_surges = max( m_surges, 0.0 );
 
    if( m_surges > 8 )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_online = true;
    }


    //
    // Pump fuel

    double fuelVal = m_surges / maxSurges;
    ProvideFuel( fuelVal );
    

    //
    // Spawn particles

    double previousPumpPos = m_previousPumpPos;
    Vector3 pumpPos = GetPumpPos();
    m_previousPumpPos = (pumpPos.y - m_pos.y) / -80.0;

    if( fuelVal > 0.0 && pumpPos.y > m_pos.y - 20.0 )
    {
        pumpPos.x += sfrand(10.0);        
        pumpPos.z += sfrand(10.0);
     
        for( int i = 0; i < int(m_surges); ++i )
        {
            Vector3 pumpVel = g_upVector * 20;
            pumpVel += g_upVector * frand(10);

            Matrix34 mat( m_front, g_upVector, pumpPos );
            Vector3 particlePos = m_pumpTip->GetWorldMatrix(mat).pos;
            double size = 150.0 + frand(150.0);

            g_app->m_particleSystem->CreateParticle( particlePos, pumpVel, Particle::TypeDarwinianFire, size );            
        }
    }


    //
    // Play sounds

    if( previousPumpPos >= 0.1 && m_previousPumpPos < 0.1 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpUp" );
    }
    else if( previousPumpPos <= 0.9 && m_previousPumpPos > 0.9 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpDown" );
    }
    
    return FuelBuilding::Advance();
}


void FuelGenerator::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "PumpUp" );
    _list->PutData( "PumpDown" );
}


Vector3 FuelGenerator::GetPumpPos()
{
    Vector3 pumpPos = m_pos;
    double pumpHeight = 80;

    pumpPos.y -= pumpHeight;
    pumpPos.y += iv_abs( iv_cos( m_pumpMovement ) ) * pumpHeight;

    return pumpPos;
}

void FuelGenerator::Render( double _predictionTime )
{
    FuelBuilding::Render( _predictionTime );

    //
    // Render the pump
    
    float maxSurges = 10;
    if( g_app->Multiplayer() ) maxSurges = 15;

    float fuelVal = m_surges / maxSurges;
    m_pumpMovement += g_advanceTime * fuelVal * 2 * g_gameTimer.GetGameSpeed();

    Vector3 pumpPos = GetPumpPos();
    Matrix34 mat( m_front, g_upVector, pumpPos );

    m_pump->Render( _predictionTime, mat );
}


void FuelGenerator::RenderAlphas( double _predictionTime )
{
    FuelBuilding::RenderAlphas( _predictionTime );
    
//    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,90,0), 10, "Surges : %2.2f", m_surges );
}


void FuelGenerator::GetObjectiveCounter(UnicodeString& _dest)
{
    static wchar_t buffer[256];
	swprintf( buffer, sizeof(buffer)/sizeof(wchar_t),
			  L"%ls %d%%", LANGUAGEPHRASE("objective_fuelpressure").m_unicodestring, int( m_currentLevel * 100 ) );
    _dest = UnicodeString( buffer );
}



// ============================================================================


FuelPipe::FuelPipe()
:   FuelBuilding()
{
    m_type = TypeFuelPipe;

    SetShape( g_app->m_resource->GetShape( "fuelpipebase.shp" ) );
}


bool FuelPipe::Advance()
{
    //
    // Ensure our sound ambiences are playing

    int numInstances = g_app->m_soundSystem->NumInstances( m_id, "FuelPipe PumpFuel" );

    if( m_currentLevel > 0.2 && numInstances == 0 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "PumpFuel" );
    }
    else if( m_currentLevel <= 0.2 && numInstances > 0 )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "FuelPipe PumpFuel" );
    }

    
    return FuelBuilding::Advance();
}


void FuelPipe::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "PumpFuel" );
}


// ============================================================================


FuelStation::FuelStation()
:   FuelBuilding(),
    m_entrance(NULL),
    m_aiTarget(NULL)
{
    m_type = TypeFuelStation;

    SetShape( g_app->m_resource->GetShape( "fuelstation.shp" ) );

    const char entranceName[] = "MarkerEntrance";
    m_entrance = m_shape->m_rootFragment->LookupMarker( entranceName );
    AppReleaseAssert( m_entrance, "FuelStation: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", entranceName, m_shape->m_name );
}


bool FuelStation::IsLoading()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( rocket->m_state == EscapeRocket::StateLoading )
        {
            return true;
        }
    }

    return false;
}


bool FuelStation::Advance()
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( rocket->m_state == EscapeRocket::StateLoading &&
            rocket->SafeToLaunch() )
        {
            if( !m_aiTarget )
            {
                m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

                AITarget targetTemplate;
                targetTemplate.m_pos       = m_pos;
                targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

                m_aiTarget->Initialise( (Building *)&targetTemplate );
                m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
                m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
                m_aiTarget->m_priorityModifier = 1.0f;

                g_app->m_location->m_buildings.PutData( m_aiTarget );
                g_app->m_location->RecalculateAITargets();
            }

            //
            // Find a random Darwinian and make him board

            int myTeam = 0;
            if( g_app->Multiplayer() ) myTeam = m_id.GetTeamId();
            Team *team = g_app->m_location->m_teams[myTeam];

            int numOthers = team->m_others.Size();
            if( numOthers > 0 )
            {
                int numFound = 0;

                bool includeTeam[NUM_TEAMS];
                memset( includeTeam, false, NUM_TEAMS * sizeof(bool) );
                includeTeam[myTeam] = true;

                g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, 300.0, &numFound, includeTeam );

                if( numFound > 0 )
                {
                    int randomIndex = syncrand() % numFound;

                    WorldObjectId id = s_neighbours[randomIndex];
                    Entity *entity = g_app->m_location->GetEntity(id);

                    if( entity && entity->m_type == Entity::TypeDarwinian )
                    {
                        Darwinian *darwinian = (Darwinian *) entity;
                        if( darwinian->m_state == Darwinian::StateIdle ||
                            darwinian->m_state == Darwinian::StateWorshipSpirit )
                        {
                            darwinian->BoardRocket( m_id.GetUniqueId() );
                        }
                    }
                }
            }
        }
        else
        {
            if( m_aiTarget )
            {
                m_aiTarget->m_destroyed = true;
                m_aiTarget->m_neighbours.EmptyAndDelete();
                m_aiTarget = NULL;
            }
        }
    }

    return FuelBuilding::Advance();
}


Vector3 FuelStation::GetEntrance()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    return m_entrance->GetWorldMatrix(mat).pos;
}


bool FuelStation::BoardRocket( WorldObjectId _id )
{
    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        bool result = rocket->BoardRocket( _id );

        if( result )
        {
            Entity *entity = g_app->m_location->GetEntity( _id );
            Vector3 entityPos = entity ? entity->m_pos : g_zeroVector;
            entityPos.y += 2;

            int numFlashes = 4 + AppRandom() % 4;
            for( int i = 0; i < numFlashes; ++i )
            {
                Vector3 vel( sfrand(15.0f), frand(35.0f), sfrand(15.0f) );
                g_app->m_particleSystem->CreateParticle( entityPos, vel, Particle::TypeControlFlash );
            }

            g_app->m_soundSystem->TriggerBuildingEvent( this, "LoadPassenger" );
        }

        return result;
    }

    return false;
}


void FuelStation::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );

    _list->PutData( "LoadPassenger" );
}


void FuelStation::Render( double _predictionTime )
{
    Building::Render( _predictionTime );        
}


void FuelStation::RenderAlphas( double _predictionTime )
{
    // Prevent FuelBuilding::RenderAlphas from being called
       


    //
    // Render countdown

    Building *building = g_app->m_location->GetBuilding( m_fuelLink );
    if( building && building->m_type == TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( (rocket->m_state == EscapeRocket::StateCountdown && rocket->m_countdown <= 10.0f)
            || rocket->m_state == EscapeRocket::StateFlight )
        {
            float screenSize = 60.0f;
            Vector3 screenFront = m_front;
            //screenFront.RotateAroundY( 0.33f * M_PI );
            Vector3 screenRight = screenFront ^ g_upVector;
            Vector3 screenPos = m_pos + Vector3(0,150,0);
            screenPos -= screenRight * screenSize * 0.5f;
            screenPos += screenFront * 30;
            Vector3 screenUp = g_upVector;

            //
            // Render lines for over effect

            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
            glEnable( GL_TEXTURE_2D );
            glEnable( GL_BLEND );
            glDepthMask( false );
            glShadeModel( GL_SMOOTH );
            glDisable( GL_CULL_FACE );

            float texX = 0.0f;
            float texW = 3.0f;
            float texY = g_gameTime*0.01f;
            float texH = 0.3f;

            glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            
            glBegin( GL_QUADS );
                glVertex3dv( screenPos.GetData() );
                glVertex3dv( (screenPos + screenRight * screenSize).GetData() );
                glVertex3dv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );
                glVertex3dv( (screenPos + screenUp * screenSize).GetData() );
            glEnd();

            glColor4f( 1.0, 0.4, 0.2, 1.0 );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            
            for( int i = 0; i < 2; ++i )
            {
                glBegin( GL_QUADS );
                    glTexCoord2f(texX,texY);            
                    glVertex3dv( screenPos.GetData() );

                    glTexCoord2f(texX+texW,texY);       
                    glVertex3dv( (screenPos + screenRight * screenSize).GetData() );
                   
                    glTexCoord2f(texX+texW,texY+texH);  
                    glVertex3dv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );

                    glTexCoord2f(texX,texY+texH);       
                    glVertex3dv( (screenPos + screenUp * screenSize).GetData() );
                glEnd();

                texY *= 1.5;
                texH = 0.1;
            }


            glDepthMask( false );
            
            //
            // Render countdown

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            Vector3 textPos = screenPos 
                                + screenRight * screenSize * 0.5f
                                + screenUp * screenSize * 0.5f;

            if( rocket->m_state == EscapeRocket::StateCountdown )
            {
                int countdown = (int) rocket->m_countdown + 1;            
                g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "%d", countdown );
            }
            else
            {
                if( fmod( g_gameTime, 2 ) < 1 )
                {
                    g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "0" );
                }
            }

            
  
            //
            // Render projection effect

            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

            Vector3 ourPos = m_pos + Vector3(0,90,0);
            Vector3 theirPos = m_pos + Vector3(0,200,0);
            theirPos += screenFront * 30.0f;

            Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
            Vector3 lineTheirPos = camToTheirPos ^ ( ourPos - theirPos );
            lineTheirPos.SetLength( m_radius * 0.5f );
        
            for( int i = 0; i < 3; ++i )
            {
                Vector3 pos = theirPos;
                pos.x += sinf( g_gameTime + i ) * 15;
                pos.y += sinf( g_gameTime + i ) * 15;
                pos.z += cosf( g_gameTime + i ) * 15;

                float blue = 0.5f + fabs( sinf(g_gameTime * i) ) * 0.5f;

                glBegin( GL_QUADS );
                    glColor4f( 1.0f, 0.4f, 0.2f, 0.4f );
                    glTexCoord2f(1,0);      glVertex3dv( (ourPos - lineTheirPos).GetData() );
                    glTexCoord2f(1,1);      glVertex3dv( (ourPos + lineTheirPos).GetData() );
                    glColor4f( 1.0f, 0.4f, 0.2f, 0.2f );
                    glTexCoord2f(0,1);      glVertex3dv( (pos + lineTheirPos).GetData() );
                    glTexCoord2f(0,0);      glVertex3dv( (pos - lineTheirPos).GetData() );
                glEnd();
            }
        
        
            glDepthMask     ( true );
            glEnable        ( GL_DEPTH_TEST );
            glDisable       ( GL_TEXTURE_2D );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable       ( GL_BLEND );        
            glShadeModel    ( GL_FLAT );
        }
    }

}


bool FuelStation::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;
    return true;
}



// ============================================================================


void ConvertFragmentColours ( ShapeFragment *_frag, RGBAColour _col )
{
    RGBAColour *newColours = new RGBAColour[_frag->m_numColours];

    for( int i = 0; i < _frag->m_numColours; ++i )
    {
        newColours[i].r = _col.r;
        newColours[i].g = _col.g;
        newColours[i].b = _col.b;

        // scale colours down - colours on 3d models appear brighter than 2d equivalents due to lighting
        newColours[i] *= 0.3f;
    }
    _frag->RegisterColours( newColours, _frag->m_numColours );


    for( int i = 0; i < _frag->m_childFragments.Size(); ++i )
    {
        ConvertFragmentColours( _frag->m_childFragments[i], _col );
    }
}


EscapeRocket::EscapeRocket()
:   FuelBuilding(),
    m_fuel(0.0f),
    m_pipeCount(0),
    m_passengers(0),
    m_countdown(-1.0f),
    m_booster(NULL),
    m_shadowTimer(0.0f),
    m_state(StateRefueling),
    m_damage(0.0f),
    m_spawnBuildingId(-1),
    m_spawnCompleted(false),
    m_cameraShake(0.0f),
    m_coloured(false),
    m_attackTimer(0.0f),
    m_refuelRate(0.0f),
    m_attackWarning(false),
    m_refueledWarning(false),
    m_countdownWarning(false),
    m_fuelBeforeExplosion(0.0f)
{
    m_type = TypeEscapeRocket;

	//
	// Memory Leak 
	//
	// If it's not Multiplayer, the standard rocket shape should do (and be managed by m_resource)
	// It it's Multiplayer we want the a coloured shape. Currently this is done in an adhoc
	// manner, but it'd be great if there was a method on Resource that did this for us
	// (and then added it to Resource m_shapes)

    m_shape = g_app->m_resource->GetShapeCopy( "rocket.shp", false, !g_app->Multiplayer() );

    const char boosterName[] = "MarkerBooster";
    m_booster = m_shape->m_rootFragment->LookupMarker( boosterName );
    AppReleaseAssert( m_booster, "EscapeRocket: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", boosterName, m_shape->m_name );

    for( int i = 0; i < 3; ++i )
    {
        char name[256];
        sprintf( name, "MarkerWindow0%d", i+1 );
        m_window[i] = m_shape->m_rootFragment->LookupMarker( name );
        AppReleaseAssert( m_window[i] , "EscapeRocket: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", name, m_shape->m_name );
    }
}


EscapeRocket::~EscapeRocket()
{
    if( m_shape )
    {
        delete m_shape;
        m_shape = NULL;
    }
}


void EscapeRocket::ListSoundEvents( LList<char *> *_list )
{
    FuelBuilding::ListSoundEvents( _list );
    
    _list->PutData( "Refueling" );
    _list->PutData( "Happy" );
    _list->PutData( "Unhappy" );
    _list->PutData( "Flight" );
    _list->PutData( "Malfunction" );
    _list->PutData( "Explode" );
    _list->PutData( "EngineBurn" );
}


void EscapeRocket::SetupSounds()
{
    char *requiredSoundName = NULL;

    //
    // What ambience should be playing?

    switch( m_state )
    {
        case StateRefueling:   
            if( m_damage < 75 )
            {
                if( m_currentLevel > 0.2 )     requiredSoundName = "Refueling";                       
                else                            requiredSoundName = "Unhappy";           
            }
            else
            {
                                            requiredSoundName = "Malfunction";
            }
                                            break;
        
        case StateLoading:
        case StateIgnition:
        case StateReady:                
        case StateCountdown:   
            if( m_damage < 75 )             requiredSoundName = "Happy";               
            else                            requiredSoundName = "Malfunction";
                                            break;
    
        case StateExploding:
            if( m_fuel > 0.0f )             requiredSoundName = "Malfunction";           
            else                            requiredSoundName = "Unhappy";            
                                            break;
            
        case StateFlight:                   requiredSoundName = "Flight";
                                            break;            
    }

    char fullName[256];
    if( requiredSoundName ) sprintf( fullName, "EscapeRocket %s", requiredSoundName );


    //
    // If we're not set up right, kill all sounds first


    int numInstances = requiredSoundName ? g_app->m_soundSystem->NumInstances( m_id, fullName ) : 0;

    if( !requiredSoundName || numInstances == 0 )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Refueling" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Happy" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Unhappy" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Malfunction" );
        g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket Flight" );
    }


    //
    // Spawn the correct sound

    if( requiredSoundName && numInstances == 0 )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, requiredSoundName );
    }


    //
    // If our engines are on then trigger the event

    int numEngineInstances = g_app->m_soundSystem->NumInstances( m_id, "EscapeRocket EngineBurn" );

    if( m_state == StateReady ||
        m_state == StateCountdown || 
        m_state == StateFlight )
    {
        if( numEngineInstances == 0 )
        {
            g_app->m_soundSystem->TriggerBuildingEvent( this, "EngineBurn" );
        }
    }
    else
    {
        if( numEngineInstances > 0 )
        {
            g_app->m_soundSystem->StopAllSounds( m_id, "EscapeRocket EngineBurn" );
        }
    }
}


void EscapeRocket::Initialise( Building *_template )
{
    FuelBuilding::Initialise( _template );

    m_fuel              = ((EscapeRocket *) _template)->m_fuel;
    m_passengers        = ((EscapeRocket *) _template)->m_passengers;
    m_spawnBuildingId   = ((EscapeRocket *) _template)->m_spawnBuildingId;
    m_spawnCompleted    = ((EscapeRocket *) _template)->m_spawnCompleted;   

    if( g_app->Multiplayer() )
    {
        m_coloured = true;
        if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
        {
            //Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
            int colourId = g_app->m_multiwinia->m_teams[m_id.GetTeamId()].m_colourId;
            RGBAColour colour = g_app->m_multiwinia->GetColour(colourId);

            ConvertFragmentColours( m_shape->m_rootFragment, colour );
            m_shape->BuildDisplayList();
        }
    }
}


void EscapeRocket::GetObjectiveCounter( UnicodeString& _dest)
{
    static wchar_t buffer[256];
	swprintf( buffer, sizeof(buffer)/sizeof(wchar_t), L"%ls %d%%, %ls %d%%", LANGUAGEPHRASE("objective_fuel").m_unicodestring,
                                         (int) m_fuel, 
                                         LANGUAGEPHRASE("objective_passengers").m_unicodestring,
                                         (int) m_passengers );
    _dest = UnicodeString( buffer );
}


bool EscapeRocket::BoardRocket( WorldObjectId _id )
{
    if( m_state == StateLoading )
    {
        ++m_passengers;
        return true;
    }

    return false;
}


void EscapeRocket::ProvideFuel( double _level )
{
#ifdef CHEATMENU_ENABLED
    if( !g_app->Multiplayer() )
    {
        if( g_inputManager->controlEvent( ControlScrollSpeedup ) )
        {
            _level *= 100;
        }
    }
#endif

    FuelBuilding::ProvideFuel( _level );

    if( _level > 0.1 )
    {
        ++m_pipeCount;
    }
}


void EscapeRocket::Refuel()
{
    if( g_app->Multiplayer() )
    {
        double newRefuelRate = m_pipeCount * m_currentLevel * 0.4;
        m_refuelRate = m_refuelRate * 0.99 + newRefuelRate * 0.01;

        m_fuel += m_refuelRate * SERVER_ADVANCE_PERIOD;
        if( m_fuel > 100 ) m_fuel = 100;
        m_pipeCount = 0;
        return;
    }

    switch( m_pipeCount )
    {
        case 0:                                                     // No incoming fuel
        case 1:                                                     // 1 pipe providing fuel
        {
            double targetFuel = 50.0;
            if( m_fuel < targetFuel )
            {
                double factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.005;
                double factor2 = 1.0 - factor1;
                m_fuel = m_fuel * factor2 + targetFuel * factor1;            
            }
            break;
        }

        case 2:                                                     // 2 pipes providing fuel
        {
            double targetFuel = 100.0;
            double factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.01;
            double factor2 = 1.0 - factor1;
            m_fuel = m_fuel * factor2 + targetFuel * factor1;
            m_fuel = min( m_fuel, 100.0 );
            break;
        }

        case 3:                                                     // 3 pipes providing fuel
        {
            double targetFuel = 100.0;
            double factor1 = m_currentLevel * SERVER_ADVANCE_PERIOD * 0.1;
            double factor2 = 1.0 - factor1;
            m_fuel = m_fuel * factor2 + targetFuel * factor1;
            m_fuel = min( m_fuel, 100.0 );
            break;
        }
    };

    m_pipeCount = 0;
}


void EscapeRocket::AdvanceRefueling()
{
    if( g_app->Multiplayer() ) SetupAttackers();

    Refuel();

    if( !g_app->Multiplayer() ) m_damage = 0.0f;

    if( m_fuel > 33.0f )
    {
        g_app->m_multiwiniaHelp->NotifyFueledUp();
    }

    if( m_fuel >= 95.0f )
    {
        m_state = StateLoading;
        if( !m_refueledWarning  && !g_app->IsSinglePlayer())
        {
            m_refueledWarning = true;
            UnicodeString msg;
            if( g_app->m_globalWorld->m_myTeamId == m_id.GetTeamId() )
            {
                msg = LANGUAGEPHRASE("multiwinia_rr_refueled");
            }
			else
            {
                msg = LANGUAGEPHRASE("multiwinia_rr_otherrefueled");
                msg.ReplaceStringFlag( L'T', g_app->m_location->m_teams[ m_id.GetTeamId() ]->GetTeamName() );
            }

            g_app->m_location->SetCurrentMessage( msg );
        }
    }
}


void EscapeRocket::AdvanceLoading()
{
    if( g_app->Multiplayer() ) SetupAttackers();

    Refuel();

    if( m_fuel > 95.0f && m_passengers >= 100 )
    {
        m_state = StateIgnition;
        m_countdown = 18.0f;

        if( !m_countdownWarning  && !g_app->IsSinglePlayer())
        {
            m_countdownWarning = true;
            UnicodeString msg;
            if( g_app->m_globalWorld->m_myTeamId == m_id.GetTeamId() )
            {
                msg = LANGUAGEPHRASE("multiwinia_rr_launchprep");
            }
            else
            {
                msg = LANGUAGEPHRASE("multiwinia_rr_launchprepother");
                msg.ReplaceStringFlag( L'T', g_app->m_location->m_teams[ m_id.GetTeamId() ]->GetTeamName() );
            }
        
            g_app->m_location->SetCurrentMessage( msg );
        }
    }
}


void EscapeRocket::AdvanceIgnition()
{
    m_countdown -= SERVER_ADVANCE_PERIOD;
   
    if( g_app->Multiplayer() ) m_countdown = 0;

    SetupSpectacle();

    if( m_countdown <= 0.0f )
    {        
        m_state = StateReady;
        m_countdown = 120.0f;

        m_cameraShake = 5.0f;

        if( g_app->Multiplayer() )
        {
            m_countdown = 10.0f;
        }
        else
        {
            if( m_spawnCompleted )
            {
                m_countdown = 10.0f;
                g_app->m_script->RunScript( "launchpad_victory.txt" );
            }
        }
    }    
}


void EscapeRocket::AdvanceReady()
{
    //
    // Spawn attacking Darwinians

    if( !m_spawnCompleted )
    {
		Team* myTeam = g_app->m_location->GetMyTeam();
		SpawnPoint* spawnpoint = NULL;
		bool gotSpawnPoints = false;

		spawnpoint = (SpawnPoint*)g_app->m_location->GetBuilding(18);
		if( myTeam && spawnpoint && spawnpoint->m_type == Building::TypeSpawnPoint ) gotSpawnPoints |= spawnpoint->m_id.GetTeamId() == 0;

		spawnpoint = (SpawnPoint*)g_app->m_location->GetBuilding(51);
		if( myTeam && spawnpoint && spawnpoint->m_type == Building::TypeSpawnPoint ) gotSpawnPoints |= spawnpoint->m_id.GetTeamId() == 0;

		spawnpoint = (SpawnPoint*)g_app->m_location->GetBuilding(52);
		if( myTeam && spawnpoint && spawnpoint->m_type == Building::TypeSpawnPoint ) gotSpawnPoints |= spawnpoint->m_id.GetTeamId() == 0;

		spawnpoint = (SpawnPoint*)g_app->m_location->GetBuilding(54);
		if( myTeam && spawnpoint && spawnpoint->m_type == Building::TypeSpawnPoint ) gotSpawnPoints |= spawnpoint->m_id.GetTeamId() == 0;

        if( m_countdown > 95.0f && gotSpawnPoints )//m_countdown < 110.0f )
        {
            Building *spawnBuilding = g_app->m_location->GetBuilding( m_spawnBuildingId );
            if( spawnBuilding )
            {
                Vector3 spawnPos = spawnBuilding->m_pos + spawnBuilding->m_front * 40.0f;
                g_app->m_location->SpawnEntities( spawnPos, 1, -1, Entity::TypeDarwinian, 2, g_zeroVector, 40.0f );
            }
        }    
    }

    SetupAttackers();
    SetupSpectacle();
    
    m_countdown -= SERVER_ADVANCE_PERIOD;

    if( m_countdown <= 0.0f )
    {
        m_state = StateCountdown;
        m_countdown = 10.0f;
    }
}


void EscapeRocket::AdvanceCountdown()
{
    m_countdown -= SERVER_ADVANCE_PERIOD * 0.5;
    m_countdown = max( m_countdown, 0.0 );

    SetupAttackers();
    SetupSpectacle();

    if( m_countdown == 0.0 )
    {
        m_state = StateFlight;

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        //if( gb ) gb->m_online = true;
    }
}


void EscapeRocket::AdvanceFlight()
{
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    double thrust = iv_sqrt(m_pos.y - landHeight) * 2;
    thrust = max( thrust, 0.1 );

    m_vel.Set( 0, thrust, 0 );

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    m_centrePos += m_vel * SERVER_ADVANCE_PERIOD;

    SetupSpectacle();
}


void EscapeRocket::AdvanceExploding()
{
    m_damage -= SERVER_ADVANCE_PERIOD * 1;

    //
    // Burn fuel

    double minFuel = 0.0;
    double amountToLose = SERVER_ADVANCE_PERIOD * 10.0;

    if( g_app->Multiplayer() )
    {
        minFuel = m_fuelBeforeExplosion * 0.5;
        amountToLose = SERVER_ADVANCE_PERIOD * 5;
    }

    m_fuel -= amountToLose;
    m_fuel = max( m_fuel, minFuel );
    

    //
    // Kill passengers

    if( m_passengers )
    {
        m_passengers--;
        
        int windowIndex = syncrand() % 3;
        Matrix34 mat( m_front, m_up, m_pos );
        Matrix34 windowMat = m_window[windowIndex]->GetWorldMatrix(mat);

        Vector3 vel = windowMat.f;
        double angle = syncsfrand( M_PI * 0.25 );
        vel.RotateAround( windowMat.u * angle );
        vel.SetLength( 10.0 + syncfrand(30.0) );

        int teamId = 0;
        bool onFire = true;

        if( g_app->Multiplayer() )
        {
            teamId = m_id.GetTeamId();
        }

        WorldObjectId id = g_app->m_location->SpawnEntities( windowMat.pos, teamId, -1, Entity::TypeDarwinian, 1, vel, 0.0 );
        Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntity( id );
        darwinian->m_onGround = false;
        
        if( g_app->Multiplayer() ) onFire = ( syncfrand(1.0) < 0.5 );

        if( onFire ) darwinian->SetFire();
    }
    
    
    if( m_fuel > 0.0 )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, 0.01 );
    }


    //
    // Spawn smoke and fire

    for( int i = 0; i < 3; ++i )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        Matrix34 windowMat = m_window[i]->GetWorldMatrix(mat);

        Vector3 vel = windowMat.f;
        double angle = syncsfrand( M_PI * 0.25 );
        vel.RotateAround( windowMat.u * angle );
        vel.SetLength( 5.0 + syncfrand(10.0) );        
        double fireSize = 150 + syncfrand(150.0);

        Vector3 smokeVel = vel;
        double smokeSize = fireSize;

        if( m_fuel > 0.0 ) g_app->m_particleSystem->CreateParticle( windowMat.pos, vel, Particle::TypeFire, fireSize );
        g_app->m_particleSystem->CreateParticle( windowMat.pos, smokeVel, Particle::TypeMissileTrail, smokeSize );
    }


    if( g_app->Multiplayer() &&
        m_passengers == 0 &&
        m_fuel <= minFuel )
    {
        m_damage -= SERVER_ADVANCE_PERIOD * 2;        
    }


    if( m_damage <= 0.0 )
    {
        m_damage = 0.0;
        m_currentLevel = 0;
        m_spawnCompleted = true;
        m_state = StateRefueling;
    }
}


void EscapeRocket::SetupSpectacle()
{
    if( !IsSpectacle() ) return;


    m_shadowTimer -= SERVER_ADVANCE_PERIOD;
    if( m_shadowTimer <= 0.0f )
    {
        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            Team *team = g_app->m_location->m_teams[t];
            for( int i = 0; i < team->m_others.Size(); ++i )
            {
                if( team->m_others.ValidIndex(i) )
                {
                    Entity *entity = team->m_others[i];
                    if( entity && entity->m_type == Entity::TypeDarwinian )
                    {
                        Darwinian *darwinian = (Darwinian *) entity;

                        bool watchSpectacle = false;
                        bool castShadow = false;

                        if( g_app->Multiplayer() )
                        {
                            watchSpectacle = true;
                            castShadow = true;
                        }
                        else
                        {
                            watchSpectacle = ( t == 0 &&
                                               darwinian->m_state == Darwinian::StateIdle &&
                                               (syncrand() % 10) < 2 );
                        }

                        if( watchSpectacle )
                        {
                            darwinian->WatchSpectacle( m_id.GetUniqueId() );
                            if( castShadow ) darwinian->CastShadow( m_id.GetUniqueId() );
                        }
                    }
                }
            }
        }

        m_shadowTimer = 1.0f;
    }
}


bool EscapeRocket::IsSpectacle()
{
    if( g_app->Multiplayer() )
    {
        return false;
    }
    else
    {
        return( m_state == StateIgnition ||
                m_state == StateReady ||
                m_state == StateCountdown ||
                m_state == StateFlight );
    }
}


bool EscapeRocket::IsInView()
{
    return FuelBuilding::IsInView() || m_state == StateFlight;
}


void EscapeRocket::SetupAttackers()
{
    if( g_app->Multiplayer() )
    {
        if( m_state < StateExploding && m_fuel > 33.0f ) 
        {
            m_attackTimer -= SERVER_ADVANCE_PERIOD;
            if( m_attackTimer <= 0.0f )
            {
                m_attackTimer = 1.0f + syncfrand(2.0f);

                int numFound = 0;
                double range = 300;
                if( g_app->Multiplayer() ) range = 150;
                g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, range, &numFound, m_id.GetTeamId() );

                if( numFound > 0 )
                {
                    int numAttackers = 1 + syncrand() % (1 + int(numFound * 0.05));                
                    for( int i = 0; i < numAttackers; ++i )
                    {
                        int chosenIndex = syncrand() % numFound;
                        WorldObjectId attackerId = s_neighbours[chosenIndex];
                        Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( attackerId, Entity::TypeDarwinian );
                        if( darwinian &&
                            !darwinian->m_dead &&
                            !darwinian->IsOnFire() ) 
                        {
                            darwinian->AttackBuilding( m_id.GetUniqueId() );
                        }
                    }
                }
            }
        }
    }
    else
    {
        if( !m_spawnCompleted && syncfrand(1) < 0.2 )
        {
			for( int i = 0; i < NUM_TEAMS; ++i )
            {
                //if( i != m_id.GetTeamId() &&
                //    g_app->m_location->m_teams[i]->m_teamType != TeamTypeUnused )
				if (i == m_id.GetTeamId())
                {
                    Team *team = g_app->m_location->m_teams[i];
                    int numOthers = team->m_others.Size();
                    if( numOthers > 0 )
                    {
                        int randomIndex = syncrand() % numOthers;
                        if( team->m_others.ValidIndex(randomIndex) )
                        {
                            Entity *entity = team->m_others[randomIndex];
                            if( entity && entity->m_type == Entity::TypeDarwinian )
                            {
                                Darwinian *darwinian = (Darwinian *) entity;
                                double range = ( darwinian->m_pos - m_pos ).Mag();
                                if( range < 350.0 )
                                {
                                    darwinian->AttackBuilding( m_id.GetUniqueId() );
                                }
                            }
                        }
                    }
                }
			}
        }
    }
}


void EscapeRocket::Damage( double _damage )
{
    FuelBuilding::Damage( _damage );

    if( m_state != StateExploding )
    {
        if( g_app->Multiplayer() )
        {
            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                if( !m_attackWarning )
                {
                    m_attackWarning = true;
                    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE( "dialog_rocketunderattack" ) );
                }
            }
            m_damage -= _damage * 0.2;
        }
        else
        {
            m_damage -= _damage;
        }

        if( m_damage > 100.0f )
        {
            m_fuelBeforeExplosion = m_fuel;
            m_state = StateExploding;
            g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );

            if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_rocketdestroyed") );
            }
        }
    }
}


bool EscapeRocket::Advance()
{    
    if( g_app->Multiplayer() )
    {
        if( m_damage > 0 )
        {
            m_damage -= SERVER_ADVANCE_PERIOD * 0.33;
            if( m_damage <= 0 ) 
            {
                m_damage = 0;
                m_attackWarning = false;
            }
        }
    }

    switch( m_state )
    {
        case StateRefueling:            AdvanceRefueling();             break;
        case StateLoading:              AdvanceLoading();               break;
        case StateIgnition:             AdvanceIgnition();              break;
        case StateReady:                AdvanceReady();                 break;
        case StateCountdown:            AdvanceCountdown();             break;
        case StateFlight:               AdvanceFlight();                break; 
        case StateExploding:            AdvanceExploding();             break;
    }
    

    SetupSounds();

    
    //
    // Create rocket flames
    // Shake the camera

    if( m_cameraShake > 0.0 )
    {
        m_cameraShake -= SERVER_ADVANCE_PERIOD;

        double actualShake = m_cameraShake/5.0;
        g_app->m_camera->CreateCameraShake( actualShake );
    }


    if( m_state == StateReady ||
        m_state == StateCountdown || 
        m_state == StateFlight )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;

        for( int i = 0; i < 15; ++i )
        {
            Vector3 pos = boosterPos;
            pos += Vector3( sfrand(20), 10, sfrand(20) );
            
            Vector3 vel( sfrand(50), -frand(150), sfrand(50) );
            double size = 500.0;

            if( i > 10 )
            {
                vel.x *= 0.75;
                vel.z *= 0.75;
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileTrail, size );
            }
            else
            {
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
            }
        }
    }

    return FuelBuilding::Advance();
}


bool EscapeRocket::SafeToLaunch()
{
    if( g_app->Multiplayer() )
    {
        return true;
    }
    else
    {
        Vector3 testPos = m_pos;// + Vector3(330,0,50);
        double testRadius = 200.0;

        int myTeam = 0;

        int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( testPos.x, testPos.z, testRadius, myTeam );

        return( numEnemies < 2 );
    }
}


void EscapeRocket::Render( double _predictionTime )
{    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    
    Matrix34 mat( m_front, m_up, predictedPos );

	//m_environment.RenderInit(m_pos); // start reflecting environment
    m_shape->Render( 0.0f, mat );   
	//m_environment.RenderDone();
}


void EscapeRocket::RenderAlphas( double _predictionTime )
{
    FuelBuilding::RenderAlphas( _predictionTime );

    if( g_app->m_editing )
    {
        Building *spawnBuilding = g_app->m_location->GetBuilding( m_spawnBuildingId );
        if( spawnBuilding )
        {
            RenderArrow( m_pos, spawnBuilding->m_pos, 1.0f );
        }

        RenderSphere( m_pos, m_radius, RGBAColour(255,255,255,255) );    

        return;
    }

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,120,0), 10, "Fuel : %2.2f", m_fuel );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,130,0), 10, "Passengers : %d", m_passengers );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,140,0), 10, "Damage : %2.2f", m_damage );
//    g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,150,0), 10, "Timer : %2.2f", m_countdown );

//    if( m_state == StateCountdown && m_countdown <= 10.0f )
//    {
//        g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,200,0), 100, "%d", int(m_countdown) );
//    }



    //
    // Render rocket glow
    // Calculate alpha value

    float alpha = m_fuel / 100.0f;
    alpha = min( alpha, 1.0f );
    alpha = max( alpha, 0.0f );

    if( alpha > 0.0f )
    {
        Vector3 camUp = g_app->m_camera->GetUp();
        Vector3 camRight = g_app->m_camera->GetRight() * 0.75f;
        
        glDepthMask     ( false );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        //glDisable       ( GL_DEPTH_TEST );

        float timeIndex = g_gameTime * 2;

        int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
        int maxBlobs = 50;
        if( buildingDetail == 2 ) maxBlobs = 20;
        if( buildingDetail == 3 ) maxBlobs = 10;

        Vector3 boosterPos = predictedPos;
        boosterPos.y += 100;

    
        //
        // Central glow effect

        for( int i = 30; i < maxBlobs; ++i )
        {        
            Vector3 pos = boosterPos;
            pos.x += sinf(timeIndex*0.5+i) * i * 3.7f;
            pos.y += cosf(timeIndex*0.5+i) * cosf(i*20) * 50;
            pos.y += 40;
            pos.z += cosf(timeIndex*0.3+i) * i * 3.7f;

            float size = 20.0f * sinf(timeIndex+i*2);
            size = max( size, 5.0f );
        
            for( int j = 0; j < 2; ++j )
            {
                size *= 0.75f;
                glColor4f( 1.0f, 0.6f, 0.2f, alpha);        
                glBegin( GL_QUADS );
                    glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
                    glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
                    glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
                    glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
                glEnd();
            }
        }

    
        //
        // Central starbursts

        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

        int numStars = 10;
        if( buildingDetail == 2 ) numStars = 5;
        if( buildingDetail == 3 ) numStars = 2;
    
        for( int i = 8; i < numStars; ++i )
        {
            Vector3 pos = boosterPos;
            pos.x += sinf(timeIndex+i) * i * 1.7f;
            pos.y += fabs(cosf(timeIndex+i) * cosf(i*20) * 64);
            pos.z += cosf(timeIndex+i) * i * 1.7f;

            float size = i * 30.0f;
        
            glColor4f( 1.0f, 0.4f, 0.2f, alpha );
            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
            glEnd();
        }

        glEnable( GL_DEPTH_TEST );
        glDisable( GL_TEXTURE_2D );    

    }

}


void EscapeRocket::Read( TextReader *_in, bool _dynamic )
{
    FuelBuilding::Read( _in, _dynamic );

    m_fuel              = iv_atof( _in->GetNextToken() );
    m_passengers        = atoi( _in->GetNextToken() );
    m_spawnBuildingId   = atoi( _in->GetNextToken() );
    m_spawnCompleted    = atoi( _in->GetNextToken() );
}


void EscapeRocket::Write( TextWriter *_out )
{
    FuelBuilding::Write( _out );

    _out->printf( "%6.2f %6d %6d %6d", m_fuel, m_passengers, m_spawnBuildingId, (int) m_spawnCompleted );
}


int EscapeRocket::GetStateId( char *_state )
{
    static char *stateNames[] = {       
                                    "Refueling",
                                    "Loading",
                                    "Ignition",
                                    "Ready",
                                    "Countdown",
                                    "Exploding",
                                    "Flight"
                                };
    
    for( int i = 0; i < NumStates; ++i )
    {
        if( stricmp( stateNames[i], _state ) == 0 )
        {
            return i;
        }
    }

    return -1;
}


bool EscapeRocket::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool EscapeRocket::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool EscapeRocket::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                              double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( g_app->m_editing )
    {
        return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
    }

    return false;
}


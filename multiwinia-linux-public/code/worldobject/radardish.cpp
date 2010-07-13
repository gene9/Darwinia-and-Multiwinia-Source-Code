#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/ogl_extensions.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/math/random_number.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "location_input.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "global_world.h"
#include "unit.h"
#include "level_file.h"

#include "sound/soundsystem.h"

#include "worldobject/radardish.h"
#include "worldobject/ai.h"
#include "worldobject/darwinian.h"
#include "worldobject/rocket.h"
#include "worldobject/insertion_squad.h"



RadarDish::RadarDish()
:   Teleport(),
    m_signal(0.0),
    m_range(0.0),
    m_receiverId(-1),
    m_horizontallyAligned(true),
    m_verticallyAligned(true),
    m_movementSoundsPlaying(false),
    m_newlyCreated(true),
	m_aiTarget(NULL),
    m_forceConnection(0),
    m_autoConnectAtStart(0),
    m_oldTeamId(-1),
    m_forceTeamMatch(false)
{
    m_type = Building::TypeRadarDish;
    m_target.Zero();
    m_front.Set(0,0,1);    
    m_sendPeriod = RADARDISH_TRANSPORTPERIOD;

	Shape *radarShape = g_app->m_resource->GetShapeCopy("radardish.shp", true);
	SetShape( radarShape );
    
    const char dishName[] = "Dish";
	m_dish = m_shape->m_rootFragment->LookupFragment( dishName );
    AppReleaseAssert( m_dish, "RadarDish: Can't get Fragment(%s) from shape(%s), probably a corrupted file\n", dishName, m_shape->m_name );

    const char upperMountName[] = "UpperMount";
	m_upperMount = m_shape->m_rootFragment->LookupFragment( upperMountName );
    AppReleaseAssert( m_upperMount, "RadarDish: Can't get Fragment(%s) from shape(%s), probably a corrupted file\n", upperMountName, m_shape->m_name );

    const char focusMarkerName[] = "MarkerFocus";
	m_focusMarker = m_shape->m_rootFragment->LookupMarker( focusMarkerName );
    AppReleaseAssert( m_focusMarker, "RadarDish: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", focusMarkerName, m_shape->m_name );
}


RadarDish::~RadarDish()
{
	delete m_shape;
	m_shape = NULL;

	if( m_aiTarget )
    {
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.EmptyAndDelete();
        m_aiTarget = NULL;
    }
}

void RadarDish::Initialise( Building *_template )
{
    Building::Initialise( _template );
    RadarDish *r = (RadarDish *)_template;
    for( int i = 0; i < r->m_validLinkList.Size(); ++i )
    {
        SetBuildingLink(r->m_validLinkList[i]);
    }
    m_forceConnection = r->m_forceConnection;
    m_autoConnectAtStart = r->m_autoConnectAtStart;

    if( g_app->m_location->m_levelFile->m_levelOptions[ LevelFile::LevelOptionForceRadarTeamMatch] == 1 )
        m_forceTeamMatch = true;
}

void RadarDish::SetDetail( int _detail )
{
    Teleport::SetDetail( _detail );

    Matrix34 rootMat(m_front, m_up, m_pos);
    Matrix34 worldMat = m_entrance->GetWorldMatrix(rootMat);
    m_entrancePos = worldMat.pos;
    m_entranceFront = worldMat.f;    
}


bool RadarDish::GetEntrance( Vector3 &_pos, Vector3 &_front )
{
    _pos = m_entrancePos;
    _front = m_entranceFront;
    return true;
}


bool RadarDish::Advance ()
{ 
    //
    // If this is our first advance, align to our dish
    // if we saved a target dish previously
    if( m_newlyCreated )
    {
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb )
        {
            Building *targetBuilding = g_app->m_location->GetBuilding( gb->m_link );
            if( targetBuilding && targetBuilding->m_type == TypeRadarDish )
            {
                RadarDish *dish = (RadarDish *) targetBuilding;
                Aim( dish->GetStartPoint() );
            }
        }
        else if( m_validLinkList.Size() > 0 && m_autoConnectAtStart )
        {
            RadarDish *r = (RadarDish *)g_app->m_location->GetBuilding( m_validLinkList[0] );
            if( r && r->m_type == Building::TypeRadarDish )
            {
                Aim( r->GetStartPoint() );
            }
        }
        m_newlyCreated = false;
    }

    if( g_app->Multiplayer() )
    {
        if( m_id.GetTeamId() != m_oldTeamId )
        {
            if( m_id.GetTeamId() != 255 )
            {
                Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
                ConvertFragmentColours( m_shape->m_rootFragment, team->m_colour );
            }
            else
            {
                ConvertFragmentColours( m_shape->m_rootFragment, RGBAColour( 255,255,255 ) );
            }
            m_oldTeamId = m_id.GetTeamId();
        }
    }

	if (m_target.MagSquared() < 0.001)		return Building::Advance();

	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
	Vector3 dishPos = worldMat.pos;
    Vector3 dishFront = worldMat.f;

    if( m_movementSoundsPlaying && m_horizontallyAligned && m_dish->m_angVel.Mag() < 0.05 )
    {     
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish BeginRotation" );
        m_movementSoundsPlaying = false;
        
    }


	//
    // Rotate slowly to face our target (horiz)

    if( !m_horizontallyAligned )
    {
		Vector3 targetFront = ( m_target - dishPos );
		Vector3 currentFront = worldMat.f;
		targetFront.HorizontalAndNormalise();
		currentFront.HorizontalAndNormalise();
		Vector3 rotationAxis = currentFront ^ targetFront;
		rotationAxis.y /= 4.0;
		if (fabsf(rotationAxis.y) > M_PI / 3000.0)
		{
			m_upperMount->m_angVel.y = signf(rotationAxis.y) * iv_sqrt(fabsf(rotationAxis.y));
		}
		else
		{
            m_horizontallyAligned = true;
			m_upperMount->m_angVel.y = 0.0;
		}
	}

  
    //
    // Rotate slowly to face our target (vert)

	m_dish->m_angVel.Zero();

    if( m_horizontallyAligned && !m_verticallyAligned )
    {
        Vector3 targetFront = ( m_target - dishPos );
        targetFront.Normalise();
		double const maxAngle = 0.3;
		double const minAngle = -0.2;
		if (targetFront.y > maxAngle) targetFront.y = maxAngle;
		if (targetFront.y < minAngle) targetFront.y = minAngle;
        targetFront.Normalise();
		double amount = worldMat.u * targetFront;
		m_dish->m_angVel = m_dish->m_transform.r * amount;

        m_verticallyAligned = ( m_dish->m_angVel.Mag() < 0.001 );
    }

	m_upperMount->m_transform.RotateAround(m_upperMount->m_angVel * SERVER_ADVANCE_PERIOD);
	m_dish->m_transform.RotateAround(m_dish->m_angVel * SERVER_ADVANCE_PERIOD);

    if( m_movementSoundsPlaying && m_horizontallyAligned && m_dish->m_angVel.Mag() < 0.05 )
    {     
        g_app->m_soundSystem->TriggerBuildingEvent( this, "EndRotation" );
    }


    //
    // Are there any receiving Radar dishes?

    m_range = 99999.9;
    bool found = false;
    
    bool previouslyAligned = ( m_receiverId != -1 );

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
		// Skip empty slots
        if( !g_app->m_location->m_buildings.ValidIndex(i) ) continue;
		
		// Filter out non radar dish buildings
		Building *building = g_app->m_location->m_buildings.GetData(i);
        if( building->m_type != TypeRadarDish ) continue;

		// Don't compare against ourself
		if( building == this ) continue;

		// Does our "ray" hit their dish
        RadarDish *otherDish = (RadarDish *) building;
        bool hit = RaySphereIntersection( dishPos, dishFront, otherDish->m_centrePos, otherDish->m_radius, 1e9 );
        if( hit )
		{
			// Make sure we aren't hitting the back of the receiving dish
	        Vector3 theirFront = otherDish->GetDishFront(0.0);
			double dotProd = dishFront * theirFront;
			if (dotProd < 0.0)
			{
				if (g_app->m_location->IsVisible( dishPos, otherDish->GetDishPos(0.0)))
				{
					double newRange = (otherDish->GetDishPos(0.0) - dishPos).Mag();
					if( newRange < m_range )
					{
                        if( ValidReceiverDish( otherDish->m_id.GetUniqueId() ) )
                        {
						    m_receiverId = otherDish->m_id.GetUniqueId();
						    m_range = newRange;
						    m_signal = 1.0;
						    found = true;
                        }
					}
				}
			}
		}
    }

    if( previouslyAligned && !found )
    {
        m_range = 0.0;
        m_signal = 0.0;
        m_receiverId = -1;
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish ConnectionEstablished" );
        g_app->m_soundSystem->TriggerBuildingEvent( this, "ConnectionLost" );

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_link = -1;    
    }

    if( !previouslyAligned && found )
    {
        g_app->m_soundSystem->TriggerBuildingEvent( this, "ConnectionEstablished" );

        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_link = m_receiverId;
    }

	if( g_app->Multiplayer() ) AdvanceAITarget();
      
    return Teleport::Advance();
}


Vector3 RadarDish::GetDishPos( double _predictionTime )
{
	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
    return worldMat.pos;
}


Vector3 RadarDish::GetDishFront( double _predictionTime )
{
    if( m_receiverId != -1 )
    {
        RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
        if( receiver )
        {
            Vector3 ourDishPos = GetDishPos( _predictionTime );
            Vector3 receiverDishPos = receiver->GetDishPos( _predictionTime );
            return ( receiverDishPos - ourDishPos ).Normalise();
        }
    }
    
	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
    return worldMat.f;
}


void RadarDish::Aim( Vector3 _worldPos )
{
	AppDebugOut("Aiming radar dish at %f,%f,%f\n", _worldPos.x, _worldPos.y, _worldPos.z);
    if( m_validLinkList.Size() > 0 )   
    {
        bool found = false;

        Matrix34 rootMat(m_front, g_upVector, m_pos);
        Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
	    Vector3 dishPos = worldMat.pos;       

        Vector3 direction = (_worldPos - dishPos).Normalise();

        for( int i = 0; i < m_validLinkList.Size(); ++i )
        {
            if( !m_validLinkList.ValidIndex(i) ) continue;
            Building *building = g_app->m_location->GetBuilding( m_validLinkList[i] );
            if( building->m_type != TypeRadarDish ) continue;
		    if( building == this ) continue;

		    // Does our "ray" hit their dish
            RadarDish *otherDish = (RadarDish *) building;
            bool hit = RaySphereIntersection( dishPos, direction, otherDish->m_centrePos, otherDish->m_radius, 1e9 );
            if( hit )
		    {
			    if (g_app->m_location->IsVisible( dishPos, otherDish->GetDishPos(0.0)))
			    {
                    found = true;				    
			    }
		    }
        }
        if( !found && m_forceConnection) return;
    }

    m_target = _worldPos;
    
    m_horizontallyAligned = false;
    m_verticallyAligned = false;

    if( m_movementSoundsPlaying )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "RadarDish BeginRotation" );    
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "BeginRotation" );
    m_movementSoundsPlaying = true;
}


void RadarDish::Render( double _predictionTime )
{
	Building::Render(_predictionTime);   
}


#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#endif

void RadarDish::RenderAlphas ( double _predictionTime )
{
#ifdef USE_DIRECT3D
	// sets default vertex format (somebody changes it and not returns back, where?)
	// without this code, radar beams are not reflected
	extern LPDIRECT3DVERTEXDECLARATION9 s_pCustomVertexDecl;
	OpenGLD3D::g_pd3dDevice->SetVertexDeclaration( s_pCustomVertexDecl );
#endif
    if( m_signal > 0.0 )
    {
        RenderSignal( _predictionTime, 10.0, 0.4 );              
        RenderSignal( _predictionTime, 9.0, 0.2 );   
        RenderSignal( _predictionTime, 8.0, 0.2 );   
        RenderSignal( _predictionTime, 4.0, 0.5 );   
    }
    
    Teleport::RenderAlphas(_predictionTime);

    bool renderLinks = false;
    if( g_app->m_editing )
    {
        renderLinks = true;
    }

    if( g_app->m_location->GetMyTeam() ) 
    {
        if( g_app->m_location->GetMyTeam()->m_currentBuildingId == m_id.GetUniqueId() )
        {
            renderLinks = true;
        }
        else
        {
            WorldObjectId id;
            g_app->m_locationInput->GetObjectUnderMouse(id, g_app->m_location->GetMyTeam()->m_teamId );
            if( id.GetUnitId() == UNIT_BUILDINGS && id.GetUniqueId() == m_id.GetUniqueId() )
            {
                renderLinks = true;
            }
        }
    }
    
    int currentReceiver = m_receiverId;
    double range = m_range;
    if( renderLinks )
    {
        for( int i = 0; i < m_validLinkList.Size(); ++i )
        {
            if( m_validLinkList.ValidIndex(i) && m_validLinkList[i] != m_receiverId )
            {
                RadarDish *r = (RadarDish *)g_app->m_location->GetBuilding( m_validLinkList[i] );
                if( r )
                {
                    if( g_app->m_editing )
                    {
                        RenderArrow( GetDishPos(_predictionTime), r->GetDishPos(_predictionTime), 1.0 );
                    }
                    else
                    {
                        m_range = (r->m_pos - m_pos).Mag();
                        m_receiverId = r->m_id.GetUniqueId();
                        RenderSignal( _predictionTime, 10.0, 0.3 );  
                    }
                        
                }
            }
        }
    }
    m_range = range;
    m_receiverId = currentReceiver;
}


void RadarDish::RenderSignal( double _predictionTime, double _radius, double _alpha )
{
	START_PROFILE( "Signal");

    Vector3 startPos = GetStartPoint();
    Vector3 endPos = GetEndPoint();
    Vector3 delta = endPos - startPos;
    Vector3 deltaNorm = delta;
    deltaNorm.Normalise();

    double distance = (startPos - endPos).Mag();
    double numRadii = 20.0;
    int numSteps = int( distance / 200.0 );
    numSteps = max( 1, numSteps );

    glEnable            (GL_TEXTURE_2D);

    gglActiveTextureARB  (GL_TEXTURE0_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laserfence.bmp", true, true));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);        
    glEnable            (GL_TEXTURE_2D);

    gglActiveTextureARB  (GL_TEXTURE1_ARB);
    glBindTexture	    (GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/radarsignal.bmp", true, true));
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE); 
    glEnable            (GL_TEXTURE_2D);

    glDisable           (GL_CULL_FACE);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE);
    glEnable            (GL_BLEND);
    glDepthMask         (false);
    glColor4f           (1.0,1.0,1.0,_alpha);            
    
    glMatrixMode        (GL_MODELVIEW);
    glTranslatef        ( startPos.x, startPos.y, startPos.z );    
    Vector3 dishFront   = GetDishFront(_predictionTime);
    double eqn1[4]      = { dishFront.x, dishFront.y, dishFront.z, -1.0 };
    glClipPlane         (GL_CLIP_PLANE0, eqn1 );

    RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
    Vector3 receiverPos = receiver->GetDishPos( _predictionTime );
    Vector3 receiverFront = receiver->GetDishFront( _predictionTime );
    glTranslatef        ( -startPos.x, -startPos.y, -startPos.z );
    glTranslatef        ( receiverPos.x, receiverPos.y, receiverPos.z );
    
    double eqn2[4]      = { receiverFront.x, receiverFront.y, receiverFront.z, -1.0 };
    glClipPlane         (GL_CLIP_PLANE1, eqn2 );
    glTranslatef        ( -receiverPos.x, -receiverPos.y, -receiverPos.z );
    glTranslatef        ( startPos.x, startPos.y, startPos.z );    

    glEnable            (GL_CLIP_PLANE0);
    glEnable            (GL_CLIP_PLANE1);

    double texXInner = -g_gameTime/_radius;
    double texXOuter = -g_gameTime;

    //double texXInner = -1.0/_radius;
    //double texXOuter = -1.0;

    glBegin( GL_QUAD_STRIP );

    for( int s = 0; s < numSteps; ++s )
    {
        Vector3 deltaFrom = 1.2 * delta * (double) s / (double) numSteps;
        Vector3 deltaTo = 1.2 * delta * (double) (s+1) / (double) numSteps;
        
        Vector3 currentPos = (-delta*0.1) + Vector3(0,_radius,0);
        
        for( int r = 0; r <= numRadii; ++r )
        {   
            gglMultiTexCoord2fARB    ( GL_TEXTURE0_ARB, texXInner, r/numRadii );
            gglMultiTexCoord2fARB    ( GL_TEXTURE1_ARB, texXOuter, r/numRadii );        
            glVertex3dv             ( (currentPos + deltaFrom).GetData() );
    
            gglMultiTexCoord2fARB    ( GL_TEXTURE0_ARB, texXInner+10.0/(double)numSteps, (r)/numRadii );
            gglMultiTexCoord2fARB    ( GL_TEXTURE1_ARB, texXOuter+distance/(200.0 *(double)numSteps), (r)/numRadii );
            glVertex3dv             ( (currentPos + deltaTo).GetData() );
    
            currentPos.RotateAround( deltaNorm * ( 2.0 * M_PI / (double) numRadii ) );
        }

        texXInner += 10.0 / (double) numSteps;
        texXOuter += distance/(200.0 * (double) numSteps);
    }

    glEnd();
    
    glTranslatef        ( -startPos.x, -startPos.y, -startPos.z );
    
    glDisable           (GL_CLIP_PLANE0);
    glDisable           (GL_CLIP_PLANE1);
    glDepthMask         (true);
    glDisable           (GL_BLEND);
    glBlendFunc         (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable            (GL_CULL_FACE);

    gglActiveTextureARB  (GL_TEXTURE1_ARB);
    glDisable           (GL_TEXTURE_2D);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    gglActiveTextureARB  (GL_TEXTURE0_ARB);
    glDisable           (GL_TEXTURE_2D);
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
    glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	END_PROFILE( "Signal");
}


bool RadarDish::Connected()
{
    return( m_receiverId != -1 && m_signal > 0.0 );
}


int RadarDish::GetConnectedDishId()
{
    return m_receiverId;
}


bool RadarDish::ReadyToSend ()
{
    return( Connected() && Teleport::ReadyToSend() );
}


Vector3 RadarDish::GetStartPoint()
{
    return GetDishPos(0.0);
}


Vector3 RadarDish::GetEndPoint()
{   
    return GetDishPos(0.0) + GetDishFront(0.0) * m_range;
}


bool RadarDish::GetExit( Vector3 &_pos, Vector3 &_front )
{
    RadarDish *receiver = (RadarDish *) g_app->m_location->GetBuilding( m_receiverId );
    if( receiver )
    {
        Matrix34 rootMat(receiver->m_front, g_upVector, receiver->m_pos);
        Matrix34 worldMat = receiver->m_entrance->GetWorldMatrix(rootMat);
        _pos = worldMat.pos;
        _front = worldMat.f;
        return true;
    }
    return false;
}


bool RadarDish::UpdateEntityInTransit( Entity *_entity )
{
    Vector3 dishPos = GetDishPos(0.0);
    Vector3 dishFront = GetDishFront(0.0);
    Vector3 targetPos = dishPos + dishFront * m_range;
    Vector3 ourOffset = ( targetPos - _entity->m_pos );
    
    WorldObjectId id( _entity->m_id );

    _entity->m_vel = dishFront * RADARDISH_TRANSPORTSPEED;
    _entity->m_pos += _entity->m_vel * SERVER_ADVANCE_PERIOD;
    _entity->m_onGround = false;
    _entity->m_enabled = false;

    if( _entity->m_id.GetUnitId() != -1 )
    {
        Unit *unit = g_app->m_location->GetUnit( _entity->m_id );
        unit->UpdateEntityPosition( _entity->m_pos, _entity->m_radius );
    }

    double distTravelled = ( _entity->m_pos - dishPos ).Mag();
    
    if( m_signal == 0.0 || PulseWave::s_pulseWave )
    {
        // Shit - we lost the carrier signal, so we die
        _entity->ChangeHealth( -500 );
        _entity->m_enabled = true;
        _entity->m_vel += Vector3(SFRAND(10.0), FRAND(10.0), SFRAND(10.0) );
                
        g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );
        return true;
    }
    else if( distTravelled >= m_range )
    {
        // We are there
        Vector3 exitPos, exitFront;
        GetExit( exitPos, exitFront );
        _entity->m_pos = exitPos;
        _entity->m_front = exitFront;
        _entity->m_enabled = true;
        _entity->m_onGround = true;
        _entity->m_vel.Zero();

		if( _entity->m_type == Entity::TypeDarwinian )
		{
			Darwinian *d = (Darwinian *)_entity;
			d->m_state = Darwinian::StateIdle;
			d->m_wayPoint = d->m_pos;
			d->m_ordersSet = false;
		}
		else if( _entity->m_type == Entity::TypeInsertionSquadie )
		{
			Squadie *pointMan = (Squadie *)_entity;
			if( pointMan && pointMan->m_type == Entity::TypeInsertionSquadie )
			{
				Vector3 newPos(pointMan->m_pos);
				int unitId = pointMan->m_id.GetUnitId();
                int teamId = pointMan->m_id.GetTeamId();
				if( g_app->m_location->m_teams[teamId]->m_units.ValidIndex(unitId) )
				{
					Unit* unit = g_app->m_location->m_teams[teamId]->m_units.GetData(unitId);
					if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
					{
						InsertionSquad* is = (InsertionSquad *)unit;
						is->SetWayPoint(newPos);
					}
				}
			}
		}
        
        g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );

        g_app->m_soundSystem->TriggerEntityEvent( _entity, "ExitTeleport" );
        return true;
    }

    return false;
}


void RadarDish::ListSoundEvents ( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "BeginRotation" );
    _list->PutData( "EndRotation" );
    _list->PutData( "ConnectionEstablished" );
    _list->PutData( "ConnectionLost" );
}


bool RadarDish::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    // This method is overridden for Radar Dish in order to expand the number
    // of cells the Radar Dish is placed into.  We were having problems where
    // entities couldn't get into the radar dish because its door was right on
    // the edge of an obstruction grid cell, so the entity didn't see the
    // building at all

    SpherePackage sphere(_pos, _radius * 1.5);
    Matrix34 transform(m_front, m_up, m_pos);
    return m_shape->SphereHit(&sphere, transform);
}


bool RadarDish::DoesRayHit( Vector3 const &rayStart, Vector3 const &rayDir, 
                            float rayLen, Vector3 *pos, Vector3 *norm )
{
    RayPackage ray(rayStart, rayDir, rayLen);
    Matrix34 transform(m_front, g_upVector, m_pos);
    return m_shape->RayHit(&ray, transform, true);
}


void RadarDish::AdvanceAITarget()
{
	if( m_signal > 0.0 )
	{
		if( !m_aiTarget )
		{
			m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

			Matrix34 mat( m_front, g_upVector, m_pos );

			AITarget targetTemplate;
			targetTemplate.m_pos       = m_entrancePos;
			targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

			m_aiTarget->Initialise( (Building *)&targetTemplate );
			m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
			m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
			m_aiTarget->m_radarTarget = true;
			//m_aiTarget->m_priorityModifier = 0.1;

			g_app->m_location->m_buildings.PutData( m_aiTarget );
            g_app->m_location->RecalculateAITargets();
		}

        if( m_aiTarget->GetBuildingLink() == -1 )
        {
            RadarDish *r = (RadarDish *)g_app->m_location->GetBuilding( m_receiverId );
            if( r && r->m_type == Building::TypeRadarDish )
            {
                if( r->m_aiTarget )
                {
                    m_aiTarget->m_linkId = r->m_aiTarget->m_id.GetUniqueId();
                    g_app->m_location->RecalculateAITargets();
                }
            }
        }
		/*for( int i = 0; i < NUM_TEAMS; ++i )
		{
            if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault )
            {
                // priority needs to be higher than idle targets but lower than important things (enemy controlled, under attack/attacking, etc)
                m_aiTarget->m_priority[i] = 0.5f;
            }
            else
            {
                RadarDish *r = (RadarDish *)g_app->m_location->GetBuilding( m_receiverId );
                if( r && r->m_type == Building::TypeRadarDish )
                {
                    m_aiTarget->m_priority[i] = r->m_aiTarget->m_priority[i] * 0.9f;
                }
            }
		}*/
	}
	else
	{
		if( m_aiTarget )
		{
			m_aiTarget->m_destroyed = true;
			m_aiTarget->m_neighbours.EmptyAndDelete();
			m_aiTarget = NULL;
			g_app->m_location->RecalculateAITargets();
		}
	}
}

void RadarDish::SetBuildingLink(int _buildingId )
{
    m_validLinkList.PutData( _buildingId );
}

void RadarDish::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    if( _in->TokenAvailable() ) m_forceConnection = atoi(_in->GetNextToken());
    if( _in->TokenAvailable() ) m_autoConnectAtStart = atoi( _in->GetNextToken() );
    while( _in->TokenAvailable() )
    {
        m_validLinkList.PutData( atoi( _in->GetNextToken() ) );
    }
}

void RadarDish::Write(TextWriter *_out)
{
    Building::Write( _out );
    _out->printf( "%-5d", m_forceConnection );
    _out->printf( "%-5d", m_autoConnectAtStart );
    for( int i = 0; i < m_validLinkList.Size(); ++i )
    {
        _out->printf("%-5d", m_validLinkList[i]);        
    }
}

int RadarDish::CountValidLinks()
{
    return m_validLinkList.Size();
}

void RadarDish::AddValidLink( int _id )
{
    m_validLinkList.PutData( _id );
}

void RadarDish::ClearLinks()
{
    m_validLinkList.Empty();
}

bool RadarDish::NotAligned()
{
    return( !m_horizontallyAligned && !m_verticallyAligned );
}

bool RadarDish::ValidReceiverDish( int _buildingId )
{
    /*if( g_app->Multiplayer() )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( !g_app->m_location->m_buildings.ValidIndex(i) ) continue;
            RadarDish *r = (RadarDish *)g_app->m_location->m_buildings.GetData(i);
            if( r == this ) continue;

            if( r && r->m_type == Building::TypeRadarDish )
            {
               // if( r->GetConnectedDishId() == _buildingId ) return false; // another dish is already connected to the target                
            }
        }
    }*/

    if( g_app->m_multiwiniaTutorial && _buildingId == 11 ) 
    {
        return false;
    }

    if( m_forceTeamMatch )
    {
        Building *b = g_app->m_location->GetBuilding( _buildingId );
        if( m_id.GetTeamId() == b->m_id.GetTeamId() ) return true;
        else return false;
    }
    if( m_validLinkList.Size() == 0 ) return true;
    for( int i = 0; i < m_validLinkList.Size(); ++i )
    {
        if( _buildingId == m_validLinkList[i] ) return true;
    }
    return false;
}

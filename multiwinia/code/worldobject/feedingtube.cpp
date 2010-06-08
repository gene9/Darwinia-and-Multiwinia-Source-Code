#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/ogl_extensions.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"

#include "app.h"
#include "camera.h"
#include "globals.h"
#include "location.h"
#include "main.h"
#include "renderer.h"
#include "global_world.h"

#include "worldobject/feedingtube.h"



FeedingTube::FeedingTube()
:   m_receiverId(-1),
    m_range(0.0),
	m_signal(0.0)
{
    m_type = Building::TypeFeedingTube;
    //m_front.Set(0,0,1);    

    SetShape( g_app->m_resource->GetShape( "feedingtube.shp" ) );

	const char focusMarkerName[] = "MarkerFocus";
	m_focusMarker = m_shape->m_rootFragment->LookupMarker( focusMarkerName );
	AppReleaseAssert( m_focusMarker, "FeedingTube: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", focusMarkerName, m_shape->m_name );
}

// *** Initialise
void FeedingTube::Initialise( Building *_template )
{
    Building::Initialise( _template );
	AppDebugAssert(_template->m_type == Building::TypeFeedingTube);
    m_receiverId = ((FeedingTube *) _template)->m_receiverId;
}

bool FeedingTube::Advance ()
{
    Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
	Vector3 dishPos = worldMat.pos;

    FeedingTube *ft = (FeedingTube *)g_app->m_location->GetBuilding( m_receiverId );
    if( ft &&
        ft->m_type == Building::TypeFeedingTube )
    {		
        m_range = (ft->GetDishPos(0.0) - dishPos).Mag();
    }
    else
    {
        m_range = 0.0;
    }

    return Building::Advance();
}


Vector3 FeedingTube::GetDishPos( double _predictionTime )
{
	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_focusMarker->GetWorldMatrix(rootMat);
    return worldMat.pos;
}


Vector3 FeedingTube::GetDishFront( double _predictionTime )
{
    if( m_receiverId != -1 )
    {
        FeedingTube *receiver = (FeedingTube *) g_app->m_location->GetBuilding( m_receiverId );
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

Vector3 FeedingTube::GetForwardsClippingDir( double _predictionTime, FeedingTube *_sender )
{
	if (_sender == NULL) {
		return GetDishFront( _predictionTime );
	}

	Vector3 senderDishFront = _sender->GetDishFront( _predictionTime );
	
	Vector3 dishFront = GetDishFront( _predictionTime );

	// Make the two dishFronts point at each other.

	Vector3 SR = GetDishPos(_predictionTime) - _sender->GetDishPos(_predictionTime) ;
	SR.Normalise();

	if (SR * senderDishFront < 0)
		senderDishFront *= -1;
	
	if (SR * dishFront > 0)
		dishFront *= -1;	

	Vector3 combinedDirection = 
		-senderDishFront +
		dishFront;

	if (combinedDirection.MagSquared() < 0.5)
		return -dishFront;

	return combinedDirection.Normalise();
}

void FeedingTube::Render( double _predictionTime )
{
	Building::Render(_predictionTime);   
}


void FeedingTube::RenderAlphas ( double _predictionTime )
{
    if( g_app->m_editing ) return;

    if( m_receiverId != -1 )
    {
        RenderSignal( _predictionTime, 10.0, 0.4 );              
        RenderSignal( _predictionTime, 9.0, 0.2 );   
        RenderSignal( _predictionTime, 8.0, 0.2 );   
        RenderSignal( _predictionTime, 4.0, 0.5 );   
    }
}


void FeedingTube::RenderSignal( double _predictionTime, double _radius, double _alpha )
{
	START_PROFILE( "Signal");

    FeedingTube *receiver = (FeedingTube *) g_app->m_location->GetBuilding( m_receiverId );
    if( !receiver ) return;

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
	Vector3 dishFront   = GetForwardsClippingDir(_predictionTime, receiver);
    double eqn1[4]      = { dishFront.x, dishFront.y, dishFront.z, -1.0 };
    glClipPlane         (GL_CLIP_PLANE0, eqn1 );


    Vector3 receiverPos = receiver->GetDishPos( _predictionTime );
    Vector3 receiverFront = receiver->GetForwardsClippingDir( _predictionTime, this );
    glTranslatef        ( -startPos.x, -startPos.y, -startPos.z );
    glTranslatef        ( receiverPos.x, receiverPos.y, receiverPos.z );

    double eqn2[4]      = { receiverFront.x, receiverFront.y, receiverFront.z, -1.0 };
    glClipPlane         (GL_CLIP_PLANE1, eqn2 );
    glTranslatef        ( -receiverPos.x, -receiverPos.y, -receiverPos.z );


	//RenderArrow(startPos, startPos + dishFront * 100, 2.0, RGBAColour( 0, 255, 0, 255 ) );
	//RenderArrow(endPos, endPos + receiverFront * 100, 2.0, RGBAColour( 255, 0, 0, 255 ) );
	//RenderArrow(startPos, endPos, 2.0 );

    glTranslatef        ( startPos.x, startPos.y, startPos.z );    

    glEnable            (GL_CLIP_PLANE0);
    glEnable            (GL_CLIP_PLANE1);

    double texXInner = -g_gameTime/_radius;
    double texXOuter = -g_gameTime;

    //double texXInner = -1.0/_radius;
    //double texXOuter = -1.0;
	if (true) {
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
	}
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

Vector3 FeedingTube::GetStartPoint()
{
    return GetDishPos(0.0);
}


Vector3 FeedingTube::GetEndPoint()
{   
    return GetDishPos(0.0) + (GetDishFront(0.0) * m_range);
}


void FeedingTube::ListSoundEvents ( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "BeginRotation" );
    _list->PutData( "EndRotation" );
    _list->PutData( "ConnectionEstablished" );
    _list->PutData( "ConnectionLost" );
}


bool FeedingTube::DoesSphereHit(Vector3 const &_pos, double _radius)
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

int  FeedingTube::GetBuildingLink()
{
    return m_receiverId;
}

void FeedingTube::SetBuildingLink(int _buildingId)
{
    Building *b = g_app->m_location->GetBuilding( _buildingId );
    if( b &&
        b->m_type == Building::TypeFeedingTube )
    {
        m_receiverId = _buildingId;

		FeedingTube *p = (FeedingTube *) b;
    }
}

// *** Read
void FeedingTube::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
   
    m_receiverId = atoi(_in->GetNextToken());
}


// *** Write
void FeedingTube::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_receiverId);
}

bool FeedingTube::IsInView()
{
    Vector3 startPoint = GetStartPoint();
    Vector3 endPoint = GetEndPoint();

    Vector3 midPoint = ( startPoint + endPoint ) / 2.0;
    double radius = ( startPoint - endPoint ).Mag() / 2.0;
    radius += m_radius;

    if( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) )
    {
        return true;
    }

    return Building::IsInView();
}

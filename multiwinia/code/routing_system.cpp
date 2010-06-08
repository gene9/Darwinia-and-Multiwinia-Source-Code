#include "lib/universal_include.h"
#include <float.h>

#include "lib/math_utils.h"
#include "lib/debug_utils.h"
#include "lib/vector2.h"
#include "lib/debug_render.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"

#include "app.h"
#include "landscape.h"
#include "routing_system.h"
#include "location.h"
#include "team.h"
#include "main.h"
#include "camera.h"
#include "gamecursor.h"
#include "particle_system.h"

#include "worldobject/radardish.h"
#include "worldobject/armour.h"
#include "lib/matrix_cache.h"



// ****************************************************************************
// Class RoutingSystem
// ****************************************************************************

RoutingSystem::RoutingSystem()
{
	m_walkableRoute = false;
}

RoutingSystem::~RoutingSystem()
{
	m_routes.EmptyAndDelete();
	m_nodes.EmptyAndDelete();
}


Route *RoutingSystem::GetRoute( int id )
{
    if( !m_routes.ValidIndex(id) )
    {
        return NULL;
    }

    return m_routes[id];
}

template<class T>
class RoutingSystemCache : public SymmetricMatrixCache<T>
{
public:
	RoutingSystemCache( RoutingSystem *_rs )
		: SymmetricMatrixCache<T>( _rs->m_nodes.Size(), _rs->m_nodes.Size() ),
		  m_nodes( _rs->m_nodes )
	{
	}

protected:
	DArray<RouteNode *> &m_nodes;
};


class WalkableRSCache : public RoutingSystemCache<bool>
{
public:
	WalkableRSCache( RoutingSystem *_rs )
	: RoutingSystemCache<bool>( _rs )
	{
	}

	bool Compute( int _from, int _to )
	{
		return g_app->m_location->IsWalkable( 
			m_nodes[_from]->m_waypoint.GetPos(), 
            m_nodes[_to]->m_waypoint.GetPos(), m_nodes[_from]->m_checkCliffs && m_nodes[_to]->m_checkCliffs );
	}
};

class WalkableDistanceRSCache : public RoutingSystemCache<double>
{
public:
	WalkableDistanceRSCache ( RoutingSystem *_rs )
		: RoutingSystemCache<double>( _rs )
	{
	}

	double Compute( int _from, int _to )
	{
		return g_app->m_location->GetWalkingDistance( 
			m_nodes[_from]->m_waypoint.GetPos(), 
			m_nodes[_to]->m_waypoint.GetPos(), false );
	}
};

int RoutingSystem::GetNearestNode( Vector3 const &pos, bool requireWalkable )
{
    int nearestId = -1;
    double nearest = 999999.9;

    for( int i = 0; i < m_nodes.Size(); ++i )
    {
        if( m_nodes.ValidIndex(i) )
        {
            RouteNode *fromNode = m_nodes[i];

            double dist = ( pos - fromNode->m_waypoint.GetPos() ).Mag();
            if( dist < nearest )
            {
                int fence = -1;
                if( !requireWalkable ||
                    g_app->m_location->IsWalkable( pos, fromNode->m_waypoint.GetPos(), true, false, &fence ) )
                {
                    if( fence == -1 )
                    {
                        nearest = dist;
                        nearestId = i;
                    }
                }
            }
        }
    }

    return nearestId;
}


void RoutingSystem::DeleteRoute( int id )
{
    if( m_routes.ValidIndex(id) )
    {
        Route *route = m_routes[id];        
        m_routes.RemoveData(id);
        delete route;        
    }
}


int RoutingSystem::GenerateRoute( Vector3 const &fromPos, Vector3 const &toPos, bool directRoute, bool permitClimbing, bool noRounding )
{    
    if( g_app->IsSinglePlayer() ) return -1;

    double startTime = GetHighResTime();

    Route *route = new Route(-1);
    route->AddWayPoint( fromPos );

    //
    // Find the best nodes nearest to our from and to positions
    // If we cant find them, find the nearest anyway that may not be walkable

    int targetNodeId = GetNearestNode( toPos, true );
    int startNodeId = GetNearestNode( fromPos, true );

    if( targetNodeId == -1 ) targetNodeId = GetNearestNode( toPos, false );
    if( startNodeId == -1 ) startNodeId = GetNearestNode( fromPos, false );


    //
    // Is it walkable in a straight line?
    // Optimisation : only check if its actually possible to plot a complex route
    //                otherwise we _have_ to go in a straight line anyway

    bool complexRoutePossible = ( targetNodeId != -1 && startNodeId != -1 && !directRoute );
    bool straightLineWalkable = false;
    double straightLineWalkDistance = -1;

    if( complexRoutePossible )
    {
        Vector3 rightAngle = ( fromPos - toPos ) ^ g_upVector;
        rightAngle.SetLength(40);
        straightLineWalkable = g_app->m_location->IsWalkable( fromPos, toPos, true, permitClimbing ) &&
                               g_app->m_location->IsWalkable( fromPos-rightAngle, toPos-rightAngle, true, permitClimbing ) &&
                               g_app->m_location->IsWalkable( fromPos+rightAngle, toPos+rightAngle, true, permitClimbing );

        straightLineWalkDistance = g_app->m_location->GetWalkingDistance( fromPos, toPos );
    }                               
       

    //
    // plot our complex route

    if( !straightLineWalkable && complexRoutePossible )
    {
        //
        // Build the basic route from node to node
        
        RouteNode *currentNode = m_nodes[startNodeId];
        route->AddWayPoint( currentNode->m_waypoint.GetPos() );
        
        while( true )
        {
            int routeId = currentNode->GetRouteId( targetNodeId );
            if( routeId == -1 )
            {
                // Something went wrong, so bail out with what we have
                break;
            }

            RoutingTableEntry *entry = currentNode->m_routeTable[routeId];
            currentNode = m_nodes[entry->m_nextNode];
            route->AddWayPoint( currentNode->m_waypoint.GetPos() );

            if( entry->m_nextNode == targetNodeId ) 
            {
                // We have arrived at our destination node
                break;
            }

            if( route->m_wayPoints.Size() >= 100 )
            {
                AppDebugOut( "Error in route planning!\n" );
                route->m_wayPoints.EmptyAndDelete();
                route->AddWayPoint( fromPos );
                break;
            }
        }
    }
    
    route->AddWayPoint( toPos );


    //
    // Smooth out corners of the route

    if( route->m_wayPoints.Size() > 2 && !noRounding )
    {
        Vector3 fromPos = route->m_wayPoints[0]->GetPos();
        for( int i = 1; i < route->m_wayPoints.Size()-1; ++i )
        {
            Vector3 midPos = route->m_wayPoints[i]->GetPos();
            Vector3 toPos = route->m_wayPoints[i+1]->GetPos();
            
            double distance = ( midPos - fromPos ).Mag();
            int numSteps = distance / 50.0;

            Vector3 smootherPos = midPos;
            int thisMaxSmooth = 9999;

            for( int j = numSteps-1; j >= 0; --j )
            {
                thisMaxSmooth--;
                if( thisMaxSmooth <= 0 ) break;

                Vector3 thisPos = fromPos + ( midPos - fromPos ) * ( j / (double)numSteps );                
                
                Vector3 rightAngle = ( thisPos - toPos ) ^ g_upVector;
                rightAngle.SetLength(40);

                if( !g_app->m_location->IsWalkable( thisPos, toPos, true, true ) ||
                    !g_app->m_location->IsWalkable( thisPos-rightAngle, toPos-rightAngle, true, true ) ||
                    !g_app->m_location->IsWalkable( thisPos+rightAngle, toPos+rightAngle, true, true ) )
                {
                    break;
                }

                smootherPos = thisPos;
            }

            route->m_wayPoints[i]->SetPos( smootherPos );                
            fromPos = smootherPos;
        }
    }


    //
    // Merge route nodes that are very close together

    if( route->m_wayPoints.Size() > 2 )
    {
        for( int i = 0; i < route->m_wayPoints.Size()-1; ++i )
        {
			WayPoint *way1 = route->m_wayPoints[i];
			WayPoint *way2 = route->m_wayPoints[i+1];

            Vector3 pos1 = way1->GetPos();
            Vector3 pos2 = way2->GetPos();

            if( (pos1 - pos2).Mag() < 50 && g_app->m_location->IsWalkable( pos1, pos2, true ) )
            {
                Vector3 averagePos = ( pos1 + pos2 ) / 2.0;
                way1->SetPos( averagePos );
                route->m_wayPoints.RemoveData(i+1);
				delete way2;
                --i;
            }
        }
    }


    //
    // Set Y values of all route nodes based on landscape

    for( int i = 0; i < route->m_wayPoints.Size(); ++i )
    {
        Vector3 pos = route->m_wayPoints[i]->GetPos();
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        route->m_wayPoints[i]->SetPos(pos);
    }


    //
    // What is the walking distance of our calculated route?
    // If the direct route is actually quicker, take it instead
    // Note: Do not take the direct route if it involves climbing

    bool climbing = ( !straightLineWalkable && straightLineWalkDistance > 0.0 );

    if( straightLineWalkDistance >= 0 &&
        (!climbing || permitClimbing) )
    {
        double complexRouteWalkDistance = route->GetWalkingDistance();
        
        if( straightLineWalkDistance <= complexRouteWalkDistance )
        {
            route->m_wayPoints.EmptyAndDelete();
            route->AddWayPoint( fromPos );
            route->AddWayPoint( toPos );
        }
    }

    
    double timeTaken = 1000.0 * (GetHighResTime() - startTime);
    if( timeTaken > 10 )
    {
        AppDebugOut( "Performance warning : Route planned in %2.2ms\n", timeTaken );
    }

    int routeId = m_routes.PutData( route );
    return routeId;
}


void RoutingSystem::HighlightRoute( int routeId, int teamId )
{
    Route *route = GetRoute( routeId );
    if( !route ) return;

    RGBAColour colour( 255, 100, 255, 255 );
    if( g_app->Multiplayer() )
    {
        Team *team = g_app->m_location->m_teams[teamId];
        colour = team->m_colour;   
        colour.AddWithClamp( RGBAColour(100, 100, 100, 100) );
    }

    double stepSize = 10;

    for( int i = 0; i < route->m_wayPoints.Size()-1; ++i )
    {
        Vector3 from = route->m_wayPoints[i]->GetPos();
        Vector3 to = route->m_wayPoints[i+1]->GetPos();

        double dist = ( from - to ).Mag();
        int numSteps = dist / stepSize;

        for( int j = 0; j <= numSteps; ++j )
        {
            Vector3 thisPos = from + ( to - from ) * ( j / (double)numSteps );
            thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( thisPos.x, thisPos.z );
            thisPos += g_upVector * 2;

            g_app->m_particleSystem->CreateParticle( thisPos, g_upVector*10, Particle::TypeMuzzleFlash, 75.0, colour );
            g_app->m_particleSystem->CreateParticle( thisPos, g_upVector*10, Particle::TypeMuzzleFlash, 60.0, colour );
        }
    }
}


void RoutingSystem::RenderNodes()
{
    for( int i = 0; i < m_nodes.Size(); ++i )
    {
        if( m_nodes.ValidIndex(i) )
        {
            RouteNode *fromNode = m_nodes[i];

            RenderSphere( fromNode->m_waypoint.GetPos(), 10 );

            g_editorFont.DrawText3DCentre( fromNode->m_waypoint.GetPos() + g_upVector * 20,
                                           10, "%d", fromNode->m_routeTable.Size() );
            g_editorFont.DrawText3DCentre( fromNode->m_waypoint.GetPos() + g_upVector * 40,
                                           10, "%d", i );

            for( int j = 0; j < fromNode->m_routeTable.Size(); ++j )
            {
                RoutingTableEntry *entry = fromNode->m_routeTable[j];

                if( entry->m_nextNode == entry->m_targetNode &&
                    m_nodes.ValidIndex( entry->m_targetNode ) )
                {
                    RouteNode *toNode = m_nodes[ entry->m_targetNode ];

                    double stepSize = 40.0;   
                    double totalDistance = ( fromNode->m_waypoint.GetPos() - toNode->m_waypoint.GetPos() ).Mag();
                    int numSteps = totalDistance / stepSize;                                                           

                    glColor4f( 1.0, 0.0, 0.0, 0.5 );

                    glBegin( GL_LINE_STRIP );
                    for( int i = 0; i <= numSteps; ++i )
                    {
                        Vector3 position = fromNode->m_waypoint.GetPos() + ( toNode->m_waypoint.GetPos() - fromNode->m_waypoint.GetPos() ) * ( i / (double)numSteps );
                        position.y = g_app->m_location->m_landscape.m_heightMap->GetValue(position.x, position.z);
                        glVertex3dv( position.GetData() );
                    }
                    glEnd();
                }
            }
        }
    }
}

void RoutingSystem::Render()
{
    glDisable   ( GL_DEPTH_TEST );
    glEnable    ( GL_BLEND );

    //RenderNodes();    

    //
    // Render all routes
    
    for( int j = 0; j < m_routes.Size(); ++j )
    {
        if( m_routes.ValidIndex(j) )
        {
            Route *route = m_routes[j];

            Vector3 routeEndPoint = route->GetWayPoint( route->m_wayPoints.Size() - 1 )->GetPos();
            if( !g_app->m_location->m_landscape.IsInLandscape( routeEndPoint ) ) continue;

            route->m_alpha -= g_advanceTime * 0.25;
            if( route->m_alpha < 0.0 ) route->m_alpha = 0.0;

            if( route->m_alpha > 0.0 )
            {
				// If we have something that goes over water, it is always walkable.
				Entity* e = g_app->m_location->GetMyTeam()->GetMyEntity();
				m_walkableRoute = false;
				int nonWalkable = -1;

				if( e )
				{
					m_walkableRoute = e->m_type == Entity::TypeArmour ||
						e->m_type == Entity::TypeEngineer ||
						e->m_type == Entity::TypeHarvester;
				}

				// If it's not walkable, we either have no entity (selected Darwinians) or
				// we need to check if it's walkable
				if( !m_walkableRoute )
				{
					m_walkableRoute = true;
					
					for( int i = 0; i < route->m_wayPoints.Size()-1; ++i )
					{
						if( !g_app->m_location->IsWalkable(
								route->GetWayPoint(i)->GetPos(), 
								route->GetWayPoint(i+1)->GetPos()) )
						{
							nonWalkable = i;
							break;
						}
					}
				}

                for( int i = 0; i < route->m_wayPoints.Size()-1; ++i )
                {
					if( i == nonWalkable ) m_walkableRoute = false;
                    RenderRouteLine( route->GetWayPoint(i)->GetPos(),
                                     route->GetWayPoint(i+1)->GetPos(), route->m_alpha );         
                }    
            }
        }
    }

    glLineWidth( 1.0 );
    glEnable( GL_DEPTH_TEST );
}


void RoutingSystem::SetRouteIntensity( int routeId, double intensity )
{
    if( !m_routes.ValidIndex(routeId) )
    {
        return;
    }

    Route *route = m_routes[routeId];
    route->m_alpha = max( route->m_alpha, intensity );
}


void RoutingSystem::RenderRouteLine( Vector3 const &fromPos, Vector3 const &toPos, double intensity )
{
    Vector3 midpoint = ( fromPos + toPos ) / 2.0;
    double camDistance = ( g_app->m_camera->GetPos() - midpoint ).Mag();

    double spotSize = camDistance / 100.0;
    double stepSize = spotSize * 2.0;

	// use g_app->m_location->IsWalkable(fromPos,toPos) to make only the non-walkable part a different colour
	RGBAColour colour = m_walkableRoute ? 
		g_app->m_location->GetMyTeam()->m_colour :
		RGBAColour(0,0,0,100);
    colour.AddWithClamp( RGBAColour(50,50,50,100) );

    Vector3 pos = toPos;
    double distance = ( fromPos - toPos ).Mag();
    int numSteps = distance / stepSize;
    Vector3 step = ( fromPos - toPos ) / numSteps;

    Vector3 camUp = g_app->m_camera->GetUp() * spotSize;
    Vector3 camRight = g_app->m_camera->GetRight() * spotSize;

    double thisAlpha = intensity;
    thisAlpha = max( thisAlpha, 0.0 );
    thisAlpha = min( thisAlpha, 1.0 );

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/particle.bmp" ) );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for( int i = 0; i < numSteps; ++i )
    {
        double thisAlpha2 = thisAlpha * ( 0.5 + iv_sin(GetHighResTime() * 10 + i * 0.7) * 0.5 );

        colour.a = thisAlpha2 * 255;

        Vector3 thisPos = pos;
        thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        if( thisPos.y < 0 ) thisPos.y = 0;
        thisPos.y += 1;

        glColor4ub( 0, 0, 0, colour.a * 0.5f );

        glBegin( GL_QUADS );
			glTexCoord2i(0,0);      glVertex3dv( (thisPos - camUp).GetData() );
			glTexCoord2i(1,0);      glVertex3dv( (thisPos + camRight).GetData() );
			glTexCoord2i(1,1);      glVertex3dv( (thisPos + camUp).GetData() );
			glTexCoord2i(0,1);      glVertex3dv( (thisPos - camRight).GetData() );
        glEnd();
      
        glColor4ubv( colour.GetData() );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (thisPos - camUp).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (thisPos + camRight).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (thisPos + camUp).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (thisPos - camRight).GetData() );
        glEnd();

        pos += step;
    }

    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );


    //g_app->m_gameCursor->RenderOffscreenArrow( (Vector3 &)from, (Vector3 &)to, intensity );
    //return;

    //double stepSize = 100;
    //
    //Vector3 rightAngle = ( fromPos - toPos ) ^ g_upVector;
    //rightAngle.y = 0;
    //rightAngle.SetLength(20);

    //double distance = ( fromPos - toPos ).Mag();
    //int numSteps = distance / stepSize;

    //double timeNow = GetHighResTime();

    //glDisable       ( GL_CULL_FACE );
    //glEnable        ( GL_TEXTURE_2D );    
    //glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/widearrow.bmp" ) );
    //glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    //glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    //glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    //glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );


    //double alpha = 0.9 * intensity;
    //if( alpha < 0 ) alpha = 0;
    //if( alpha > 1 ) alpha = 1;
    //RGBAColour teamCol = g_app->m_location->GetMyTeam()->m_colour;
    //glColor4ub( teamCol.r, teamCol.g, teamCol.b, alpha * 255 );
    //
    //glBegin         ( GL_QUAD_STRIP );

    //for( int i = 0; i <= numSteps; ++i )
    //{                       
    //    Vector3 thisPos = fromPos + ( toPos - fromPos ) * ( i / (double)numSteps);
    //    thisPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( thisPos.x, thisPos.z );

    //    glTexCoord2f(0,i*5);      glVertex3dv( (thisPos - rightAngle).GetData() );
    //    glTexCoord2f(1,i*5);      glVertex3dv( (thisPos + rightAngle).GetData() );
    //}

    //glEnd();
    //glDisable       ( GL_TEXTURE_2D );
}


RouteNode::RouteNode()
:   m_checkCliffs(true)
{
}

RouteNode::~RouteNode()
{
	m_routeTable.EmptyAndDelete();
}

int RouteNode::GetRouteId( int targetNodeId )
{
    for( int i = 0; i < m_routeTable.Size(); ++i )
    {
        if( m_routeTable[i]->m_targetNode == targetNodeId )
        {
            return i;
        }
    }

    return -1;
}


void RouteNode::AddRoute( int targetNodeId, int nextNodeId, double totalDistance )
{
    //
    // Do we already have a quicker route to this node?

    for( int i = 0; i < m_routeTable.Size(); ++i )
    {
        if( m_routeTable[i]->m_targetNode == targetNodeId &&
            m_routeTable[i]->m_totalDistance <= totalDistance )
        {
            // We already have a quicker or identical route
            return;
        }
    }

    //
    // If we have an existing route, delete it and replace it

    int routeId = GetRouteId( targetNodeId );
    if( routeId != -1 )
    {
        RoutingTableEntry *oldRoute = m_routeTable[routeId];
        m_routeTable.RemoveData(routeId);
        delete oldRoute;
    }


    //
    // Add the entry

    RoutingTableEntry *entry = new RoutingTableEntry;
    entry->m_targetNode = targetNodeId;
    entry->m_nextNode = nextNodeId;
    entry->m_totalDistance = totalDistance;

    m_routeTable.PutData( entry );
}



// ****************************************************************************
// Class WayPoint
// ****************************************************************************

WayPoint::WayPoint()
:   m_type(Type3DPos),
    m_buildingId(-1)
{
}


// *** Constructor
WayPoint::WayPoint(int _type, Vector3 const &_pos)
:	m_type(_type),
	m_pos(_pos),
	m_buildingId(-1)
{
}


// *** Destructor
WayPoint::~WayPoint()
{
}


// *** GetPos
// I could have made it so that the Y values for GroundPoses were set once at
// level load time, but that would require an init phases after level file
// parsing and landscape terrain generation.
Vector3 WayPoint::GetPos()
{
	Vector3 rv(m_pos);

	if (m_type == TypeGroundPos)
	{
		//Landscape *land = &g_app->m_location->m_landscape;
		//rv.y = land->m_heightMap->GetValue(rv.x, rv.z);
		//if (rv.y < 0.0)
		//{
		//	rv.y = 0.0;
		//}
		//rv.y += 1.0;
	}
	else if (m_type == TypeBuilding)
	{        
		Building *building = g_app->m_location->GetBuilding(m_buildingId);
        if( building )
        {
            AppDebugAssert( building->m_type == Building::TypeRadarDish ||
                         building->m_type == Building::TypeBridge );
            Teleport *teleport = (Teleport *) building;
            Vector3 pos, front;
            teleport->GetEntrance( pos, front );
		    return pos;
        }
	}

	return rv;
}


// *** SetPos
void WayPoint::SetPos(Vector3 const &_pos)
{
	m_pos = _pos;
}


// ****************************************************************************
// Class Route
// ****************************************************************************

// *** Constructor
Route::Route(int id)
:   m_id(id),
    m_alpha(0.0)
{
}


// *** Destructor
Route::~Route()
{
	m_wayPoints.EmptyAndDelete();
}


void Route::AddWayPoint(Vector3 const &_pos)
{
    WayPoint *wayPoint = new WayPoint( WayPoint::Type3DPos, _pos );
    m_wayPoints.PutDataAtEnd( wayPoint );
}


void Route::AddWayPoint(int _buildingId)
{
    WayPoint *wayPoint = new WayPoint( WayPoint::TypeBuilding, g_zeroVector );
    wayPoint->m_buildingId = _buildingId;
    m_wayPoints.PutDataAtEnd( wayPoint );
}


WayPoint *Route::GetWayPoint(int _id)
{
    if( m_wayPoints.ValidIndex(_id) )
    {
        WayPoint *wayPoint = m_wayPoints[_id];
        return wayPoint;
    }

    return NULL;
}


int	Route::GetIdOfNearestWayPoint(Vector3 const &_pos)
{
	int idOfNearest = -1;
	double distToNearestSqrd = FLT_MAX;

	int size = m_wayPoints.Size();
	for (int i = 0; i < size; ++i)
	{
		WayPoint *wp = m_wayPoints.GetData(i);
		Vector3 delta = _pos - wp->GetPos();
		double distSqrd = delta.MagSquared();
		if (distSqrd < distToNearestSqrd)
		{
			distToNearestSqrd = distSqrd;
			idOfNearest = i;
		}
	}

	return idOfNearest;
}


// Returns the id of the first waypoint of the nearest edge
int	Route::GetIdOfNearestEdge(Vector3 const &_pos, double *_dist)
{
	int idOfNearest = 0;
	double distToNearest = FLT_MAX;
	
	Vector2 pos(_pos.x, _pos.z);
	WayPoint *wp = m_wayPoints.GetData(0);
	Vector3 newPos = wp->GetPos();
	Vector2 oldPos( newPos.x, newPos.z );
	
	int size = m_wayPoints.Size();
	for (int i = 1; i < size; ++i)
	{
		wp = m_wayPoints.GetData(i);
		newPos = wp->GetPos();
		Vector2 temp(newPos.x, newPos.z);
		double dist = PointSegDist2D(pos, oldPos, temp);
		oldPos = temp;

		if (dist < distToNearest)
		{
			distToNearest = dist;
			idOfNearest = i;
		}
	}

	return idOfNearest - 1;
}


Vector3 Route::GetEndPoint()
{
    return m_wayPoints[ m_wayPoints.Size() - 1 ]->GetPos();
}


double Route::GetWalkingDistance()
{
    double totalDistance = 0;
    
    for( int i = 0; i < m_wayPoints.Size()-1; ++i )
    {
        Vector3 from = m_wayPoints[i]->GetPos();
        Vector3 to = m_wayPoints[i+1]->GetPos();

        double thisDist = g_app->m_location->GetWalkingDistance( from, to );
        if( thisDist < 0 ) return thisDist;

        totalDistance += thisDist;
    }
    
    return totalDistance;
}

LandRoutingSystem::LandRoutingSystem()
:	RoutingSystem()
{
}

void LandRoutingSystem::Initialise()
{
    double startTime = GetHighResTime();

    //
    // Load nodes based on the AI Target objects in the world

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *target = (AITarget *)building;

                RouteNode *node = new RouteNode();
                node->m_checkCliffs = target->m_checkCliffs;
                node->m_waypoint.SetPos( target->m_pos );
                m_nodes.PutData( node );
            }
        }
    }

	WalkableRSCache walkableCache( this );
	WalkableDistanceRSCache walkableDistanceCache( this );

    //
    // Calculate all nearby walkable nodes
    
    for( int i = 0; i < m_nodes.Size(); ++i )
    {
        if( m_nodes.ValidIndex(i) )
        {
            RouteNode *fromNode = m_nodes[i];

            for( int j = 0; j < m_nodes.Size(); ++j )
            {
                if( m_nodes.ValidIndex(j) )
                {
                    RouteNode *toNode = m_nodes[j];

                    double distance = ( fromNode->m_waypoint.GetPos() - toNode->m_waypoint.GetPos() ).Mag();
                    if( distance < 600 &&
						walkableCache( i, j ) )
                    {
                        double walkDistance = 
							walkableDistanceCache( i, j );
                        fromNode->AddRoute( j, j, walkDistance );
                    }
                }
            }
        }
    }


    //
    // Propagate all routes to all nodes

    for( int n = 0; n < m_nodes.Size(); ++n )
    {
        for( int i = 0; i < m_nodes.Size(); ++i )
        {
            if( m_nodes.ValidIndex(i) )
            {
                RouteNode *fromNode = m_nodes[i];

                for( int j = 0; j < fromNode->m_routeTable.Size(); ++j )
                {                
                    RoutingTableEntry *fromEntry = fromNode->m_routeTable[j];             
                    int toNodeId = fromEntry->m_nextNode;
                    RouteNode *toNode = m_nodes[ toNodeId ];

                    double walkDistance = walkableDistanceCache( i, toNodeId );

                    for( int k = 0; k < toNode->m_routeTable.Size(); ++k )
                    {
                        RoutingTableEntry *entry = toNode->m_routeTable[k];                        
                        double totalDistance = entry->m_totalDistance;
                        totalDistance += walkDistance;

                        fromNode->AddRoute( entry->m_targetNode, toNodeId, totalDistance );                        
                    }
                }
            }
        }
    }


    //
    // Check propogation worked ok

   /* for( int i = 0; i < m_nodes.Size(); ++i )
    {
        AppDebugOut( "Node %d has %d routes (should be %d)\n", i, m_nodes[i]->m_routeTable.Size(), m_nodes.Size() );
    }*/


    double totalTime = GetHighResTime() - startTime;
    AppDebugOut( "Routing System intitialised in %dms\n", int(totalTime * 1000) );
}

SeaRoutingSystem::SeaRoutingSystem()
:	RoutingSystem()
{
}

void SeaRoutingSystem::Initialise()
{
	double startTime = GetHighResTime();

    //
    // Load nodes based on the AI Target objects in the world

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *target = (AITarget *)building;

                RouteNode *node = new RouteNode();
                node->m_waypoint.SetPos( target->m_pos );
                node->m_checkCliffs = false;
                m_nodes.PutData( node );
            }
        }
    }

	WalkableDistanceRSCache walkableDistanceCache( this );

    //
    // Calculate all nearby walkable nodes
    
    for( int i = 0; i < m_nodes.Size(); ++i )
    {
        if( m_nodes.ValidIndex(i) )
        {
            RouteNode *fromNode = m_nodes[i];

            for( int j = 0; j < m_nodes.Size(); ++j )
            {
                if( m_nodes.ValidIndex(j) )
                {
                    RouteNode *toNode = m_nodes[j];

                    double distance = ( fromNode->m_waypoint.GetPos() - toNode->m_waypoint.GetPos() ).Mag();

					if( distance < 600 && Armour::IsRouteClear( fromNode->m_waypoint.GetPos(), toNode->m_waypoint.GetPos() ) )
                    {
                        double walkDistance = distance;
//							walkableDistanceCache( i, j );
                        fromNode->AddRoute( j, j, walkDistance );
                    }
                }
            }
        }
    }


    //
    // Propagate all routes to all nodes

    for( int n = 0; n < m_nodes.Size(); ++n )
    {
        for( int i = 0; i < m_nodes.Size(); ++i )
        {
            if( m_nodes.ValidIndex(i) )
            {
                RouteNode *fromNode = m_nodes[i];

                for( int j = 0; j < fromNode->m_routeTable.Size(); ++j )
                {                
                    RoutingTableEntry *fromEntry = fromNode->m_routeTable[j];             
                    int toNodeId = fromEntry->m_nextNode;
                    RouteNode *toNode = m_nodes[ toNodeId ];

                    double walkDistance = (m_nodes[i]->m_waypoint.GetPos() - m_nodes[toNodeId]->m_waypoint.GetPos()).Mag();

                    for( int k = 0; k < toNode->m_routeTable.Size(); ++k )
                    {
                        RoutingTableEntry *entry = toNode->m_routeTable[k];                        
                        double totalDistance = entry->m_totalDistance;
                        totalDistance += walkDistance;

                        fromNode->AddRoute( entry->m_targetNode, toNodeId, totalDistance );                        
                    }
                }
            }
        }
    }


    //
    // Check propogation worked ok

    for( int i = 0; i < m_nodes.Size(); ++i )
    {
		if( m_nodes[i]->m_routeTable.Size() != m_nodes.Size() )
		{
			AppDebugOut( "Node %d has %d routes (should be %d)\n", i, m_nodes[i]->m_routeTable.Size(), m_nodes.Size() );
		}
    }


    double totalTime = GetHighResTime() - startTime;
    AppDebugOut( "Routing System intitialised in %dms\n", int(totalTime * 1000) );
}
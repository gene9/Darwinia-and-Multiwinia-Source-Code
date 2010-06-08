#include "lib/universal_include.h"

#include <float.h>

#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/debug_utils.h"

#include "app.h"
#include "camera.h"
#include "global_internet.h"
#include "global_world.h"
#include "main.h"


#define DISPLAY_LIST_NAME_LINKS "GlobalInternetLinks"
#define DISPLAY_LIST_NAME_NODES "GlobalInternetNodes"


//*****************************************************************************
// Class GlobalInternetNode
//*****************************************************************************

GlobalInternetNode::GlobalInternetNode()
:   m_size(0),
    m_burst(0),
	m_numLinks(0)
{
}


void GlobalInternetNode::AddLink(int _id)
{
	DarwiniaDebugAssert(m_numLinks < GLOBALINTERNET_MAXNODELINKS);
	m_links[m_numLinks] = _id;
	m_numLinks++;
}


// ****************************************************************************
// Class GlobalInternet
// ****************************************************************************

GlobalInternet::GlobalInternet()
:   m_links(0),
	m_numLinks(0),
	m_nodes(NULL),
	m_numNodes(0),
    m_nearestNodeToCentre(-1),
    m_nearestDistance(FLT_MAX)
{
    darwiniaSeedRandom(1);
    GenerateInternet();
}


GlobalInternet::~GlobalInternet()
{
	DeleteInternet();
}


unsigned short GlobalInternet::GenerateInternet( Vector3 const &_pos, unsigned char _size )
{
	GetHighResTime();

    GlobalInternetNode *node = &m_nodes[m_numNodes];
    node->m_pos = _pos;
    node->m_size = _size;    
    unsigned short nodeIndex = m_numNodes;
	m_numNodes++;
	DarwiniaDebugAssert(m_numNodes < GLOBALINTERNET_MAXNODES);

    float distanceToCentre = _pos.Mag();
    if( distanceToCentre < m_nearestDistance )
    {
        m_nearestDistance = distanceToCentre;
        m_nearestNodeToCentre = nodeIndex;
    }

    unsigned char numLinks = _size;
    float distance = powf( _size, 4.0f ) * 2.0f;
    
    while( numLinks > 0 )
    {
        float z = sfrand(distance);
        float y = sfrand(distance);
        float x = sfrand(distance);
        Vector3 newPos = _pos + Vector3(x,y,z);
        unsigned short newIndex = GenerateInternet( newPos, _size-1 );
        m_links[m_numLinks].m_from = nodeIndex;
        m_links[m_numLinks].m_to = newIndex;
        m_links[m_numLinks].m_size = _size;

        m_nodes[newIndex].AddLink( m_numLinks );
        node->AddLink( m_numLinks );

		m_numLinks++;
		DarwiniaDebugAssert(m_numLinks <= GLOBALINTERNET_MAXLINKS);

        --numLinks;
    }

    return nodeIndex;    
}


void GlobalInternet::GenerateInternet()
{
    double timeStart = GetHighResTime();

	m_links = new GlobalInternetLink[GLOBALINTERNET_MAXLINKS];
	m_nodes = new GlobalInternetNode[GLOBALINTERNET_MAXNODES];

    //Vector3 centre(200, 200, 200);
    //Vector3 centre(449,1787,-139);
    Vector3 centre(-797,1949,-1135);
    unsigned short firstNode = GenerateInternet( centre, GLOBALINTERNET_ITERATIONS );
    
    m_nodes[m_numNodes].m_pos.Zero();
    m_nodes[m_numNodes].m_size = 0.0f;
    unsigned short nodeIndex = m_numNodes;
	m_numNodes++;
	DarwiniaDebugAssert(m_numNodes <= GLOBALINTERNET_MAXNODES);
    
    m_links[m_numLinks].m_from = m_nearestNodeToCentre;
    m_links[m_numLinks].m_to = nodeIndex;
    m_links[m_numLinks].m_size = 1.0f;
    m_numLinks++;
	DarwiniaDebugAssert(m_numLinks <= GLOBALINTERNET_MAXLINKS);
    
    for( int i = 0; i < m_numNodes; ++i )
    {
        GlobalInternetNode *node = &m_nodes[i];
        if( node->m_numLinks == 1 )
        {
            m_leafs.PutData( i );
        }
    }

#ifdef DEBUG
    for( int i = 0; i < 5; ++i )
    {
        GlobalInternetNode *node = &m_nodes[i];
        DebugOut( "Node %d : %2.2f %2.2f %2.2f\n", i, node->m_pos.x, node->m_pos.y, node->m_pos.z );

/*
        Node 0 : -797.00 1949.00 -1135.00
        Node 1 : 675.75 1643.66 1259.99
        Node 2 : 727.92 1423.32 459.74
        Node 3 : 324.37 928.37 646.87
        Node 4 : 140.59 1095.22 520.61
*/
    }

    double timeTaken = GetHighResTime() - timeStart;
    DebugOut( "Global Internet time to generate : %.2fms\n", timeTaken*1000.0 );
    DebugOut( "Global Internet number of nodes  : %d\n", m_numNodes );
    DebugOut( "Global Internet number of links  : %d\n", m_numLinks );
    DebugOut( "Global Internet number of leafs  : %d\n", m_leafs.Size() );
#endif
}


void GlobalInternet::DeleteInternet()
{
    delete [] m_nodes;
	m_numNodes = 0;
    delete [] m_links;
	m_numLinks = 0;
    m_leafs.Empty();
    m_bursts.Empty();

    g_app->m_resource->DeleteDisplayList(DISPLAY_LIST_NAME_LINKS);
    g_app->m_resource->DeleteDisplayList(DISPLAY_LIST_NAME_NODES);
}


void GlobalInternet::Render()
{
    START_PROFILE(g_app->m_profiler, "Internet");

    /*static*/ float scale = 1000.0f;

    glPushMatrix();
    glScalef    ( scale, scale, scale );


    float fog = 0.0f;
    float fogCol[] = { fog, fog, fog, fog };

    /*static*/ int fogVal = 5810000;
//    if( g_keys[KEY_P] )
//    {
//        fogVal += 100000;
//    }
//    if( g_keys[KEY_O] )
//    {
//        fogVal -= 100000;
//    }
    
    glFogf      ( GL_FOG_DENSITY, 1.0f );
    glFogf      ( GL_FOG_START, 0.0f );
    glFogf      ( GL_FOG_END, (float) fogVal );
    glFogfv     ( GL_FOG_COLOR, fogCol );
    glFogi      ( GL_FOG_MODE, GL_LINEAR );
    glEnable    ( GL_FOG );
    
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDepthMask     ( false );
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );


	int linksId = g_app->m_resource->GetDisplayList(DISPLAY_LIST_NAME_LINKS);
    if( linksId >= 0 )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laserfence2.bmp") );

        glCallList( linksId );
    }
    else
    {
        linksId = g_app->m_resource->CreateDisplayList(DISPLAY_LIST_NAME_LINKS);
        glNewList(linksId, GL_COMPILE);       

        glColor4f       ( 0.25f, 0.25f, 0.5f, 0.8f );

		//
		// Render Links

        glBegin( GL_QUADS );

        for( int i = 0; i < m_numLinks; ++i )
        {
            GlobalInternetLink *link = &m_links[i];
            GlobalInternetNode *from = &m_nodes[ link->m_from ];
            GlobalInternetNode *to = &m_nodes[ link->m_to ];

            Vector3 fromPos         = from->m_pos;
            Vector3 toPos           = to->m_pos;
            Vector3 midPoint        = (toPos + fromPos)/2.0f;
            Vector3 camToMidPoint   = g_zeroVector - midPoint;
            Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
        
            rightAngle *= link->m_size * 0.5f;
        
            glTexCoord2i( 0, 0 );       glVertex3fv( (fromPos - rightAngle).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3fv( (fromPos + rightAngle).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (toPos + rightAngle).GetData() );                
            glTexCoord2i( 1, 0 );       glVertex3fv( (toPos - rightAngle).GetData() );                
        }
        glEnd();

        glEndList();
    }


    int nodesId = g_app->m_resource->GetDisplayList(DISPLAY_LIST_NAME_NODES);
    if( nodesId >= 0 )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp") );

        glCallList( nodesId );
    }
    else
    {
        nodesId = g_app->m_resource->CreateDisplayList(DISPLAY_LIST_NAME_NODES);
        glNewList(nodesId, GL_COMPILE);

        glColor4f       ( 0.8f, 0.8f, 1.0f, 0.6f );
        float nodeSize = 10.0f;

        glBegin         ( GL_QUADS );
        for( int i = 0; i < m_numNodes; ++i )
        {
            GlobalInternetNode *node = &m_nodes[i];
       
            Vector3 camToMidPoint   = (g_zeroVector - node->m_pos).Normalise();
            Vector3 right           = camToMidPoint ^ Vector3(0,0,1);
            Vector3 up              = right ^ camToMidPoint;
            right *= nodeSize;
            up *= nodeSize;
            glTexCoord2i( 0, 0 );       glVertex3fv( (node->m_pos - up - right).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3fv( (node->m_pos - up + right).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (node->m_pos + up + right).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3fv( (node->m_pos + up - right).GetData() );
        }
        glEnd();

        glEndList();
    }


    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );    
    
        
   
    RenderPackets();
    
    g_app->m_globalWorld->SetupFog();
    glDisable( GL_FOG );

    glPopMatrix ();

    END_PROFILE(g_app->m_profiler,  "Internet" );
}


void GlobalInternet::TriggerPacket( unsigned short _nodeId, unsigned short _fromLinkId )
{
    GlobalInternetNode *newNode = &m_nodes[ _nodeId ];

    if( newNode->m_numLinks == 1 && newNode->m_links[0] == _fromLinkId )
    {
        return;
    }

    while( true )
    {
        int newLinkIndex = darwiniaRandom() % newNode->m_numLinks;
        if( newNode->m_links[newLinkIndex] != _fromLinkId )
        {
            GlobalInternetLink *newLink = &m_links[ newNode->m_links[newLinkIndex] ];
            if( newLink->m_from == _nodeId )
            {
                newLink->m_packets.PutData( 1.0f );
            }
            else
            {
                newLink->m_packets.PutData( -1.0f );
            }
            break;
        }
    }
}


void GlobalInternet::RenderPackets()
{
    //
    // Create new packets

    if( frand(100.0f) < 11.0f )
    {
        int leafIndex = frand( m_leafs.Size() );    
        GlobalInternetNode *node = &m_nodes[m_leafs[leafIndex]];
        node->m_burst = 4.0f;
        m_bursts.PutData( m_leafs[leafIndex] );
    }


    //
    // Deal with data bursts

    for( int i = 0; i < m_bursts.Size(); ++i )
    {
        GlobalInternetNode *node = &m_nodes[ m_bursts[i] ];
        node->m_burst -= g_advanceTime;
        if( node->m_burst > 0.0f )
        {
            TriggerPacket( m_bursts[i], -1 );
        }
        else
        {
            m_bursts.RemoveData(i);
            --i;
        }
    }


    //
    // Advance / render all packets

    float packetSize = 30.0f;
    Vector3 camRight = g_app->m_camera->GetRight() * packetSize;
    Vector3 camUp = g_app->m_camera->GetUp() * packetSize;
    float posChange = g_advanceTime;

    glColor4f( 0.25f, 0.25f, 0.5f, 0.8f );

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    
    for( int i = 0; i < m_numLinks; ++i )
    {
        GlobalInternetLink *link = &m_links[i];
        for( int j = 0; j < link->m_packets.Size(); ++j )
        {
            float *thisPacket = link->m_packets.GetPointer(j);
            float packetVal = *thisPacket;
            if( *thisPacket > 0.0f )
            {
                *thisPacket -= posChange;
                if( *thisPacket <= 0.01f )
                {
                    *thisPacket = 0.01f;
                    TriggerPacket( link->m_to, i );
                    link->m_packets.RemoveData(j);
                    --j;
                }
            }
            else if( *thisPacket < 0.0f )
            {
                *thisPacket += posChange;
                if( *thisPacket >= -0.01f )
                {
                    *thisPacket = -0.01f;
                    TriggerPacket( link->m_from, i );
                    link->m_packets.RemoveData(j);
                    --j;                        
                }
            }                
        
            GlobalInternetNode *from = &m_nodes[ link->m_from ];
            GlobalInternetNode *to = &m_nodes[ link->m_to ];
            Vector3 packetPos;
            if( packetVal > 0.0f )
            {
                packetPos = to->m_pos + ( from->m_pos - to->m_pos ) * packetVal;
            }
            else if( packetVal < 0.0f )
            {
                packetPos = from->m_pos + ( to->m_pos - from->m_pos ) * -packetVal;
            }
                        
            glBegin( GL_QUADS );
                glTexCoord2i( 0, 0 );       glVertex3fv( (packetPos - camUp - camRight).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3fv( (packetPos - camUp + camRight).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3fv( (packetPos + camUp + camRight).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3fv( (packetPos + camUp - camRight).GetData() );
            glEnd();                
        }
    }

    glDepthMask ( true );
    glDisable   ( GL_TEXTURE_2D );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable   ( GL_BLEND );    
    glEnable    ( GL_CULL_FACE );

    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
}

#include "lib/universal_include.h"
#include "lib/tosser/darray.h"
#include "lib/tosser/directory.h"
#include "lib/netlib/net_lib.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"

#include "matchmaker.h"
#include "metaserver.h"
#include "metaserver_defines.h"


// ============================================================================
// Static data for MatchMaker

static char     *s_matchMakerIp = NULL;
static int      s_matchMakerPort = -1;

static NetMutex s_uniqueRequestMutex;
static int      s_uniqueRequestid = 0;


struct MatchMakerListener
{
public:
    MatchMakerListener()
        :   m_listener(NULL),
            m_ip(NULL),
            m_port(-1),
            m_uniqueId(-1),
            m_identified(false),
            m_shutDown(false)
    {
    }

    NetSocketListener   *m_listener;
    char                *m_ip;
    int                 m_uniqueId;
    int                 m_port;
    bool                m_identified;
    bool                m_shutDown;
};

static DArray<MatchMakerListener *> s_listeners;
static NetMutex                     s_listenersMutex;


// ============================================================================


void MatchMaker_LocateService( const char *_matchMakerIp, int _port )
{
    if( s_matchMakerIp )
    {
        delete s_matchMakerIp;
        s_matchMakerIp = NULL;
    }

    if( _matchMakerIp ) 
    {
        s_matchMakerIp = strdup(_matchMakerIp);
        s_matchMakerPort = _port;
    }
}

static int GetListenerIndex( NetSocketListener *_listener, const NetLockMutex &_lockSListeners )
{
    for( int i = 0; i < s_listeners.Size(); ++i )
    {
        if( s_listeners.ValidIndex(i) )
        {
            MatchMakerListener *listener = s_listeners[i];
            if( listener->m_listener == _listener )
            {
                return i;
            }
        }
    }

    return -1;
}

static void RemoveListener( MatchMakerListener *_listener, NetLockMutex &_lockSListeners )
{
    for( int i = 0; i < s_listeners.Size(); ++i )
    {
		if( s_listeners.ValidIndex(i) && s_listeners[i] == _listener )
        {
			s_listeners.RemoveData(i);
			return;
		}
	}
}

static MatchMakerListener *GetListener( NetSocketListener *_listener, const NetLockMutex &_lockSListeners )
{
    int listenerIndex = GetListenerIndex(_listener, _lockSListeners);
    if( listenerIndex == -1 ) return NULL;
    
    return s_listeners[listenerIndex];
}


static NetCallBackRetType RequestIdentityThread(void *ignored)
{
#ifdef WAN_PLAY_ENABLED
    NetSocketListener *listener = (NetSocketListener *) ignored;    
	MatchMakerListener *_listener = GetListener( listener, NetLockMutex(s_listenersMutex) );
    AppAssert(_listener);

	AppDebugOut("RequestIdentityThread for listener %p started\n", _listener );

    //
    // Generate a uniqueID for this request
    // So we can tell the replies apart

    s_uniqueRequestMutex.Lock();
    int uniqueId = s_uniqueRequestid;
    ++s_uniqueRequestid;
    s_uniqueRequestMutex.Unlock();

    _listener->m_uniqueId = uniqueId;


    //
    // Build our request and convert to a byte stream
    // (only need to do this once and keep re-sending)

    Directory request;
    request.SetName( NET_MATCHMAKER_MESSAGE );
    request.CreateData( NET_METASERVER_COMMAND, NET_MATCHMAKER_REQUEST_IDENTITY );
    request.CreateData( NET_MATCHMAKER_UNIQUEID, uniqueId );
    

    //
    // Open a connection to the MatchMaker service
    // Start sending requests for our ID every few seconds
    // to ensure the connection stays open in the NAT 

    NetSocketSession socket( *listener, s_matchMakerIp, s_matchMakerPort  );


    while( true )
    {
        //
        // Stop if We've been asked to 

        if( _listener->m_shutDown )
        {
            break;
        }


        //
        // Update the request with the latest auth data
        
        Directory *clientProps = MetaServer_GetClientProperties();
        request.CreateData( NET_METASERVER_GAMENAME,    clientProps->GetDataString(NET_METASERVER_GAMENAME) );
        request.CreateData( NET_METASERVER_GAMEVERSION, clientProps->GetDataString(NET_METASERVER_GAMEVERSION) );
        request.CreateData( NET_METASERVER_AUTHKEY,     clientProps->GetDataString(NET_METASERVER_AUTHKEY) );
        delete clientProps;

        int requestSize = 0;
        char *requestByteStream = request.Write(requestSize);


        //
        // Send the request

        int numBytesWritten = 0;
        NetRetCode result = socket.WriteData( requestByteStream, requestSize, &numBytesWritten );
        delete [] requestByteStream;

        if( result != NetOk || numBytesWritten != requestSize )
        {
            AppDebugOut( "MatchMaker encountered error sending data\n" );
            break;
        }

        NetSleep( PERIOD_MATCHMAKER_REQUESTID );
    }


    //
    // Shut down the request

	AppDebugOut("Deleting MatchMakerListener %p\n", _listener);
    delete _listener;

#endif

    return 0;
}

static bool IsRequestingIdentity( NetSocketListener *_listener, NetLockMutex &_lock )
{
    MatchMakerListener *listener = GetListener(_listener, _lock);
    if( !listener ) return false;
    if( listener->m_shutDown ) return false;

    return true;
}

bool MatchMaker_IsRequestingIdentity( NetSocketListener *_listener )
{
	NetLockMutex lock( s_listenersMutex );
	return IsRequestingIdentity( _listener, lock );
}


void MatchMaker_StartRequestingIdentity( NetSocketListener *_listener )
{
	NetLockMutex lock( s_listenersMutex );

    AppAssert( s_matchMakerIp );

    if( _listener &&
        !IsRequestingIdentity(_listener, lock) )
    {
        MatchMakerListener *listener = new MatchMakerListener();
        listener->m_listener = _listener;
        int index = s_listeners.PutData(listener);

		AppDebugOut( "Started requesting public IP:port for socket %d (%p)\n", 
			_listener->GetBoundSocketHandle(), listener );
        
        NetStartThread( RequestIdentityThread, _listener );
    }
}


void MatchMaker_StopRequestingIdentity( NetSocketListener *_listener )
{
	NetLockMutex lock(s_listenersMutex);

	MatchMakerListener *listener = GetListener(_listener, lock);

    if( listener )
    {
        AppDebugOut( "Stopped requesting public IP:port for socket %d (%p)\n", 
			_listener->GetBoundSocketHandle(), listener );

        listener->m_shutDown = true;
		RemoveListener( listener, lock );
    }
}


bool MatchMaker_IsIdentityKnown( NetSocketListener *_listener )
{
	NetLockMutex lock(s_listenersMutex);

    MatchMakerListener *listener = GetListener(_listener, lock );

    if( !listener ) return false;

    return( listener->m_identified );    
}


bool MatchMaker_GetIdentity( NetSocketListener *_listener, char *_ip, int *_port )
{
	NetLockMutex lock(s_listenersMutex);

    MatchMakerListener *listener = GetListener(_listener, lock);

    if( !listener ) return false;

    if( !listener->m_identified ) return false;

    strcpy( _ip, listener->m_ip );
    *_port = listener->m_port;

    return true;
}


void MatchMaker_RequestConnection( NetSocketListener *_listener, const char *_targetIp, int _targetPort, Directory *_myDetails )
{
#ifdef WAN_PLAY_ENABLED
    AppDebugOut( "Requesting connection to %s:%d via matchmaker\n", _targetIp, _targetPort );

    Directory request( *_myDetails );
    request.SetName     ( NET_MATCHMAKER_MESSAGE );
    request.CreateData  ( NET_METASERVER_COMMAND, NET_MATCHMAKER_REQUEST_CONNECT );
    request.CreateData  ( NET_MATCHMAKER_TARGETIP, _targetIp );
    request.CreateData  ( NET_MATCHMAKER_TARGETPORT, _targetPort );

    AppAssert( s_matchMakerIp );

	if( !_listener )
	{
		AppDebugOut("NULL listener requesting connection to %s:%d\n", _targetIp, _targetPort );
		return;
	}

	NetSocketSession socket( *_listener, s_matchMakerIp, s_matchMakerPort  );
    MetaServer_SendDirectory( &request, &socket );
#endif
}


bool MatchMaker_ReceiveMessage( NetSocketListener *_listener, Directory *_message )
{
    AppAssert( _message );
    AppAssert( strcmp(_message->m_name, NET_MATCHMAKER_MESSAGE) == 0 );
    AppAssert( _message->HasData( NET_METASERVER_COMMAND, DIRECTORY_TYPE_STRING ) );

    char *cmd = _message->GetDataString( NET_METASERVER_COMMAND );

    if( strcmp( cmd, NET_MATCHMAKER_IDENTIFY ) == 0 )
    {
        //
        // This message contains the external IP and port of one of our connections

        if( !_message->HasData( NET_MATCHMAKER_UNIQUEID, DIRECTORY_TYPE_INT ) ||
            !_message->HasData( NET_MATCHMAKER_YOURIP, DIRECTORY_TYPE_STRING ) ||
            !_message->HasData( NET_MATCHMAKER_YOURPORT, DIRECTORY_TYPE_INT ) )
        {
            AppDebugOut( "MatchMaker : Received badly formed identity message, discarded\n" );
        }
        else
        {
            int uniqueId = _message->GetDataInt( NET_MATCHMAKER_UNIQUEID );
            
            s_listenersMutex.Lock();
            for( int i = 0; i < s_listeners.Size(); ++i )
            {
                if( s_listeners.ValidIndex(i) )
                {
                    MatchMakerListener *listener = s_listeners[i];
                    if( listener->m_uniqueId == uniqueId )
                    {
                        if( !listener->m_identified )
                        {                        
                            listener->m_ip = strdup( _message->GetDataString(NET_MATCHMAKER_YOURIP) );
                            listener->m_port = _message->GetDataInt(NET_MATCHMAKER_YOURPORT);
                            listener->m_identified = true;
                            AppDebugOut( "Socket %d identified as public IP %s:%d\n", 
                                            listener->m_listener->GetBoundSocketHandle(), listener->m_ip, listener->m_port );
                        }
                        break;
                    }
                }
            }
            s_listenersMutex.Unlock();
        }
    }
    else if( strcmp( cmd, NET_MATCHMAKER_REQUEST_CONNECT ) == 0 )
    {
        //
        // This is a request from a client for the server to set up a UDP hole punch
        
        if( !_message->HasData( NET_METASERVER_IP, DIRECTORY_TYPE_STRING ) ||
            !_message->HasData( NET_METASERVER_PORT, DIRECTORY_TYPE_INT ) )
        {
            AppDebugOut( "MatchMaker : Received badly formed connection request, discarded\n" );
        }
        else
        {
            char *ip = _message->GetDataString( NET_METASERVER_IP );
            int port = _message->GetDataInt( NET_METASERVER_PORT );

            AppDebugOut( "MatchMaker : SERVER Received request to allow client to join from %s:%d\n", ip, port );

            Directory dir;
            dir.SetName( NET_MATCHMAKER_MESSAGE );
            dir.CreateData( NET_METASERVER_COMMAND, NET_MATCHMAKER_RECEIVED_CONNECT );

            NetSocketSession session( *_listener, ip, port );            
            MetaServer_SendDirectory( &dir, &session );
        }
    }
    else if( strcmp( cmd, NET_MATCHMAKER_RECEIVED_CONNECT ) == 0 )
    {
        AppDebugOut( "MatchMaker : CLIENT received confirmation from Server that Hole Punch is set up\n" );
    }
    else
    {
        AppAbort( "unrecognised matchmaker message" );
    }

    return true;
}



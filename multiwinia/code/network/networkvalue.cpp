#include "lib/universal_include.h"
#include "network/networkvalue.h"
#include "network/clienttoserver.h"
#include "app.h"
#include "multiwinia.h"
#include "game_menu.h"
#include "UI/MapData.h"

// NetworkValue

void NetworkValue::ToLatinString( char *_buf, int _bufSize )
{
	UnicodeString result;
	ToUnicodeString( result );

	strncpy( _buf, result.m_charstring, _bufSize );

	if( _bufSize > 0 )	
		_buf[ _bufSize - 1 ] = '\0';
}

void NetworkValue::FromLatinString( const char *_buf )
{
	FromUnicodeString( UnicodeString( _buf ) );
}

// BoundedNetworkValue

BoundedNetworkValue::BoundedNetworkValue( int _type )
:	NetworkValue( _type ),
	m_bounded( false ), 
	m_min( 0.0 ), 
	m_max( m_max )
{
}

void BoundedNetworkValue::Unbind()
{
	m_bounded = false;
	m_min = 0.0; 
	m_max = m_max;
}

void BoundedNetworkValue::SetBounds( double _min, double _max )
{
	m_bounded = true;
	m_min = _min;
	m_max = _max;
}

double BoundedNetworkValue::Bound( double _x )
{
	if( m_bounded )
	{
		if( _x < m_min )
			return m_min;
		if( _x > m_max)
			return m_max;
	}
	return _x;
}

// NetworkBool

void NetworkBool::ToUnicodeString( UnicodeString &_to )
{
	wchar_t buf[256];
	swprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%d", Get() );
	_to = UnicodeString( buf );
}

void NetworkBool::FromUnicodeString( const UnicodeString &_from )
{
	int value;
	if( swscanf( _from.m_unicodestring, L"%d", &value ) == 1)
		Set( value );
}

// NetworkIntAsBool

bool NetworkIntAsBool::Get() const
{
	return m_networkInt->Get() != 0;
}

void NetworkIntAsBool::Set( bool _x )
{
	m_networkInt->Set( (int) _x );
}

// NetworkInt

void NetworkInt::ToUnicodeString( UnicodeString &_to )
{
	wchar_t buf[256];
	swprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%d", Get() );
	_to = UnicodeString( buf );
}

void NetworkInt::FromUnicodeString( const UnicodeString &_from )
{
	int value;
	if( swscanf( _from.m_unicodestring, L"%d", &value ) == 1)
		Set( value );
}

// NetworkIntAsString

const char *NetworkIntAsString::Get() const
{
	sprintf( m_buf, "%d", m_networkInt->Get() );
	return m_buf;
}

void NetworkIntAsString::Set( const char *_x )
{
	int value = 0;
	sscanf( _x, "%d", &value );
	m_networkInt->Set( value );
}

// NetworkFloat

void NetworkFloat::ToUnicodeString( UnicodeString &_to )
{
	wchar_t buf[256];
	swprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%.2f", Get() );
	_to = UnicodeString( buf );
}

void NetworkFloat::FromUnicodeString( const UnicodeString &_from )
{
	double value;
	if( swscanf( _from.m_unicodestring, L"%lf", &value ) == 1 )
		Set( iv_trunc_low_nibble(value) );
}

// NetworkDouble

void NetworkDouble::ToUnicodeString( UnicodeString &_to )
{
	wchar_t buf[256];
	swprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%.2f", Get() );
	_to = UnicodeString( buf );
}

void NetworkDouble::FromUnicodeString( const UnicodeString &_from )
{
	double value;
	if( swscanf( _from.m_unicodestring, L"%lf", &value ) == 1 )
		Set( iv_trunc_low_nibble(value) );
}


// NetworkString

void NetworkString::ToUnicodeString( UnicodeString &_to )
{
	_to = UnicodeString( Get() );
}

void NetworkString::FromUnicodeString( const UnicodeString &_from )
{
	Set( _from.m_charstring );
}

// NetworkUnicodeString

void NetworkUnicodeString::ToUnicodeString( UnicodeString &_to )
{
	_to = Get();
}

void NetworkUnicodeString::FromUnicodeString( const UnicodeString &_from )
{
	Set( _from );
}

// NetworkChar

void NetworkChar::ToUnicodeString( UnicodeString &_to )
{
	wchar_t buf[256];
	swprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%d", Get() );
	_to = UnicodeString( buf );
}


void NetworkChar::FromUnicodeString( const UnicodeString &_from )
{
	//wchar_t buf[256];
	int value;
	if( swscanf( _from.m_unicodestring, L"%d", &value ) == 1)
		Set( (unsigned char) value );
}

// LocalString

void LocalString::Set( const char *_x )
{
	snprintf( m_buf, m_bufSize, "%s", _x );
	m_buf[m_bufSize - 1] = '\0';
}

// NetworkGameOptionInt

void NetworkGameOptionInt::Set( int _value )
{
	g_app->m_clientToServer->RequestSetGameOptionInt( m_gameOption, Bound(_value) );			
}

int NetworkGameOptionInt::m_dataEmpty = 0;

// NetworkIntArray

NetworkIntArray::NetworkIntArray( int _size, const NetworkIntFactory &_factory )
:	m_size( _size )
{
	m_array = new NetworkInt *[ _size ];
	for( int i = 0; i < _size; i++ )
	{
		m_array[i] = _factory.Create( i );
	}
}

NetworkIntArray::~NetworkIntArray()
{
	for( int i = 0; i < m_size; i++ )
	{
		delete m_array[i];
		m_array[i] = NULL;
	}

	delete[] m_array;
	m_array = NULL;
}
	
NetworkInt& NetworkIntArray::operator []( int _index )
{
	return *m_array[_index];
}

// NetworkGameParam

int NetworkGameParam::Get() const
{
	return g_app->m_multiwinia->m_params[m_index];
}

void NetworkGameParam::Set( int _value )
{
	g_app->m_clientToServer->RequestSetGameOptionArrayElementInt( 
		GAMEOPTION_GAME_PARAM, m_index, Bound(_value) );
}

// NetworkGameResearchLevel

int NetworkGameResearchLevel::Get() const
{
	return g_app->m_globalWorld->m_research->m_researchLevel[m_index];
}

void NetworkGameResearchLevel::Set( int _value )
{
	g_app->m_clientToServer->RequestSetGameOptionArrayElementInt( 
		GAMEOPTION_GAME_RESEARCH_LEVEL, m_index, Bound(_value) );
}

// NetworkGameOptionBool

void NetworkGameOptionBool::Set( int _value )
{
	g_app->m_clientToServer->RequestSetGameOptionInt( m_gameOption, Bound(_value) );			
}

// NetworkGameOptionString

void NetworkGameOptionString::Set( const char *_value )
{
	g_app->m_clientToServer->RequestSetGameOptionString( m_gameOption, _value );
}


/*
int NetworkGameType::Get() const
{
	return g_app->m_multiwinia->m_gameType;
}

void NetworkGameType::Set( int _value )
{
	g_app->m_clientToServer->RequestSetGameOptionInt( GAMEOPTION_GAME_TYPE, _value );

	// When setting the game type, also set up the map to the first value
	g_app->m_clientToServer->RequestSetGameOptionInt( GAMEOPTION_m_gameOption, _value );			

}
*/

int GetMapId( const char *_mapName, int _gameType )
{	
	if( _gameType < 0 || _gameType >= MAX_GAME_TYPES )
		return -1;

	DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[_gameType];

	for( int i = 0; i < maps.Size(); i++ )
	{
		if( maps.ValidIndex( i ) )
		{
			if( stricmp( maps[i]->m_fileName, _mapName ) == 0 )
			{
				return i;					
				break;
			}
		}
	}

	return -1;
}

void SetMapId( int _value, int _gameType )
{
	if( _gameType < 0 || _gameType >= MAX_GAME_TYPES )
		return;

	DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[_gameType];

	if( maps.ValidIndex( _value ) )
	{
		g_app->m_clientToServer->RequestSetGameOptionString( 
			GAMEOPTION_MAPNAME,
			maps[ _value ]->m_fileName );
	}
}

// NetworkGameOptionMapId

int NetworkGameOptionMapId::Get() const
{
	return GetMapId( g_app->m_requestedMap, g_app->m_multiwinia->m_gameType );
}

void NetworkGameOptionMapId::Set( int _value )
{
	SetMapId( _value, g_app->m_multiwinia->m_gameType );
}


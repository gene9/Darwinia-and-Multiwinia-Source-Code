#ifndef __NETWORK_VALUE
#define __NETWORK_VALUE

#include "lib/unicode/unicode_string.h"

class NetworkValue {
public:

	// To match with InputField::Type
	enum
	{
		TypeNowt,
		TypeChar,
		TypeBool,
		TypeInt,
		TypeFloat,
		TypeString,
		TypeUnicodeString
	};

	NetworkValue( int _type /* InputField::Type */)
		: m_type( _type )		
	{
	}

	virtual ~NetworkValue()
	{
	}

	void ToLatinString( char *_buf, int _bufSize );
	void FromLatinString( const char *_buf );

	virtual void ToUnicodeString( UnicodeString &_to ) = 0;
	virtual void FromUnicodeString( const UnicodeString &_from ) = 0;


private:
	int m_type;
};

class BoundedNetworkValue : public NetworkValue
{
public:

	BoundedNetworkValue( int _type );

	void SetBounds( double _min, double _max );
	double Bound( double _x );
	void Unbind();

private:
	bool m_bounded;
	double m_min, m_max;
};


class NetworkInt : public BoundedNetworkValue {
public:
	NetworkInt()
		: BoundedNetworkValue( TypeInt )
	{
	}

	void operator = (int _value) 
	{ 
		Set( _value ); 
	}

	operator int() const
	{
		return Get();
	}

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );

	virtual int Get() const = 0;
	virtual void Set( int _x ) = 0;
};

class NetworkBool : public BoundedNetworkValue {
public:
	NetworkBool()
		: BoundedNetworkValue( TypeBool )
	{
	}

	void operator = (int _value) 
	{ 
		Set( _value ); 
	}

	operator int() const
	{
		return Get();
	}

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );

	virtual bool Get() const = 0;
	virtual void Set( bool _x ) = 0;
};


class NetworkIntAsBool : public NetworkBool {
public:
	NetworkIntAsBool( NetworkInt *_networkInt )
		: m_networkInt( _networkInt )
	{
	}

	void operator = (int _value) 
	{ 
		Set( _value ); 
	}

	operator int() const
	{
		return Get();
	}

	bool Get() const;
	void Set( bool _x );

private:
	NetworkInt *m_networkInt;
};

class NetworkFloat : public BoundedNetworkValue 
{
public:
	NetworkFloat()
		: BoundedNetworkValue( TypeFloat )
	{
	}

	void operator = (float _value) 
	{ 
		Set( _value ); 
	}

	operator float() const
	{
		return Get();
	}

	virtual float Get() const = 0;
	virtual void Set( float _x ) = 0;

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );

};

class NetworkDouble : public BoundedNetworkValue 
{
public:
	NetworkDouble()
		: BoundedNetworkValue( TypeFloat )
	{
	}

	void operator = (double _value) 
	{ 
		Set( _value ); 
	}

	operator double() const
	{
		return Get();
	}

	virtual double Get() const = 0;
	virtual void Set( double _x ) = 0;

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );

};


class NetworkChar : public BoundedNetworkValue 
{
public:
	NetworkChar()
		: BoundedNetworkValue( TypeChar )
	{
	}

	void operator = (unsigned char _value) 
	{ 
		Set( _value ); 
	}

	operator unsigned char() const
	{
		return Get();
	}

	virtual unsigned char Get() const = 0;
	virtual void Set( unsigned char _x ) = 0;

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );

};

class NetworkUnicodeString : public NetworkValue
{
public:
	NetworkUnicodeString()
		: NetworkValue( TypeUnicodeString )
	{
	}

	virtual const UnicodeString &Get() const = 0;
	virtual void Set( const UnicodeString &_x ) = 0;

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );
};

class NetworkString : public NetworkValue
{
public:
	NetworkString()
		: NetworkValue( TypeString )
	{
	}

	virtual const char *Get() const = 0;
	virtual void Set( const char *_x ) = 0;

	void ToUnicodeString( UnicodeString &_to );
	void FromUnicodeString( const UnicodeString &_from );
};

class NetworkIntAsString : public NetworkString
{
public:
	NetworkIntAsString( NetworkInt *_networkInt )
		: m_networkInt( _networkInt )
	{
	}

	const char *Get() const;
	void Set( const char *_x );

private:
	mutable char m_buf[64];
	NetworkInt *m_networkInt;
};

class LocalString : public NetworkString
{
public:
	LocalString( char *_buf, int _bufSize )
		: m_buf( _buf ), m_bufSize( _bufSize )
	{
	}

	const char *Get() const
	{
		return m_buf;
	}

	void Set( const char *_x );

private:
	char *m_buf;
	int m_bufSize;
};

class LocalUnicodeString : public NetworkUnicodeString {
public:
	LocalUnicodeString( UnicodeString &_value )
		: m_value( _value )
	{
	}

	const UnicodeString &Get() const
	{
		return m_value;
	}

	void Set( const UnicodeString &_x )
	{
		m_value = _x;
	}

private:
	UnicodeString &m_value;
};


template <class Type, class StorageType, class Super>
class LocalType : public Super {
public:
	LocalType( StorageType &_value )
		: m_value( _value )
	{
	}

	Type Get() const
	{
		return m_value;
	}

	void Set( Type _x )
	{
		m_value = _x;
	}

private:
	StorageType &m_value;
};

typedef LocalType<int, int, NetworkInt> LocalInt;
typedef LocalType<float, float, NetworkFloat> LocalFloat;
typedef LocalType<double, double, NetworkDouble> LocalDouble;
typedef LocalType<unsigned char, unsigned char, NetworkChar> LocalChar;
typedef LocalType<bool, bool, NetworkBool> LocalBool;
typedef LocalType<bool, int, NetworkBool> LocalIntAsBool;

class NetworkGameOptionInt : public NetworkInt {
public:
	NetworkGameOptionInt()
	:	m_gameOption( -1 ),
		m_data( m_dataEmpty )
	{
	}

	NetworkGameOptionInt( int _gameOption, int &_data )
	:	m_gameOption(_gameOption),
		m_data( _data )
	{
	}

	int Get() const
	{
		return m_data;
	}

	void Set( int _value );

	void operator = (int _value) 
	{ 
		Set( _value ); 
	}

	operator int() const
	{
		return Get();
	}

private:
	int &m_data;
	// Only used for empty constructor
	static int m_dataEmpty;
	
	void operator = (const NetworkGameOptionInt &opt);

	int m_gameOption;
};

class NetworkIntFactory {
public:
	virtual NetworkInt *Create( int _index ) const = 0;
};


class NetworkGameResearchLevel : public NetworkInt {
public:	
	NetworkGameResearchLevel( int _index )
		: m_index( _index )
	{
	}

	int Get() const;
	void Set( int _value );

private:
	int m_index;
};

class NetworkGameResearchLevelFactory : public NetworkIntFactory 
{
public:
	NetworkInt *Create( int _index ) const
	{
		return new NetworkGameResearchLevel( _index );
	}
};


class NetworkGameParam : public NetworkInt {
public:
	NetworkGameParam( int _index )
		: m_index( _index )
	{
	}

	int Get() const;
	void Set( int _value );

private:
	int m_index;
};

class NetworkGameParamFactory : public NetworkIntFactory 
{
public:
	NetworkInt *Create( int _index ) const
	{
		return new NetworkGameParam( _index );
	}
};


class NetworkIntArray {
public:
	NetworkIntArray( int _size, const NetworkIntFactory &_factory );
	~NetworkIntArray();
		
	NetworkInt& operator []( int _index );

private:
	
	int m_size;
	NetworkInt **m_array;
};

class NetworkGameOptionBool : public NetworkInt {
public:
	NetworkGameOptionBool( int _gameOption, bool &_data )
	:	m_gameOption(_gameOption),
		m_data( _data )
	{
	}

	int Get() const 
	{
		return m_data;
	}

	void Set( int _value );

	void operator = (bool _value) 
	{ 
		Set( _value ); 
	}

	operator bool() 
	{
		return Get();
	}

private:
	bool &m_data;
	int m_gameOption;
};


class NetworkGameOptionString {
public:
	NetworkGameOptionString( int _gameOption, const char *&_data )
	:	m_gameOption(_gameOption),
		m_data( _data )
	{
	}

	const char *Get() const 
	{
		return m_data;
	}

	void Set( const char *_value );

private:
	const char *&m_data;
	int m_gameOption;
};

class NetworkGameOptionMapId : public NetworkInt {
public:
	int Get() const;
	void Set( int _value );

	void operator = (int _value) 
	{ 
		Set( _value ); 
	}

	operator int() const
	{
		return Get();
	}
};

#endif

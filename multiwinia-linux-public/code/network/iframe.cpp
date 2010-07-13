#include "lib/universal_include.h"
#include "iframe.h"

#include "lib/math/random_number.h"
#include "lib/network_stream.h"
#include "network/clienttoserver.h"
#include "location.h"
#include "app.h"
#include "team.h"
#include "unit.h"
#include "main.h"

#include "contrib/unrar/rarbloat.h"
#include "contrib/unrar/sha1.h"

#include <strstream>
#include <fstream>

#include "network/syncdiff.h"

//#define CHECK_IFRAME_SERIALISATION

IFrame::IFrame()
:	m_data(NULL),
	m_sequenceId(0),
	m_length(0),
	m_hashValue(0)
{
}


IFrame::IFrame( IFrame const &_copyMe )
:	m_data( NULL ),
	m_sequenceId( _copyMe.m_sequenceId ),
	m_length( _copyMe.m_length ),
	m_hashValue( _copyMe.m_hashValue )
{
	if( _copyMe.m_data ) 
	{
		m_data = new char [m_length];
		memcpy( m_data, _copyMe.m_data, m_length );
	}
}


IFrame::IFrame( const char *_data, int _length )
:	m_data( NULL ),
	m_length( _length )
{
	if( _data )
	{
		m_data = new char [m_length];
		memcpy( m_data, _data, m_length );
	}

	// The sequence id is the first piece of data
	std::istrstream input(_data, _length);
	ReadNetworkValue(input, m_sequenceId );

	CalculateHashValue();
}


IFrame::~IFrame()
{
	delete[] m_data;
	m_data = NULL;
}


unsigned char IFrame::HashValue() const
{
	return m_hashValue;
}


int IFrame::SequenceId() const
{
	return m_sequenceId;
}


const char *IFrame::Data() const
{
	return m_data;
}


int IFrame::Length() const
{
	return m_length;
}


void ReadTag( std::istream &_input, const char *_tag )
{
	char tag[1024];
	int tagLen = strlen(_tag);
	_input.read( tag, tagLen );

    bool matches = strncmp( tag, _tag, tagLen ) == 0;
#ifdef _DEBUG

	if( !matches )
	{
		AppDebugOut("Remaining data: [");

		while (!_input.eof())
			AppDebugOut("%c", _input.get());
	}
#endif
	//AppReleaseAssert( matches, "Failed to read tag: %s\n", _tag );
}


void WriteTag( std::ostream &_output, const char *_tag )
{
	_output.write( _tag, strlen(_tag) );
}


class IFrameEntity
{
public:
	Vector3 m_pos, m_vel, m_front, m_up, m_centre;
    double m_radius;
	WorldObjectId m_id;

    IFrameEntity()
        :   m_radius(0.0)
    {
    }
};


struct IFrameUnit
{
	~IFrameUnit()
	{
		m_entities.EmptyAndDelete();
	}

	WorldObjectId m_id;
	Vector3 m_centrePos;
	DArray<IFrameEntity *> m_entities;
};


struct IFrameTeam
{
	~IFrameTeam()
	{
		m_units.EmptyAndDelete();
		m_others.EmptyAndDelete();
	}

	DArray<IFrameUnit *> m_units;
	DArray<IFrameEntity *> m_others;
};


struct IFrameWorld
{	
	int						m_sequenceId;
	double					m_networkTime;
	IFrameTeam				m_team[NUM_TEAMS];
	DArray<IFrameEntity *>	m_lasers, m_effects, m_buildings;
	double					m_syncRand;
};


bool Different( const IFrameEntity &_a, const IFrameEntity &_b )
{
	return !(_a.m_pos == _b.m_pos &&
			 _a.m_vel == _b.m_vel &&
             _a.m_front == _b.m_front &&
             _a.m_up == _b.m_up &&
             _a.m_centre == _b.m_centre &&
             _a.m_radius == _b.m_radius );
}


bool Different( const IFrameUnit &_a, const IFrameUnit &_b )
{
	if( !(_a.m_centrePos == _b.m_centrePos &&
		  _a.m_entities.Size() == _b.m_entities.Size()) )
		return true;

	int entitiesSize = _a.m_entities.Size();
	for( int i = 0; i < entitiesSize; i++ )
	{
		if( _a.m_entities.ValidIndex( i ) != _b.m_entities.ValidIndex( i ) )
			return true;
	
		if( _a.m_entities.ValidIndex( i ) )
		{
			if( Different( *_a.m_entities[i], *_b.m_entities[i] ) )
				return true;
		}
	}

	return false;
}


void ReadIFrameData( std::istream &_input, IFrameWorld &_world )
{
	ReadNetworkValue( _input, _world.m_sequenceId );
	ReadNetworkValue( _input, _world.m_networkTime );

	// Teams
	ReadTag( _input, "Teams" ); 

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
		IFrameTeam *team = &_world.m_team[t];

		bool teamUsed;
		ReadNetworkValue( _input, teamUsed );
		AppReleaseAssert( _input, "teamUsed" );

		if( teamUsed )
		{
			DArray<IFrameUnit *> &units = team->m_units;
			int unitsSize;
			ReadNetworkValue( _input, unitsSize );

            for( int u = 0; u < unitsSize; ++u )
            {
				bool validUnitIndex;
				ReadNetworkValue( _input, validUnitIndex );
				AppReleaseAssert( _input, "validUnitIndex" );

				if( validUnitIndex )
				{
					IFrameUnit *unit = new IFrameUnit;
					units.PutData( unit, u );

					ReadNetworkValue( _input, unit->m_centrePos );
					
					int entitiesSize;
					ReadNetworkValue( _input, entitiesSize );

                    for( int e = 0; e < entitiesSize; ++e )
                    {
						bool validEntityIndex;
						ReadNetworkValue( _input, validEntityIndex );
						AppReleaseAssert( _input, "validEntityIndex" );

						if( validEntityIndex )
						{
							IFrameEntity *ent = new IFrameEntity;
							unit->m_entities.PutData( ent, e );

							ReadNetworkValue( _input, ent->m_pos );
							ReadNetworkValue( _input, ent->m_vel );
                            ReadNetworkValue( _input, ent->m_front );

							int uniqueId;

							ReadNetworkValue( _input, uniqueId );
							ent->m_id = WorldObjectId( t, u, e, uniqueId );							
						}
					}
				}			
			}

			int othersSize;
			ReadNetworkValue( _input, othersSize );
			DArray<IFrameEntity *> &others = team->m_others;

			for( int e = 0; e < othersSize; ++e )
			{
				bool validOtherIndex;

				ReadNetworkValue( _input, validOtherIndex );
				AppReleaseAssert( _input, "validOtherIndex" );

				if( validOtherIndex )
				{
					IFrameEntity *entity = new IFrameEntity;
					others.PutData( entity, e );

					ReadNetworkValue( _input, entity->m_pos );
					ReadNetworkValue( _input, entity->m_vel );
                    ReadNetworkValue( _input, entity->m_front );

					int uniqueId;
					ReadNetworkValue( _input, uniqueId );
					entity->m_id = WorldObjectId( t, -1 /* Other */, e, uniqueId /* Unknown */ );					
				}
			}
		}		
	}

	// Lasers
	ReadTag( _input, "Lasers" ); 

	int lasersSize;
	ReadNetworkValue( _input, lasersSize );
	DArray<IFrameEntity *> &lasers = _world.m_lasers;

	for( int l = 0; l < lasersSize; ++l )
	{
		bool validLaserIndex;
		ReadNetworkValue( _input, validLaserIndex );
		
		if( validLaserIndex )
		{
			IFrameEntity *laser = new IFrameEntity;
			lasers.PutData( laser, l );
			
			ReadNetworkValue( _input, laser->m_pos );
			ReadNetworkValue( _input, laser->m_vel );
		}
	}

	// Effects
	ReadTag( _input, "Effects" );

	int effectsSize;
	ReadNetworkValue( _input, effectsSize );
	DArray<IFrameEntity *> &effects = _world.m_effects;
	
	for( int e = 0; e < effectsSize; ++e )
	{
		bool validEffectIndex;
		ReadNetworkValue( _input, validEffectIndex );

		if( validEffectIndex )
		{
			IFrameEntity *wobj = new IFrameEntity;
			effects.PutData( wobj, e );

			ReadNetworkValue( _input, wobj->m_pos );
			ReadNetworkValue( _input, wobj->m_vel );
		}
	}

	// Buildings
	ReadTag( _input, "Buildings" ); 
	int buildingsSize;
	ReadNetworkValue( _input, buildingsSize );
	DArray<IFrameEntity *> &buildings = _world.m_buildings;

	for( int b = 0; b < buildingsSize; ++b )
	{
		bool validBuildingIndex;
		ReadNetworkValue( _input, validBuildingIndex );

		if( validBuildingIndex )
		{
			IFrameEntity *building = new IFrameEntity;
			buildings.PutData( building, b );

			ReadNetworkValue( _input, building->m_pos );
			ReadNetworkValue( _input, building->m_vel );
            ReadNetworkValue( _input, building->m_radius );

			int uniqueId;
			ReadNetworkValue( _input, uniqueId );
			building->m_id = WorldObjectId( -1, -1, -1, uniqueId );

            ReadNetworkValue( _input, building->m_front );
            ReadNetworkValue( _input, building->m_up );
            ReadNetworkValue( _input, building->m_centre );
        }
	}

	// Random value
	ReadTag( _input, "RandValue" ); 

	double syncRand;
	ReadNetworkValue( _input, syncRand );
	_world.m_syncRand = syncRand;
}


void IFrame::Generate()
{
	Location *location = g_app->m_location;

	if (!location)
		return;

#if defined( TARGET_DEBUG ) || defined( TARGET_BETATEST_GROUP )

	std::ostrstream output;

	WriteNetworkValue( output, g_lastProcessedSequenceId );
	WriteNetworkValue( output, (double) GetNetworkTime() );
	
	// Teams
	WriteTag( output, "Teams" );
    
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = location->m_teams[t];

		bool teamUsed = team->m_teamType != TeamTypeUnused;
		WriteNetworkValue( output, teamUsed );	
		
        if( teamUsed )
        {
			FastDArray  <Unit *> &units = team->m_units;
			int unitsSize = units.Size();
			WriteNetworkValue( output, unitsSize );

            for( int u = 0; u < unitsSize; ++u )
            {
				bool validUnitIndex = units.ValidIndex(u);
				WriteNetworkValue( output, validUnitIndex );
				
                if( validUnitIndex )
                {
                    Unit *unit = units[u];

					WriteNetworkValue( output, unit->m_centrePos );

					int entitiesSize = unit->m_entities.Size();
					WriteNetworkValue( output, entitiesSize );

                    for( int e = 0; e < entitiesSize; ++e )
                    {
						bool validEntityIndex = unit->m_entities.ValidIndex(e);
						WriteNetworkValue( output, validEntityIndex );

                        if( validEntityIndex )
                        {
                            Entity *ent = unit->m_entities[e];

							WriteNetworkValue( output, ent->m_pos ); 
							WriteNetworkValue( output, ent->m_vel );
                            WriteNetworkValue( output, ent->m_front );
							WriteNetworkValue( output, ent->m_id.GetUniqueId() );
                        }
                    }
                }
            }

			SliceDArray <Entity *> &others = team->m_others;
			int othersSize = others.Size();
			WriteNetworkValue( output, othersSize );

            for( int e = 0; e < othersSize; ++e )
            {
				bool validOtherIndex = others.ValidIndex(e);

				WriteNetworkValue( output, validOtherIndex );

                if( validOtherIndex )
                {
                    Entity *entity = others[e];

					WriteNetworkValue( output, entity->m_pos );
					WriteNetworkValue( output, entity->m_vel );
                    WriteNetworkValue( output, entity->m_front );
					WriteNetworkValue( output, entity->m_id.GetUniqueId());
                }
            }
        }
    }

	// Lasers
	WriteTag( output, "Lasers" );

    SliceDArray <Laser> &lasers = location->m_lasers;
	int lasersSize = lasers.Size();
	WriteNetworkValue( output, lasersSize );

    for( int l = 0; l < lasersSize; ++l )
    {
		bool validLaserIndex = lasers.ValidIndex(l);
		WriteNetworkValue( output, validLaserIndex );

        if( validLaserIndex )
        {
            Laser *laser = lasers.GetPointer(l);
			
			WriteNetworkValue( output, laser->m_pos );
			WriteNetworkValue( output, laser->m_vel );
        }
    }
    
	// Effects
	WriteTag( output, "Effects" );
    
    SliceDArray <WorldObject *> &effects = location->m_effects;
	int effectsSize = effects.Size();
	WriteNetworkValue( output, effectsSize );

    for( int e = 0; e < effectsSize; ++e )
    {
		bool validEffectIndex = effects.ValidIndex(e);
		WriteNetworkValue( output, validEffectIndex );

        if( validEffectIndex )
        {
            WorldObject *wobj = effects[e];

			WriteNetworkValue( output, wobj->m_pos );
			WriteNetworkValue( output, wobj->m_vel );
        }
    }

	// Buildings
	WriteTag( output, "Buildings" );

	SliceDArray <Building *> &buildings = location->m_buildings;
	int buildingsSize = buildings.Size();
	WriteNetworkValue( output, buildingsSize );

	for( int b = 0; b < buildingsSize; ++b )
	{
		bool validBuildingIndex = buildings.ValidIndex(b);
		WriteNetworkValue( output, validBuildingIndex );

		if( validBuildingIndex )
		{
			Building *building = buildings[b];

			WriteNetworkValue( output, building->m_pos );
			WriteNetworkValue( output, building->m_vel );
            WriteNetworkValue( output, building->m_radius );
			WriteNetworkValue( output, building->m_id.GetUniqueId() );

            if( building->m_type != Building::TypeCrate )
            {
                WriteNetworkValue( output, building->m_front );
                WriteNetworkValue( output, building->m_up );
                WriteNetworkValue( output, building->m_centrePos );
            }
            else
            {
                WriteNetworkValue( output, g_zeroVector );
                WriteNetworkValue( output, g_zeroVector );
                WriteNetworkValue( output, g_zeroVector );
            }
		}
	}

    //
    // Random value
	WriteTag( output, "RandValue" );
    
	double syncRand = syncfrand(255);
	WriteNetworkValue( output, syncRand );

	m_length = output.tellp();
	m_data = output.str();	

	CalculateHashValue();

#ifdef CHECK_IFRAME_SERIALISATION
	// Check
	std::istrstream input(m_data, m_length);
	IFrameWorld world;
	ReadIFrameData(input, world);
#endif

#else

	m_hashValue = 255 * syncfrand(1);

#endif
}


template<class T>
int DiffDArray( DArray<T> &_a, DArray<T> &_b, LList<int> &_indexesOnlyInA, LList<int> &_indexesOnlyInB, LList<int> &_indexesDifferent )
{
	int aSize = _a.Size();
	int bSize = _b.Size();
	int commonSize = std::min( aSize, bSize );

	for( int i = 0; i < commonSize; i++ )
	{
		if( _a.ValidIndex( i ) && !_b.ValidIndex( i ) )
			_indexesOnlyInA.PutDataAtEnd( i );
		else if ( !_a.ValidIndex( i ) && _b.ValidIndex( i ) )
			_indexesOnlyInB.PutDataAtEnd( i );
		else if ( _a.ValidIndex( i ) && _b.ValidIndex( i ) && 
			Different( *_a.GetData( i ), *_b.GetData( i ) ) )
			_indexesDifferent.PutDataAtEnd( i );
		// (else the element is not in either array)
	}
	
	for( int i = commonSize; i < aSize; i++)
		if( _a.ValidIndex( i ))
			_indexesOnlyInA.PutDataAtEnd( i );

	for( int i = commonSize; i < bSize; i++)
		if( _b.ValidIndex( i ))
			_indexesOnlyInB.PutDataAtEnd( i );

	return _indexesDifferent.Size() + _indexesOnlyInA.Size() + _indexesOnlyInB.Size();
}


void CompareEntities( IFrameEntity *_a, IFrameEntity *_b, char *_description )
{
	sprintf( _description, "%s%s%s%s%s%s",	
		(_a->m_pos != _b->m_pos ? "[POS]" : ""),
		(_a->m_vel != _b->m_vel ? "[VEL]" : ""),
        (_a->m_front != _b->m_front ? "[FRONT]" : ""),
        (_a->m_up != _b->m_up ? "[UP]" : ""),
        (_a->m_centre != _b->m_centre ? "[CENTRE]" : ""),
        (_a->m_radius != _b->m_radius ? "[RADIUS]" : "") );		
}


void CompareUnits( IFrameUnit *_a, IFrameUnit *_b, int _numEntityDiffs, char *_description )
{
	int n = sprintf( _description, "%s",
		(_a->m_centrePos != _b->m_centrePos ? "[CENTREPOS]" : "") );

	if (n != -1) 
		_description += n;
	
	if( _numEntityDiffs )
		sprintf( _description, "[%d ENTITY DIFFS]", _numEntityDiffs );	
}


int DiffEntities( const char *_name, DArray<IFrameEntity *> &_a, DArray<IFrameEntity *> &_b, const RGBAColour &_colour, LList<SyncDiff *> &_differences )
{
	LList<int> onlyInA, onlyInB, different;

	int numDiffs = DiffDArray( _a, _b, onlyInA, onlyInB, different );	

	for( int e = 0; e < onlyInA.Size(); e++ )
	{
		IFrameEntity *entity = _a[ onlyInA.GetData(e) ];
		_differences.PutDataAtEnd( new SyncDiff( entity->m_id, entity->m_pos, _colour, _name, "Local only" ) );
	}

	for( int e = 0; e < onlyInB.Size(); e++ )
	{
		IFrameEntity *entity = _b[ onlyInB.GetData(e) ];
		_differences.PutDataAtEnd( new SyncDiff( entity->m_id, entity->m_pos, _colour, _name, "Remote only" ) );
	}

	for( int e = 0; e < different.Size(); e++ )
	{
		IFrameEntity *entityA = _a[ different.GetData(e) ];
		IFrameEntity *entityB = _b[ different.GetData(e) ];

		char description[1024];
		CompareEntities( entityA, entityB, description );

		_differences.PutDataAtEnd( new SyncDiff( entityA->m_id, entityA->m_pos, _colour, _name, description ) );
	}

	return numDiffs;
}


int DiffUnits( const char *_name, DArray<IFrameUnit *> &_a, DArray<IFrameUnit *> &_b, const RGBAColour &_colour, LList<SyncDiff *> &_differences )
{
	LList<int> onlyInA, onlyInB, different;

	int numDiffs = DiffDArray( _a, _b, onlyInA, onlyInB, different );	

	for( int u = 0; u < onlyInA.Size(); u++ )
	{
		IFrameUnit *unit = _a[ onlyInA.GetData(u) ];
		_differences.PutDataAtEnd( new SyncDiff( unit->m_id, unit->m_centrePos, _colour, _name, "Local only" ) );
	}

	for( int u = 0; u < onlyInB.Size(); u++ )
	{
		IFrameUnit *unit = _b[ onlyInB.GetData(u) ];
		_differences.PutDataAtEnd( new SyncDiff( unit->m_id, unit->m_centrePos, _colour, _name, "Remote only" ) );
	}

	for( int e = 0; e < different.Size(); e++ )
	{
		IFrameUnit *unitA = _a[ different.GetData(e) ];
		IFrameUnit *unitB = _b[ different.GetData(e) ];

		char name[64];
		sprintf(name, "Entity[%s]", _name );
		int numEntityDiffs = DiffEntities( name, unitA->m_entities, unitB->m_entities, _colour, _differences );

		char description[1024];
		CompareUnits( unitA, unitB, numEntityDiffs, description );
		_differences.PutDataAtEnd( new SyncDiff( unitA->m_id, unitA->m_centrePos, _colour, _name, description ) );
	}

	return numDiffs;
}


void IFrame::CalculateDiff( const IFrame &_other, const RGBAColour &_otherColour, LList<SyncDiff *> &_differences )
{	
	std::istrstream aInput(m_data, m_length), bInput(_other.m_data, _other.m_length);
	IFrameWorld a, b;
	
	ReadIFrameData( aInput, a );
	ReadIFrameData( bInput, b );

	AppReleaseAssert(a.m_sequenceId == b.m_sequenceId, "IFrame::CalculateDiff - different sequence ids");
	// AppReleaseAssert( aInput.eof(), "Failed to completely read iframe a" );
	// AppReleaseAssert( bInput.eof(), "Failed to completely read iframe b" );

	for( int t = 0; t < NUM_TEAMS; ++t )
	{
		char name[32];

		sprintf( name, "Unit[%d]", t );
		DiffUnits( name, a.m_team[t].m_units, b.m_team[t].m_units, _otherColour, _differences );

		sprintf( name, "Other[%d]", t );
		DiffEntities( name, a.m_team[t].m_others, b.m_team[t].m_others, _otherColour, _differences );
	}

	DiffEntities( "Laser",  a.m_lasers,  b.m_lasers,  _otherColour, _differences );
	DiffEntities( "Effect", a.m_effects, b.m_effects, _otherColour, _differences );
	DiffEntities( "Building", a.m_buildings, b.m_buildings, _otherColour, _differences );
	
	if( a.m_networkTime != b.m_networkTime )
	{
		_differences.PutDataAtEnd( new SyncDiff( WorldObjectId(), Vector3( 0, 0, 100 ), g_colourWhite, "Network Time", "Different" ) );
	}

	if (a.m_syncRand != b.m_syncRand)
	{
		_differences.PutDataAtEnd( new SyncDiff( WorldObjectId(), g_zeroVector, g_colourWhite, "Sync Rand", "Different" ) );
	}
}


void IFrame::CalculateHashValue()
{
	hash_context c;

    hash_initial( &c );

	// Unfortunately for us, our SHA1 implementation modifies the data it 
	// processes, so we take a copy.

	char *scratch = new char[m_length];
	
	memcpy( scratch, m_data, m_length );

	hash_process( &c, (unsigned char *) scratch, m_length );
	uint32 hashResult[5];
	hash_final( &c, hashResult );

	m_hashValue = hashResult[0] & 0xFF;
	m_sequenceId = g_lastProcessedSequenceId;

	static int maxIFrameLength = 0;
	if (m_length > maxIFrameLength) 
	{
		maxIFrameLength = m_length;
		//AppDebugOut("Biggest IFrame so far: %d bytes\n", m_length);
	}

	delete[] scratch;
}

void DumpSyncDiffs( LList<SyncDiff *> &_differences )
{
	char filename[512];
	g_app->m_clientToServer->GetSyncFileName( "syncdiff", filename, sizeof(filename) );

	std::ofstream o( filename );
	if( o )
	{
		for( int i = 0; i < _differences.Size(); i++ )
		{
			_differences[i]->Print( o );
		}
		o.close();
	}
}

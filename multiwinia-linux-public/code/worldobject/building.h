#ifndef _included_building_h
#define _included_building_h

#include "lib/vector3.h"
#include "lib/matrix34.h"
#include "lib/tosser/llist.h"

#include "worldobject/entity.h"
#include "worldobject/worldobject.h"

#include "globals.h"

#include "lib/unicode/unicode_string.h"

#include <vector>

class Shape;
class ShapeFragment;
class ShapeMarker;
class TextReader;
class BuildingPort;
class TextWriter;


class Building: public WorldObject
{
public:
    enum
    {
        TypeInvalid,		// When you add an entry here remember to update building.cpp
        TypeFactory,		// 1	
        TypeCave,			// 2
        TypeRadarDish,		
        TypeLaserFence,
        TypeControlTower,
        TypeGunTurret,
        TypeBridge,		
		TypePowerstation,
        TypeTree,		
        TypeWall,
        TypeTrunkPort,	
        TypeResearchItem,
        TypeLibrary,	
        TypeGenerator,
        TypePylon,		
        TypePylonStart,
        TypePylonEnd,
        TypeSolarPanel,
        TypePowerSplitter,
        TypeTrackLink,
        TypeTrackJunction,
        TypeTrackStart,
        TypeTrackEnd,		
        TypeRefinery,       
        TypeMine,			
        TypeYard,			
        TypeDisplayScreen,	
		TypeUpgradePort,	
        TypePrimaryUpgradePort,
        TypeIncubator,		
        TypeAntHill,
        TypeSafeArea,
        TypeTriffid,
        TypeSpiritReceiver,
        TypeReceiverLink,
        TypeReceiverSpiritSpawner,
        TypeSpiritProcessor,        
        TypeSpawnPoint,
        TypeSpawnPopulationLock,
        TypeSpawnPointMaster,
        TypeSpawnLink,
        TypeAITarget,
        TypeAISpawnPoint,
        TypeBlueprintStore,
        TypeBlueprintConsole,
        TypeBlueprintRelay,
        TypeScriptTrigger,
        TypeSpam,
        TypeGodDish,
        TypeStaticShape,
        TypeFuelGenerator,
        TypeFuelPipe,
        TypeFuelStation,
        TypeEscapeRocket,
        TypeFenceSwitch,
        TypeDynamicHub,
        TypeDynamicNode,
        TypeFeedingTube,
        TypeMultiwiniaZone,
        TypeChessPiece,
        TypeChessBase,
        TypeCloneLab,
		TypeControlStation,
        TypePortal,
        TypeCrate,
        TypeStatue,
        TypeWallBuster,
        TypePulseBomb,
        TypeRestrictionZone,
        TypeJumpPad,
        TypeAIObjective,
        TypeAIObjectiveMarker,
        TypeEruptionMarker,
        TypeSmokeMarker,
		NumBuildingTypes		
    };
    
    Vector3     m_front;
    Vector3     m_up;
	double		m_timeOfDeath;
	bool		m_dynamic;								// Only appears on this level, not all levels for this map
	bool		m_isGlobal;
    Vector3     m_centrePos;
    double       m_radius;

	bool		m_destroyed;							// Building has been destroyed using the script command DestroyBuilding, remove it next Advance
	
    Shape		*m_shape;
    LList       <ShapeMarker *> m_lights;               // Ownership lights
    LList       <BuildingPort *> m_ports;               // Require Darwinians in them to operate

    static Shape        *s_controlPad;
    static ShapeMarker  *s_controlPadStatus;

protected:
	static std::vector<WorldObjectId> s_neighbours;

public:
    Building();
	~Building();
    
    virtual void Initialise( Building *_template );
    virtual bool Advance();
    
    virtual void SetShape       ( Shape *_shape );
    void         SetShapeLights ( ShapeFragment *_fragment );   // Recursivly search for lights
    void         SetShapePorts  ( ShapeFragment *_fragment );

    virtual void SetDetail      ( int _detail );

    virtual bool IsInView       ();

    virtual void Render         ( double predictionTime );        
    virtual void RenderAlphas   ( double predictionTime );    
    virtual void RenderLights   ();
    virtual void RenderPorts    ();
	virtual void RenderHitCheck ();
    virtual void RenderLink     ();                             // ie link to another building

    virtual bool PerformDepthSort( Vector3 &_centrePos );       // Return true if you plan to use transparencies

    virtual void SetTeamId          ( int _teamId );
    virtual void Reprogram          ( double _complete );
    virtual void ReprogramComplete  ();
    
    virtual void Damage( double _damage );
	virtual void Destroy( double _intensity );

    Vector3 PushFromBuilding( Vector3 const &_pos, double _radius );
    
    virtual void            EvaluatePorts          ();
    virtual int             GetNumPorts            ();
    virtual int             GetNumPortsOccupied    ();
    virtual WorldObjectId   GetPortOccupant        ( int _portId );
    virtual bool            GetPortPosition        ( int _portId, Vector3 &_pos, Vector3 &_front );
    
    virtual void            OperatePort            ( int _portId, int _teamId );    
    virtual int             GetPortOperatorCount   ( int _portId, int _teamId );

    virtual void            GetObjectiveCounter   (UnicodeString& _dest);

    virtual bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    virtual bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    virtual bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                        double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    virtual void ListSoundEvents        ( LList<char *> *_list );
    
    virtual void Read   ( TextReader *_in, bool _dynamic );     // Use these to read/write additional building-specific
    virtual void Write  ( TextWriter *_out );					// data to the level files
    
    virtual int  GetBuildingLink();                             // Allows a building to link to another
    virtual void SetBuildingLink( int _buildingId );            // eg control towers

    static char *GetTypeName    ( int _type );
    static int   GetTypeId	    ( char const *_name );
    static Building *CreateBuilding ( int _type );
    static Building *CreateBuilding ( char *_name );

    void GetTypeNameTranslated( int _type, UnicodeString& _dest );
	char		*LogState( char *_message = NULL );

    static bool BuildingIsBlocked( int _buildingId );
};



class BuildingPort
{
public:
    ShapeMarker     *m_marker;
    WorldObjectId   m_occupant;
    Matrix34        m_mat;
    int             m_counter[NUM_TEAMS];
};

#endif

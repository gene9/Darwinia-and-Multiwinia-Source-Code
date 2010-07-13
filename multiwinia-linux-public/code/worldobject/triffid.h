
#ifndef _included_triffid_h
#define _included_triffid_h

#include "worldobject/building.h"


class Triffid : public Building
{
protected:
    ShapeMarker     *m_launchPoint;
    ShapeMarker     *m_stem;
    double           m_timerSync;
    double           m_damage;
    bool            m_triggered;
    double           m_triggerTimer;
    bool            m_renderDamaged;

    int             m_numEggs;
    
public:
    enum
    {
        SpawnVirii,
        SpawnCentipede,
        SpawnSpider,
        SpawnSpirits,
        SpawnEggs,
        SpawnTriffidEggs,
        SpawnAnts,
        SpawnSporeGenerator,
        SpawnSoulDestroyer,
        SpawnDarwinians,
        NumSpawnTypes
    };

    bool            m_spawn[NumSpawnTypes];
    double           m_size;
    double           m_reloadTime;
    double           m_pitch;                    
    double           m_force;
    double           m_variance;                 // Horizontal 

    int             m_useTrigger;               // Num enemies required to trigger
    Vector3         m_triggerLocation;          // Offset from m_pos
    double           m_triggerRadius;
    
    Matrix34        GetHead( double _predictionTime );                 // So to speak

public:
    Triffid();
    
    void Initialise     ( Building *_template );
    bool Advance        ();
    void Launch         ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void Damage         ( double _damage );

    void ListSoundEvents( LList<char *> *_list );

    bool DoesRayHit     (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                         double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    static char *GetSpawnName( int _spawnType );
    static void GetSpawnNameTranslated( int _spawnType, UnicodeString &_dest );
    
    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
};


// ============================================================================
// Triffid Egg

#define TRIFFIDEGG_BOUNCEFRICTION 0.65

class TriffidEgg : public Entity
{    
protected:
    Vector3     m_up;
    double       m_force;
    double       m_timerSync;
    double       m_life;
    
public:
    double       m_size;
    int         m_spawnType;
    Vector3     m_spawnPoint;
    double       m_spawnRange;
    
public:
    TriffidEgg();
    void Begin();

    bool ChangeHealth       ( int _amount, int _damageType );
    void Spawn              ();
    bool Advance            ( Unit *_unit );
    void Render             ( double _predictionTime );
    bool RenderPixelEffect  ( double _predictionTime );

	void ResetTimer			();
    void SetTimer           ( double _time );

    void ListSoundEvents    ( LList<char *> *_list );
};


#endif

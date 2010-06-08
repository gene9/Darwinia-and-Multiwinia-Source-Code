
#ifndef _included_triffid_h
#define _included_triffid_h

#include "worldobject/building.h"


class Triffid : public Building
{
protected:
    ShapeMarker     *m_launchPoint;
    ShapeMarker     *m_stem;
    float           m_timerSync;
    float           m_damage;
    bool            m_triggered;
    float           m_triggerTimer;
    bool            m_renderDamaged;

public:
    enum
    {
        SpawnVirii,
        SpawnCentipede,
        SpawnSpider,
        SpawnSpirits,
        SpawnEggs,
        SpawnTriffidEggs,
        SpawnDarwinians,
        NumSpawnTypes
    };

    bool            m_spawn[NumSpawnTypes];
    float           m_size;
    float           m_reloadTime;
    float           m_pitch;
    float           m_force;
    float           m_variance;                 // Horizontal

    int             m_useTrigger;               // Num enemies required to trigger
    Vector3         m_triggerLocation;          // Offset from m_pos
    float           m_triggerRadius;

    Matrix34        GetHead();                  // So to speak

public:
    Triffid();

    void Initialise     ( Building *_template );
    bool Advance        ();
    void Launch         ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    void Damage         ( float _damage );

    void ListSoundEvents( LList<char *> *_list );

    bool DoesRayHit     (Vector3 const &_rayStart, Vector3 const &_rayDir,
                         float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    static char *GetSpawnName( int _spawnType );
    static char *GetSpawnNameTranslated( int _spawnType );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
};


// ============================================================================
// Triffid Egg

#define TRIFFIDEGG_BOUNCEFRICTION 0.65f

class TriffidEgg : public Entity
{
protected:
    Vector3     m_up;
    float       m_force;
    float       m_timerSync;
    float       m_life;

public:
    float       m_size;
    int         m_spawnType;
    Vector3     m_spawnPoint;
    float       m_spawnRange;

public:
    TriffidEgg();

    void ChangeHealth       ( int _amount );
    void Spawn              ();
    bool Advance            ( Unit *_unit );
    void Render             ( float _predictionTime );
    bool RenderPixelEffect  ( float _predictionTime );

    void ListSoundEvents    ( LList<char *> *_list );
};


#endif
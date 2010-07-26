
#ifndef _included_incubator_h
#define _included_incubator_h

#include "lib/fast_darray.h"

#include "worldobject/building.h"
#include "worldobject/spirit.h"

class ShapeMarker;

#define INCUBATOR_PROCESSTIME       5.0f

struct IncubatorIncoming
{
    Vector3 m_pos;
    int     m_entrance;
    float   m_alpha;
};


class Incubator: public Building
{
protected:
    FastDArray      <Spirit> m_spirits;
    ShapeMarker     *m_spiritCentre;
    ShapeMarker     *m_exit;
    ShapeMarker     *m_dock;
    ShapeMarker     *m_spiritEntrance[3];

    int             m_troopType;
    float           m_timer;

    LList           <IncubatorIncoming *> m_incoming;

public:
    int             m_numStartingSpirits;
	bool			m_teamSpawner;

public:
    Incubator();
    ~Incubator();

    void Initialise ( Building *_template );

    bool Advance    ();
    void SpawnEntity();
    void AddSpirit  ( Spirit *_spirit );

    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    int  NumSpiritsInside();

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

    void GetDockPoint( Vector3 &_pos, Vector3 &_front );
    void GetExitPoint( Vector3 &_pos, Vector3 &_front );

	void ListSoundEvents( LList<char *> *_list );

	int m_renderDamaged;
};


#endif

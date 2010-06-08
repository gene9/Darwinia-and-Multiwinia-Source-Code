
#ifndef _included_trunkport_h
#define _included_trunkport_h

#include "worldobject/building.h"

#define TRUNKPORT_HEIGHTMAP_MAXSIZE 16


class TrunkPort : public Building
{
public:
    int m_targetLocationId;
    
    ShapeMarker *m_destination1;
    ShapeMarker *m_destination2;
    
    int     m_heightMapSize;
    Vector3 *m_heightMap;
    float   m_openTimer;
    
public:
    TrunkPort();

    void Initialise         ( Building *_template );
    void SetDetail          ( int _detail );
    bool Advance            ();
    void Render             ( float predictionTime );
    void RenderAlphas       ( float predictionTime );       
    
    bool PerformDepthSort   ( Vector3 &_centrePos );     

    void ReprogramComplete();

    void ListSoundEvents    ( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
};



#endif

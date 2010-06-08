
#ifndef _included_scripttrigger_h
#define _included_scripttrigger_h

#include "worldobject/building.h"

#define SCRIPTRIGGER_RUNALWAYS              999
#define SCRIPTRIGGER_RUNNEVER               998
#define SCRIPTRIGGER_RUNCAMENTER            997
#define SCRIPTRIGGER_RUNCAMVIEW             996


class ScriptTrigger : public Building
{
public:
    char    m_scriptFilename[256];
    float   m_range;
    int     m_entityType;
    int     m_linkId;
    
protected:
    int     m_triggered;
    float   m_timer;
    
public:
    ScriptTrigger();

    void Initialise     ( Building *_template );
    bool Advance        ();
    void RenderAlphas   ( float predictionTime );        

    void Trigger();

    bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            

    void Read       ( TextReader *_in, bool _dynamic );     
    void Write      ( FileWriter *_out );							    
};


#endif
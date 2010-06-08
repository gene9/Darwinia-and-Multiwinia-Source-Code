
#ifndef _included_mine_h
#define _included_mine_h

#include "worldobject/building.h"

class Refinery;
class MineCart;


// ****************************************************************************
// Class MineBuilding
// ****************************************************************************

class MineBuilding : public Building
{
protected:
    int             m_trackLink;
    ShapeMarker     *m_trackMarker1;
    ShapeMarker     *m_trackMarker2;

    Matrix34        m_trackMatrix1;
    Matrix34        m_trackMatrix2;

    LList           <MineCart *> m_carts;

    float           m_previousMineSpeed;
    float           m_wheelRotate;
    
    static Shape        *s_wheelShape;
    static Shape        *s_cartShape;
    static ShapeMarker  *s_cartMarker1;
    static ShapeMarker  *s_cartMarker2;
    static ShapeMarker  *s_cartContents[3];
    static Shape        *s_polygon1;
    static Shape        *s_primitive1;
    
    static float        s_refineryPopulation;
    static float        s_refineryRecalculateTimer;
    static float        RefinerySpeed();
                
public:
    MineBuilding();

    void Initialise     ( Building *_template );    
    bool Advance        ();

    bool IsInView       ();

    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );
    void RenderCart     ( MineCart *_cart, float _predictionTime );

    Vector3     GetTrackMarker1();
    Vector3     GetTrackMarker2();
    
    virtual void TriggerCart ( MineCart *_cart, float _initValue );
    
    void ListSoundEvents    ( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            

};


class MineCart
{
public:
    float   m_progress;                     // Progress down current line, 0.0f - 1.0f

    bool    m_polygons[3];
    bool    m_primitives[3];
    
public:
    MineCart();
};


// ****************************************************************************
// Class TrackLink
// ****************************************************************************

class TrackLink : public MineBuilding
{
public:
    TrackLink();

    bool Advance();
};


// ****************************************************************************
// Class TrackJunction
// ****************************************************************************

class TrackJunction : public MineBuilding
{
public:
    LList<int>      m_trackLinks;

public:
    TrackJunction();

    void Initialise     ( Building *_template );    

    void Render         ( float _predictionTime );
    void TriggerCart    ( MineCart *_cart, float _initValue );

    void RenderLink     ();
    void SetBuildingLink( int _buildingId );            

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
};


// ****************************************************************************
// Class TrackStart
// ****************************************************************************

class TrackStart : public MineBuilding
{
public:
    int m_reqBuildingId;                   // This building must be online
    
public:    
    TrackStart();

    void Initialise     ( Building *_template );
    bool Advance        ();    
    void RenderAlphas   ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
};


// ****************************************************************************
// Class TrackEnd
// ****************************************************************************

class TrackEnd : public MineBuilding
{
public:
    int m_reqBuildingId;                   // This building must be online
    
public:
    TrackEnd();

    void Initialise     ( Building *_template );
    bool Advance();

    void RenderAlphas   ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
};


// ****************************************************************************
// Class Refinery
// ****************************************************************************

class Refinery : public MineBuilding
{
protected:
    ShapeMarker *m_wheel1;
    ShapeMarker *m_wheel2;
    ShapeMarker *m_wheel3;
    ShapeMarker *m_counter1;
    
public:
    Refinery();

    bool Advance();
    void Render         ( float _predictionTime );

    char *GetObjectiveCounter();

    void TriggerCart    ( MineCart *_cart, float _initValue );
};


// ****************************************************************************
// Class Mine
// ****************************************************************************

class Mine : public MineBuilding
{
protected:
    ShapeMarker *m_wheel1;
    ShapeMarker *m_wheel2;
    
public:
    Mine();

    void Render         ( float _predictionTime );
    
    void TriggerCart    ( MineCart *_cart, float _initValue );
};

#endif

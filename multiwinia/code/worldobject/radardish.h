#ifndef _included_radardish_h
#define _included_radardish_h

#define RADARDISH_TRANSPORTPERIOD    0.1                        // Minimum wait time between sends
#define RADARDISH_TRANSPORTSPEED     50.0                       // Speed of in-transit entities (m/s)

#include "worldobject/teleport.h"
#include "worldobject/ai.h"


class RadarDish : public Teleport
{
protected:
	ShapeFragment *m_dish;
	ShapeFragment *m_upperMount;
	ShapeMarker	  *m_focusMarker;

    Vector3     m_entrancePos;
    Vector3     m_entranceFront;
    
    Vector3     m_target;    
    int         m_receiverId;
    double       m_range;
    double       m_signal;

    bool        m_newlyCreated;
    
    bool        m_horizontallyAligned;
    bool        m_verticallyAligned;
    bool        m_movementSoundsPlaying;

    int         m_oldTeamId;

    DArray<int> m_validLinkList;

    void RenderSignal   ( double _predictionTime, double _radius, double _alpha );

public:

	AITarget	*m_aiTarget;
    int         m_forceConnection;
    int         m_autoConnectAtStart;
    bool         m_forceTeamMatch;

    RadarDish();
	~RadarDish();

    void SetDetail      ( int _detail );
    void Initialise     ( Building *_template );

    bool Advance        ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void Aim            ( Vector3 _worldPos );

    bool    Connected       ();
    bool    ReadyToSend     ();

    int     GetConnectedDishId();

    Vector3 GetStartPoint   ();
    Vector3 GetEndPoint     ();
    bool    GetEntrance     ( Vector3 &_pos, Vector3 &_front );
    bool    GetExit         ( Vector3 &_pos, Vector3 &_front );

	Vector3 GetDishFront    ( double _predictionTime );      // Returns the front vector of the dish
	Vector3 GetDishPos      ( double _predictionTime );      // Returns the position of the transmission point

    bool    DoesSphereHit   (Vector3 const &_pos, double _radius);
    bool    DoesRayHit      (Vector3 const &rayStart, Vector3 const &rayDir, 
                             float rayLen=1e10, Vector3 *pos=NULL, Vector3 *norm=NULL );

    bool    UpdateEntityInTransit( Entity *_entity );

    void    ListSoundEvents ( LList<char *> *_list );

	void	AdvanceAITarget();

    void    Read           ( TextReader *_in, bool _dynamic );     
    void    Write          ( TextWriter *_out );

    void    SetBuildingLink( int _buildingId );
    void    ClearLinks();
    int     CountValidLinks();
    void    AddValidLink( int _id );
    bool    ValidReceiverDish( int _buildingId );
    bool    NotAligned();
};



#endif

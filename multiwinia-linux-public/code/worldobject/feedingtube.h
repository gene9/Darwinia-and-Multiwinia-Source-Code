#ifndef _included_feedingtube_h
#define _included_feedingtube_h


class FeedingTube : public Building
{
protected:	
	ShapeMarker	  *m_focusMarker;

    int         m_receiverId;
    double       m_range;
    double       m_signal;
    
    Vector3     GetDishPos      ( double _predictionTime );      // Returns the position of the transmission point
    Vector3     GetDishFront    ( double _predictionTime );      // Returns the front vector of the dish
	Vector3		GetForwardsClippingDir( double _predictionTime, FeedingTube *_sender );// Returns a good clipping direction for signal

    void RenderSignal   ( double _predictionTime, double _radius, double _alpha );


public:
    FeedingTube();

    void Initialise( Building *_template );
    void Read( TextReader *_in, bool _dynamic );
    void Write( TextWriter *_out );

    bool Advance        ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    Vector3 GetStartPoint   ();
    Vector3 GetEndPoint     ();

    int  GetBuildingLink();                 
    void SetBuildingLink(int _buildingId);

    bool    DoesSphereHit   (Vector3 const &_pos, double _radius);

    void    ListSoundEvents ( LList<char *> *_list );
    bool IsInView();
};



#endif

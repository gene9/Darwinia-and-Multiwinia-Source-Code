#ifndef _included_feedingtube_h
#define _included_feedingtube_h


class FeedingTube : public Building
{
protected:	
	ShapeMarker	  *m_focusMarker;

    int         m_receiverId;
    float       m_range;
    float       m_signal;
    
    Vector3     GetDishPos      ( float _predictionTime );      // Returns the position of the transmission point
    Vector3     GetDishFront    ( float _predictionTime );      // Returns the front vector of the dish
	Vector3		GetForwardsClippingDir( float _predictionTime, FeedingTube *_sender );// Returns a good clipping direction for signal

    void RenderSignal   ( float _predictionTime, float _radius, float _alpha );


public:
    FeedingTube();

    void Initialise( Building *_template );
    void Read( TextReader *_in, bool _dynamic );
    void Write( FileWriter *_out );

    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    Vector3 GetStartPoint   ();
    Vector3 GetEndPoint     ();

    int  GetBuildingLink();                 
    void SetBuildingLink(int _buildingId);

    bool    DoesSphereHit   (Vector3 const &_pos, float _radius);

    void    ListSoundEvents ( LList<char *> *_list );
    bool IsInView();
};



#endif

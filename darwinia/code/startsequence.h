
#ifndef _included_startsequence_h
#define _included_startsequence_h


class StartSequenceCaption;


class StartSequence
{
protected:
    float m_startTime;
    LList<StartSequenceCaption *> m_captions;

    void RegisterCaption( char *_caption, float _x, float _y, float _size,
                          float _startTime, float _endTime );

public:
    StartSequence();

    bool Advance();
    void Render();
};



class StartSequenceCaption
{
public:
    char *m_caption;
    float m_x;
    float m_y;
    float m_size;
    float m_startTime;
    float m_endTime;

};


#endif
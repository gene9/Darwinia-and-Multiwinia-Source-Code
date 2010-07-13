#ifndef _included_animatedpanel_h
#define _included_animatedpanel_h

#include "lib/tosser/darray.h"
#include "lib/tosser/llist.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"

class AnimatedPanelObject;
class AnimatedPanelKeyframe;


// ============================================================================
// AnimatedPanelObject


class AnimatedPanel
{
public:
    LList<AnimatedPanelObject *> m_objects;

    int     m_width;
    int     m_height;
    float   m_start;
    float   m_stop;

    float   m_time;

public:
    AnimatedPanel();
    ~AnimatedPanel();

    int     NewObject       ();
    void    DeleteObject    ( int objId );

    int     NewKeyframe     ( int objId, float timeIndex );
    int     GetKeyframe     ( int objId, float timeIndex, float maxTimeDiff );
    void    DeleteKeyframe  ( int objId, int keyframeId );
    
    bool CalculateObjectProperties( int objId, float timeIndex, AnimatedPanelKeyframe &result );
    
    void Read   ( TextReader *reader );
    void Write  ( TextFileWriter *writer );
};


// ============================================================================
// AnimatedPanelObject


class AnimatedPanelObject
{
public:
    LList<AnimatedPanelKeyframe *> m_keyframes;
    
public:
    ~AnimatedPanelObject();

    void Read   ( TextReader *reader );    
};



// ============================================================================
// AnimatedPanelKeyframe

class AnimatedPanelKeyframe
{
public:
    float m_timeIndex;
    char  m_imageName[256];
    float m_x;
    float m_y;
    int   m_colour;
    float m_scale;
    float m_angle;
    int   m_flipHoriz;
    int   m_alpha;

public:
    enum
    {
        ColourNone,
        ColourFriendly,
        ColourEnemy,
        ColourInvisible
    };

public:
    AnimatedPanelKeyframe();

    void Read   ( TextReader *reader );
};


#endif

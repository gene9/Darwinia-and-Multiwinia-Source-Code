#include "lib/universal_include.h"

#include "lib/render/renderer.h"

#include "animatedpanel.h"


AnimatedPanel::AnimatedPanel()
:   m_time(0.0f),
    m_width(600),
    m_height(300),
    m_start(0),
    m_stop(30)
{
}


AnimatedPanel::~AnimatedPanel()
{
    m_objects.EmptyAndDelete();
}

int AnimatedPanel::NewObject()
{
    AnimatedPanelObject *obj = new AnimatedPanelObject();
    m_objects.PutData( obj );
    return m_objects.Size()-1;    
}


void AnimatedPanel::DeleteObject( int objId )
{
    if( m_objects.ValidIndex(objId) )
    {
        AnimatedPanelObject *obj = m_objects[objId];
        m_objects.RemoveData(objId);
        delete obj;
    }
}


int AnimatedPanel::NewKeyframe( int objId, float timeIndex )
{
    if( !m_objects.ValidIndex(objId) )
    {
        return -1;
    }

    AnimatedPanelKeyframe keyframe;
    CalculateObjectProperties( objId, timeIndex, keyframe );

    AnimatedPanelObject *obj = m_objects[objId];
    AnimatedPanelKeyframe *kf = new AnimatedPanelKeyframe();
    *kf = keyframe;
    kf->m_timeIndex = timeIndex;
    obj->m_keyframes.PutData( kf );
    
    return obj->m_keyframes.Size()-1;
}


int AnimatedPanel::GetKeyframe( int objId, float timeIndex, float maxTimeDiff )
{
    if( !m_objects.ValidIndex(objId) )
    {
        return -1;
    }

    AnimatedPanelObject *obj = m_objects[objId];

    for( int j = 0; j < obj->m_keyframes.Size(); ++j )
    {
        AnimatedPanelKeyframe *kf = obj->m_keyframes[j];

        if( iv_abs( kf->m_timeIndex - timeIndex ) <= maxTimeDiff )
        {
            return j;
        }
    }

    return -1;
}


void AnimatedPanel::DeleteKeyframe( int objId, int keyframeId )
{
    if( m_objects.ValidIndex(objId) )
    {
        AnimatedPanelObject *obj = m_objects[objId];
        
        if( obj->m_keyframes.ValidIndex(keyframeId) )
        {
            obj->m_keyframes.RemoveData(keyframeId);
        }
    }
}


double CalculateRampUpAndDown(double _startTime, double _duration, double _timeNow)
{
    if (_timeNow > _startTime + _duration) return 1.0f;

    double fractionalTime = (_timeNow - _startTime) / _duration;

    if (fractionalTime < 0.5)
    {
        double t = fractionalTime * 2.0;
        t *= t;
        t *= 0.5;
        return t;
    }
    else
    {
        double t = (1.0 - fractionalTime) * 2.0;
        t *= t;
        t *= 0.5;
        t = 1.0 - t;
        return t;
    }
}


bool AnimatedPanel::CalculateObjectProperties( int objId, float timeIndex, AnimatedPanelKeyframe &result )
{
    //
    // Look for a keyframe to the left and one to the right

    AnimatedPanelKeyframe *left = NULL;
    AnimatedPanelKeyframe *right = NULL;

    AnimatedPanelObject *obj = m_objects[objId];

    for( int j = 0; j < obj->m_keyframes.Size(); ++j )
    {
        AnimatedPanelKeyframe *kf = obj->m_keyframes[j];

        if( kf->m_timeIndex <= timeIndex )
        {
            if( !left || kf->m_timeIndex > left->m_timeIndex ) left = kf;
        }

        if( kf->m_timeIndex >= timeIndex )
        {
            if( !right || kf->m_timeIndex < right->m_timeIndex ) right = kf;
        }
    }


    //
    // If we have no results, we are done

    if( !left && !right )
    {
        return false;
    }


    //
    // If we have only one result and it's too the left, return that

    if( left && !right )
    {
        result = *left;
        return true;
    }


    //
    // If we have only one result to the right, it means we don't exist yet

    if( right && !left )
    {
        return false;        
    }


    //
    // If our left and right are the same keyframe, or both times are the same, return one

    if( left && right && left == right )
    {
        result = *left;
        return true;
    }

    if( iv_abs(right->m_timeIndex - left->m_timeIndex) < 0.01f )
    {
        result = *left;
        return true;
    }


    //
    // Otherwise interpolate

    float linearFraction = (timeIndex - left->m_timeIndex) / (right->m_timeIndex - left->m_timeIndex);
    float rampedFraction = CalculateRampUpAndDown( left->m_timeIndex, (right->m_timeIndex - left->m_timeIndex), timeIndex );

    result = *left;
    
    result.m_x = left->m_x + ( right->m_x - left->m_x ) * rampedFraction;
    result.m_y = left->m_y + ( right->m_y - left->m_y ) * rampedFraction;
    result.m_scale = left->m_scale + ( right->m_scale - left->m_scale ) * rampedFraction;
    
    result.m_angle = left->m_angle + ( right->m_angle - left->m_angle ) * linearFraction;
    result.m_alpha = left->m_alpha + ( right->m_alpha - left->m_alpha ) * linearFraction;
    
    return true;
}


void AnimatedPanel::Write( TextFileWriter *writer )
{        
    writer->printf( "WIDTH      %d\n", m_width );
    writer->printf( "HEIGHT     %d\n", m_height );
    writer->printf( "START      %2.2f\n", m_start );
    writer->printf( "STOP       %2.2f\n", m_stop );
    writer->printf( "\n" );

    for( int i = 0; i < m_objects.Size(); ++i )
    {
        if( m_objects.ValidIndex(i) )
        {
            AnimatedPanelObject *obj = m_objects[i];
            
            writer->printf( "BEGIN Object\n" );
            
            for( int j = 0; j < obj->m_keyframes.Size(); ++j )
            {
                AnimatedPanelKeyframe *kf = obj->m_keyframes[j];
                
                writer->printf( "\tBEGIN Keyframe\n" );
                writer->printf( "\t\tTIMEINDEX       %2.2f\n", kf->m_timeIndex );
                writer->printf( "\t\tIMAGENAME       %s\n", kf->m_imageName );
                writer->printf( "\t\tXPOS            %2.2f\n", kf->m_x );
                writer->printf( "\t\tYPOS            %2.2f\n", kf->m_y );
                writer->printf( "\t\tCOLOUR          %d\n", kf->m_colour );
                writer->printf( "\t\tSCALE           %2.2f\n", kf->m_scale );
                writer->printf( "\t\tANGLE           %2.2f\n", kf->m_angle );
                writer->printf( "\t\tFLIPHORIZ       %d\n", kf->m_flipHoriz );
                writer->printf( "\t\tALPHA           %d\n", kf->m_alpha );
                writer->printf( "\tEND\n" );
            }
            
            writer->printf( "END\n\n" );
        }
    }
}


void AnimatedPanel::Read( TextReader *reader )
{
    while( reader->ReadLine() )
    {
        if( !reader->TokenAvailable() ) continue;
        
        char *command = reader->GetNextToken();
        char *data = reader->GetNextToken();
        
        if     ( stricmp( command, "WIDTH" ) == 0 )         m_width = atoi( data );
        else if( stricmp( command, "HEIGHT" ) == 0 )        m_height = atoi( data );
        else if( stricmp( command, "START" ) == 0 )         m_start = atof( data );
        else if( stricmp( command, "STOP" ) == 0 )          m_stop = atof( data );
        else if( stricmp( command, "BEGIN" ) == 0 )
        {
            if( stricmp( data, "Object" ) == 0 )
            {
                AnimatedPanelObject *obj = new AnimatedPanelObject();
                obj->Read( reader );
                m_objects.PutData( obj );
            }
        }
    }
}


AnimatedPanelObject::~AnimatedPanelObject()
{
    m_keyframes.EmptyAndDelete();
}


void AnimatedPanelObject::Read( TextReader *reader )
{
    while( reader->ReadLine() )
    {
        if( !reader->TokenAvailable() ) continue;
        
        char *command = reader->GetNextToken();
        char *data = ( reader->TokenAvailable() ? reader->GetNextToken() : NULL );

        if( stricmp( command, "BEGIN" ) == 0 )
        {
            if( stricmp( data, "Keyframe" ) == 0 )
            {
                AnimatedPanelKeyframe *kf = new AnimatedPanelKeyframe();
                kf->Read( reader );
                m_keyframes.PutData( kf );
            }
        }
        else if( stricmp( command, "END" ) == 0 )
        {
            break;
        }
    }
}


AnimatedPanelKeyframe::AnimatedPanelKeyframe()
:   m_timeIndex(0.0f),
    m_x(100),
    m_y(100),
    m_scale(0.3f),
    m_angle(0.0f),
    m_colour(ColourNone),
    m_alpha(255),
    m_flipHoriz(0)
{
    strcpy( m_imageName, "darwinian.bmp" );
}


void AnimatedPanelKeyframe::Read( TextReader *reader )
{
    while( reader->ReadLine() )
    {
        if( !reader->TokenAvailable() ) continue;
                
        char *command = reader->GetNextToken();
        char *data = ( reader->TokenAvailable() ? reader->GetNextToken() : NULL );

        if     ( stricmp( command, "TIMEINDEX" ) == 0 )         m_timeIndex = atof( data );
        else if( stricmp( command, "IMAGENAME" ) == 0 )         strcpy( m_imageName, data );
        else if( stricmp( command, "XPOS" ) == 0 )              m_x = atof( data );
        else if( stricmp( command, "YPOS" ) == 0 )              m_y = atof( data );
        else if( stricmp( command, "COLOUR" ) == 0 )            m_colour = atoi( data );
        else if( stricmp( command, "SCALE" ) == 0 )             m_scale = atof( data );
        else if( stricmp( command, "ANGLE" ) == 0 )             m_angle = atof( data );
        else if( stricmp( command, "FLIPHORIZ" ) == 0 )         m_flipHoriz = atoi( data );
        else if( stricmp( command, "ALPHA" ) == 0 )             m_alpha = atoi( data );        
        else if( stricmp( command, "END" ) == 0 )               break;
    }
}


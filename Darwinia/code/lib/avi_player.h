#ifndef _included_aviplayer_h
#define _included_aviplayer_h

#include <vfw.h>


class AviPlayer
{
protected:
    PAVIFILE            m_aviFile;
    AVIFILEINFO         m_aviFileInfo;

    PAVISTREAM          m_streamVideo;
    PGETFRAME           m_streamVideoFrame;
    BITMAPINFOHEADER    m_streamVideoInfo;

    unsigned char       *m_currentFrame;
    int                 m_previousFrameIndex;

    PAVISTREAM          m_streamAudio;
    PGETFRAME           m_streamAudioFrame;
    PCMWAVEFORMAT       m_streamAudioInfo;

    int                 m_nextAudioSample;

public:
    AviPlayer();
    ~AviPlayer();

    int OpenAvi( char *_filename );
    int CloseAvi();

    int DecodeVideoFrame( int _frameIndex );                //returns -1 if finished
    int DecodeVideoFrame( float _timeIndex );               //returns -1 if finished

    int GetWidth();
    int GetHeight();
    unsigned char *GetFrameData();

    int GetAudioData( int _numSamples, signed short *_buffer );
};



#endif

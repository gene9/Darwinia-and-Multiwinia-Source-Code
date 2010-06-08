#include "lib/universal_include.h"

#include <vfw.h>

#include "lib/avi_player.h"
#include "lib/debug_utils.h"


AviPlayer::AviPlayer()
:   m_currentFrame(NULL),
    m_nextAudioSample(0),
    m_previousFrameIndex(-1)
{
    AVIFileInit();
}


AviPlayer::~AviPlayer()
{
    AVIFileExit();
}


#define HandleAviError(_error) \
        if( _error != 0 ) \
        { \
            DebugOut( "Error with AVI playback: %d\n", _error ); \
            return -1; \
        }


int AviPlayer::OpenAvi( char *_filename )
{
    //
    // Open the avi file

	int result = AVIFileOpen(&m_aviFile,
					         _filename,
					         OF_READ,
					         NULL);

    HandleAviError(result);

    result = AVIFileInfo( m_aviFile, &m_aviFileInfo, sizeof(m_aviFileInfo) );

    HandleAviError(result);


    //
    // Open a video stream from the file

    result = AVIFileGetStream( m_aviFile,
                               &m_streamVideo,
                               streamtypeVIDEO,
                               0 );

    HandleAviError(result);



    //
    // Read the specs of our avi video stream
    // Allocate space for one video frame

    long sizeOfStreamVideoInfo = sizeof(m_streamVideoInfo);
    result = AVIStreamReadFormat( m_streamVideo,
                                  0,
                                  &m_streamVideoInfo,
                                  &sizeOfStreamVideoInfo );

    HandleAviError(result);

    if( m_streamVideoInfo.biSizeImage <= 0 ) return -1;

    m_currentFrame = new unsigned char[512 * 256 * 3];
    memset( m_currentFrame, 0, 512*256*3 );


    //
    // Begin video streaming

    int firstFrame = AVIStreamStart( m_streamVideo );
    int lastFrame = AVIStreamEnd( m_streamVideo );
    result = AVIStreamBeginStreaming( m_streamVideo,
                                      firstFrame,                                       // start frame
                                      lastFrame,                                        // end frame
                                      2000 );                                           // speed (1000=normal)

    HandleAviError(result);


    m_streamVideoFrame = AVIStreamGetFrameOpen( m_streamVideo, NULL );

    if( !m_streamVideoFrame ) return -1;


    //
    // Open an audio stream from the file

    result = AVIFileGetStream( m_aviFile,
                              &m_streamAudio,
                              streamtypeAUDIO,
                              0 );

    HandleAviError(result);

    long sizeOfWaveFormat = sizeof(m_streamAudioInfo);
    result = AVIStreamReadFormat( m_streamAudio, 0, &m_streamAudioInfo, &sizeOfWaveFormat );

    HandleAviError(result);

    //
    // Read audio data

//    result = AVIStreamBeginStreaming( m_streamAudio,
//                                      firstFrame,
//                                      lastFrame,
//                                      1000 );
//
//    HandleAviError(result);

    return 0;
}


int AviPlayer::GetAudioData( int _numSamples, signed short *_buffer )
{
    long numBytesWritten, numSamplesWritten;
    int result = AVIStreamRead( m_streamAudio,
                                m_nextAudioSample,
                                _numSamples,
                                _buffer,
                                _numSamples*sizeof(signed short),
                                &numBytesWritten, &numSamplesWritten );

    m_nextAudioSample += numSamplesWritten;

    return numSamplesWritten;
}


int AviPlayer::CloseAvi()
{
    if( m_currentFrame )
    {
        delete m_currentFrame;
        m_currentFrame = NULL;
    }


    int result = AVIStreamGetFrameClose( m_streamVideoFrame );
    result = AVIStreamEndStreaming( m_streamVideo );
    result = AVIStreamRelease( m_streamVideo );

    result = AVIStreamEndStreaming( m_streamAudio );
    result = AVIStreamRelease( m_streamAudio );

    result = AVIFileRelease( m_aviFile );

    return 0;
}


int AviPlayer::GetWidth()
{
    return m_streamVideoInfo.biWidth;
}


int AviPlayer::GetHeight()
{
    return m_streamVideoInfo.biHeight;
}


unsigned char *AviPlayer::GetFrameData()
{
    return m_currentFrame;
}


int AviPlayer::DecodeVideoFrame( float _timeIndex )
{
    int frameIndex = AVIStreamTimeToSample( m_streamVideo, _timeIndex*1000 );

    return DecodeVideoFrame( frameIndex );
}


int AviPlayer::DecodeVideoFrame( int _frameIndex )
{
    static int numDisplayed = 0;
    static int numMissed = 0;

    if( _frameIndex > m_previousFrameIndex )
    {
        numDisplayed++;
        numMissed += (_frameIndex-m_previousFrameIndex-1);
    }

    int lastFrame = AVIStreamEnd( m_streamVideo );

    if( _frameIndex == m_previousFrameIndex )
    {
        return 0;
    }
    else if( _frameIndex >= lastFrame )
    {
        DebugOut( "Displayed %d, Missed %d, out of %d  (%2.2f%%)\n",
                     numDisplayed, numMissed, _frameIndex+1,
                     100*(float)numDisplayed/(float)(_frameIndex+1));
        return 1;
    }
    else
    {
        unsigned char *pDib = (unsigned char *) AVIStreamGetFrame( m_streamVideoFrame,
                                                                   _frameIndex );
        if( !pDib ) return 1;

        unsigned char *imgDataStart = pDib + sizeof(BITMAPINFOHEADER);

        //
        // We need to get the data into a usable form
        // Firstly the colour ordering is wrong : DIBS are BGR and openGL requires RGB
        // Secondly the size must be a power of 2 for openGL, ie 512x256 instead of 320x240

        for( int y = 0; y < m_streamVideoInfo.biHeight; ++y )
        {
            unsigned char *fromRow = imgDataStart + y * m_streamVideoInfo.biWidth * 3;
            unsigned char *toRow = m_currentFrame + y * 512 * 3;

            for( int x = 0; x < m_streamVideoInfo.biWidth; ++x )
            {
                unsigned char *fromPixel = fromRow + x * 3;
                unsigned char *toPixel = toRow + x * 3;

                toPixel[0] = fromPixel[2];
                toPixel[1] = fromPixel[1];
                toPixel[2] = fromPixel[0];
            }
        }

        m_previousFrameIndex = _frameIndex;

    }

    return 0;
}



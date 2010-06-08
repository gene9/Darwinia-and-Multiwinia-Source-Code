#include "lib/universal_include.h"

#include <stdio.h>

#include "lib/avi_generator.h"
#include "lib/debug_utils.h"
#include "lib/preferences.h"

#include "app.h"
#include "renderer.h"


#ifdef AVI_GENERATOR


AVIGenerator::AVIGenerator()
:	m_frameRate(60),
	m_AVIFile(NULL), 
	m_stream(NULL), 
	m_streamCompressed(NULL),
	m_pixels(NULL),
	m_accumBuffer(NULL)
{
	m_motionBlurFactor = g_prefsManager->GetInt("AVIMotionBlurFactor", 1);

	m_frameRate = g_prefsManager->GetInt("DemoFrameRate", 25);
	m_frameRate /= m_motionBlurFactor;
	
	m_bih.biSize = sizeof(BITMAPINFOHEADER);
	m_bih.biWidth = g_app->m_renderer->ScreenW();
	m_bih.biHeight = g_app->m_renderer->ScreenH();
	m_bih.biPlanes = 1;
	m_bih.biBitCount = 24;
	m_bih.biCompression = BI_RGB;
	m_bih.biSizeImage = 0;
	m_bih.biXPelsPerMeter = 1000;
	m_bih.biYPelsPerMeter = 1000;
	m_bih.biClrUsed = 0;
	m_bih.biClrImportant = 0;
    m_outputFilename = "Untitled.avi"; 
}


AVIGenerator::~AVIGenerator()
{   
	if ( g_prefsManager->GetInt( "RecordDemo" ) == 2 )
    {
	    ReleaseEngine();
    }

	// Just checking that all allocated resources have been released
	DarwiniaDebugAssert(m_stream==NULL);
	DarwiniaDebugAssert(m_streamCompressed==NULL);
	DarwiniaDebugAssert(m_AVIFile==NULL);
    DarwiniaDebugAssert(m_pixels==NULL);
    DarwiniaDebugAssert(m_accumBuffer==NULL);
}


void AVIGenerator::Setup()
{
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&m_opts};

	memset(&m_opts, 0, sizeof(m_opts));
	
    if (!AVISaveOptions(NULL, 0, 1, &m_stream, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
	{
		AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
	}

    //g_prefsManager->AddData("AVIOptions", &opts, sizeof(opts));
}


int AVIGenerator::InitEngine()
{
	AVISTREAMINFO strHdr; // information for a single stream 

	int hr;

	m_error = "OK";

	// Step 0 : Let's make sure we are running on 1.1 
	DWORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a)
	{
		 // oops, we are too old, blow out of here 
		m_error="Version of Video for Windows too old. Come on, join the 21th century!";
		return S_FALSE;
	}

	// Step 1 : initialize AVI engine
	AVIFileInit();

	// Step 2 : Open the movie file for writing....
	hr = AVIFileOpen(&m_AVIFile,			// Address to contain the new file interface pointer
					 m_outputFilename,		// Name your file .avi -> very important
					 OF_WRITE | OF_CREATE,	// Access mode to use when opening the file. 
					 NULL);					// use handler determined from file extension.

	if (hr != AVIERR_OK)
	{
		switch(hr)
		{
		case AVIERR_BADFORMAT: 
			m_error = "The file couldn't be read, indicating a corrupt file or an unrecognized format.";
			break;
		case AVIERR_MEMORY:		
			m_error = "The file could not be opened because of insufficient memory."; 
			break;
		case AVIERR_FILEREAD:
			m_error = "A disk error occurred while reading the file."; 
			break;
		case AVIERR_FILEOPEN:		
			m_error = "A disk error occurred while opening the file.";
			break;
		case REGDB_E_CLASSNOTREG:		
			m_error = "According to the registry, the type of file specified in AVIFileOpen does not have a handler to process it";
			break;
		}

		return hr;
	}

	// Fill in the header for the video stream....
	memset(&strHdr, 0, sizeof(strHdr));
	strHdr.fccType                = streamtypeVIDEO;	// video stream type
	strHdr.fccHandler             = 0;
	strHdr.dwScale                = 1;					// should be one for video
	strHdr.dwRate                 = m_frameRate;		// fps
	strHdr.dwSuggestedBufferSize  = m_bih.biSizeImage;	// Recommended buffer size, in bytes, for the stream.
	SetRect(&strHdr.rcFrame, 0, 0,					    // rectangle for stream
			(int) m_bih.biWidth,
			(int) m_bih.biHeight);

	// Step 3 : Create the stream;
	hr = AVIFileCreateStream(m_AVIFile,		    // file pointer
							 &m_stream,		    // returned stream pointer
							 &strHdr);		    // stream header

	// Check it succeeded.
	if (hr != AVIERR_OK)
	{
		m_error = "AVI Stream creation failed. Check Bitmap info.";
		if (hr==AVIERR_READONLY)
		{
			m_error = "Read only file.";
		}
		return hr;
	}

    // Step 4:  Get video options
//    if (!g_prefsManager->DoesKeyExist("AVIOptions"))
    {
        Setup();
    }
//	AVICOMPRESSOPTIONS opts;
//    g_prefsManager->GetData("AVIOptions", &opts, sizeof(opts));


	// Step 5:  Create a compressed stream using codec options.
	hr = AVIMakeCompressedStream(&m_streamCompressed, m_stream, &m_opts, NULL);

	if (hr != AVIERR_OK)
	{
		m_error = "AVI Compressed Stream creation failed.";
		
		switch(hr)
		{
		case AVIERR_NOCOMPRESSOR:
			m_error = "A suitable compressor cannot be found.";
			break;
		case AVIERR_MEMORY:
			m_error = "There is not enough memory to complete the operation.";
			break; 
		case AVIERR_UNSUPPORTED:
			m_error = "Compression is not supported for this type of data. "
					  "This error might be returned if you try to compress data that is not audio or video.";
			break;
		}

		return hr;
	}

	// releasing memory allocated by AVISaveOptionFree
//	hr = AVISaveOptionsFree(1,(LPAVICOMPRESSOPTIONS FAR *) &aopts);
//	if (hr != AVIERR_OK)
//	{
//		m_error = "Error releasing memory";
//		return hr;
//	}

	// Step 6 : sets the format of a stream at the specified position
	hr = AVIStreamSetFormat(m_streamCompressed, 
					0,			// position
					&m_bih,	    // stream format
					m_bih.biSize +   // format size
					m_bih.biClrUsed * sizeof(RGBQUAD));

	if (hr != AVIERR_OK)
	{
		m_error = "AVI Compressed Stream format setting failed.";
		return hr;
	}

	// Step 6 : Initialize step counter
	m_inFrameCount = 0;
	m_outFrameCount = 0;

    // Make room for our image buffer
    m_pixels = new unsigned char[ m_bih.biWidth * m_bih.biHeight * (m_bih.biBitCount / sizeof(unsigned char)) ];
    m_accumBuffer = new unsigned char[ m_bih.biWidth * m_bih.biHeight * (m_bih.biBitCount / sizeof(unsigned char)) ];
    
	return hr;
}

void AVIGenerator::ReleaseEngine()
{
	if (m_stream)
	{
		AVIStreamRelease(m_stream);
		m_stream=NULL;
	}

	if (m_streamCompressed)
	{
		AVIStreamRelease(m_streamCompressed);
		m_streamCompressed=NULL;
	}

	if (m_AVIFile)
	{
		AVIFileRelease(m_AVIFile);
		m_AVIFile=NULL;
	}

    delete m_pixels;
    m_pixels = NULL;

	delete m_accumBuffer;
	m_accumBuffer = NULL;

	// Close engine
	AVIFileExit();
}

void AVIGenerator::AddFrame()
{
    DarwiniaDebugAssert( m_pixels );
    DarwiniaDebugAssert( m_accumBuffer );

    glReadPixels(0, 0, m_bih.biWidth, m_bih.biHeight, GL_RGB, GL_UNSIGNED_BYTE, m_pixels );            

    unsigned char *inPixel = m_pixels;
    unsigned char *outPixel = m_accumBuffer;
	float multiplier = 1.0f / m_motionBlurFactor;

	if (m_inFrameCount % m_motionBlurFactor == 0)
	{
		for( int y = 0; y < m_bih.biHeight; ++y )
		{
			for( int x = 0; x < m_bih.biWidth; ++x )
			{        
				outPixel[0] = inPixel[2] * multiplier;
				outPixel[1] = inPixel[1] * multiplier;
				outPixel[2] = inPixel[0] * multiplier;

				inPixel += 3;
				outPixel += 3;
			}
		}
	}
	else
	{
		for( int y = 0; y < m_bih.biHeight; ++y )
		{
			for( int x = 0; x < m_bih.biWidth; ++x )
			{        
				outPixel[0] += inPixel[2] * multiplier;
				outPixel[1] += inPixel[1] * multiplier;
				outPixel[2] += inPixel[0] * multiplier;

				inPixel += 3;
				outPixel += 3;
			}
		}
	}

	if (m_inFrameCount % m_motionBlurFactor == m_motionBlurFactor - 1)
	{
		AddFrame(m_accumBuffer);
	}
	
	m_inFrameCount++;
}

int AVIGenerator::AddFrame(unsigned char *bmBits)
{
	CheckSanity();

	int hr;

	// compress bitmap
	hr = AVIStreamWrite(m_streamCompressed,	// stream pointer
		m_outFrameCount,					// time of this frame
		1,									// number to write
		bmBits,								// image buffer
		m_bih.biSizeImage,					// size of this frame
		AVIIF_KEYFRAME,						// flags....
		NULL,
		NULL);

	// updating frame counter
	m_outFrameCount++;

	return hr;
}

void AVIGenerator::CheckSanity()
{
	// Make sure filename is sensible
	char *extension = m_outputFilename + strlen(m_outputFilename)-3;
	DarwiniaDebugAssert( stricmp(extension, "avi") == 0 );

	// checking that bitmap size are multiple of 4
	DarwiniaDebugAssert(m_bih.biWidth%4 == 0);
	DarwiniaDebugAssert(m_bih.biHeight%4 == 0);
}

int AVIGenerator::GetRecordingFrameRate()
{
    return m_frameRate;
}

#endif // AVI_GENERATOR

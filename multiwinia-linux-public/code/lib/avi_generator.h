// http://www.codeproject.com/audio/avigenerator.asp?print=true
// AVIGenerator.h: interface for the CAVIGenerator class.
//
// A class to easily create AVI
//
// Original code : Example code in WriteAvi.c of MSDN
// 
// Author : Jonathan de Halleux. dehalleux@auto.ucl.ac.be
//////////////////////////////////////////////////////////////////////

#ifndef _included_avi_generator
#define _included_avi_generator

#ifdef AVI_GENERATOR

#include <comdef.h>
#include <vfw.h>

/*************************************************************************
 Class AVIGenerator

Usage

  Step 1 : Declare an AVIGenerator object
  Step 2 : Set Bitmap header + other parameters
  Step 3 : Initialize engine by calling InitEngine
  Step 4 : Send each frames to engine with function AddFrame
  Step 5 : Close engine by calling ReleaseEngine

Demo Code:

	AVIGenerator AviGen;
	unsigned char *bmBits;

	// set characteristics
	AviGen.m_frameRate = 20;					// set 20fps
	AviGen.m_bih.blah = x;						// give info about bitmap
	AviGen.m_bih.blah = y;
	AviGen.m_bih.blah = z;

	AviGen.InitEngine();

	..... // Draw code, bmBits is the buffer containing the frame
	AviGen.AddFrame(bmBits);
	.....

	AviGen.ReleaseEngine();
*/
class AVIGenerator  
{
public:
	char				*m_error;
	BITMAPINFOHEADER	m_bih;			// Format of the one uncompressed frame's bitmap
	char				*m_outputFilename;			

	AVIGenerator();
	~AVIGenerator();

    void Setup();

	// Initialize engine and choose codec
	// Some asserts are made to check that bitmap has been properly initialized
	int InitEngine();

	// Adds a frame to the movie. 
    // First one grabs directly from openGL
	void AddFrame(); 

    // The data pointed by bmBits has to be compatible with the bitmap description of the movie.
    int AddFrame(unsigned char* bmBits);

	// Release ressources allocated for movie and close file.
	void ReleaseEngine();

	void CheckSanity();

    int GetRecordingFrameRate();

private:
	AVICOMPRESSOPTIONS  m_opts;

	unsigned			m_motionBlurFactor;		// Number of frames blended together in the accumulation buffer. 1 = no motion blur
	unsigned long		m_frameRate;	
	int					m_inFrameCount;
	int					m_outFrameCount;
	PAVIFILE			m_AVIFile;				// file interface pointer
	PAVISTREAM			m_stream;				// Address of the stream interface
	PAVISTREAM			m_streamCompressed;		// Address of the compressed video stream

    unsigned char       *m_pixels;              // Our copy of the frame buffer
    unsigned char       *m_accumBuffer;         // Our accumulation buffer, containing the blended result of one or more frame buffers
};

#endif // AVI_GENERATOR

#endif

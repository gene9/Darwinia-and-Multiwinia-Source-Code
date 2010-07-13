#ifndef FAKE_ASWSDLRINGBUFFER_H_
#define FAKE_ASWSDLRINGBUFFER_H_

#include <stdlib.h>
#include <string.h>

#define HAVE_FAKE_ASWSDLRINGBUFFER

class ASWSDLRingBuffer
{
public:
	ASWSDLRingBuffer()
	{
		m_Storage = 0;
		Reset();
	}
	~ASWSDLRingBuffer()
	{
		Reset();
	}
	void Reset(int sizeInBytes=0, bool clear = false)
	{
		if (m_Storage)
			free(m_Storage);
		m_Length = sizeInBytes;
		m_ReadIndex = m_WriteIndex = 0;
		if (!m_Length)
			return;
		if (clear)
			m_Storage = (char*)calloc(1,m_Length);
		else
			m_Storage = (char*)malloc(m_Length);
	}
	//Compat with ASWSDLRingBuffer
	//NOTE: I don't know what the parameters actually are, but these give the smoothest audio.
	void Allocate(int channels, int samplesize, int buffersize)
	{
		Reset(samplesize*buffersize);
	}
	
	void StoreAudio(void *data, int size)
	{
		if (size > m_Length)
		{
			//Read one buffer length from the end of the input
			size = m_Length;
			data = (void*)((char*)data+(size-m_Length));
		}	
		int firstSize = (m_Length - m_WriteIndex > size) ? size : m_Length - m_WriteIndex;
		memcpy(m_Storage + m_WriteIndex, data, firstSize);
		if (size - firstSize)
		{
			memcpy(m_Storage, ((char*)data) + firstSize, size - firstSize);
		}
		m_WriteIndex = (m_WriteIndex + size) % m_Length;
	}
	
	
	void FetchAudio(void *data, int size)
	{
		if (size > m_Length)
		{
			//Write out one buffer length, zeroing the end of the input.
			size = m_Length;
			memset((char*)data+m_Length,0,size-m_Length);
		}
		int firstSize = (m_Length - m_ReadIndex > size) ? size : m_Length - m_ReadIndex;
		memcpy(data, m_Storage + m_ReadIndex, firstSize);
		if (size - firstSize)
		{
			memcpy(((char*)data) + firstSize, m_Storage, size - firstSize);
		}
		m_ReadIndex = (m_ReadIndex + size) % m_Length;
	}	
	
	int SamplesFree()
	{
		return ((m_ReadIndex + m_Length) - m_WriteIndex)%m_Length;
	}
	
private:
	char *m_Storage;
	int m_Length;
	int m_ReadIndex;
	int m_WriteIndex;
};
#endif

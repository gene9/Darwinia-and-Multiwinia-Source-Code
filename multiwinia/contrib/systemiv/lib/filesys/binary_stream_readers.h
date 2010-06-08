#ifndef INCLUDED_BINARY_STREAM_READERS_H
#define INCLUDED_BINARY_STREAM_READERS_H

#include <stdio.h>
#include <string>


//*****************************************************************************
// Class BinaryReader
//*****************************************************************************

class BinaryReader
{
public:
	bool			m_eof;
	std::string		m_filename;

	BinaryReader		 ();
	virtual ~BinaryReader();

    virtual bool			IsOpen  () = 0;
	virtual const char *	GetFileType();

	virtual signed char		ReadS8	() = 0;
	virtual short			ReadS16	() = 0;
	virtual int				ReadS32	() = 0;
	virtual unsigned char	ReadU8	() = 0;

	virtual unsigned int	ReadBytes(unsigned int _count, unsigned char *_buffer) = 0;

	virtual int				Seek	(int _offset, int _origin) = 0;
	virtual int				Tell	() = 0;

	int                     GetSize ();
};


//*****************************************************************************
// Class BinaryFileReader
//*****************************************************************************

class BinaryFileReader: public BinaryReader
{
protected:
	FILE			*m_file;

public:
	BinaryFileReader			(char const *_filename);
	~BinaryFileReader			();

    bool			IsOpen		();

	signed char		ReadS8		();
	short			ReadS16		();
	int				ReadS32		();
	unsigned char	ReadU8		();

	unsigned int	ReadBytes	(unsigned int _count, unsigned char *_buffer);

	int				Seek		(int _offset, int _origin);
	int				Tell		();
};


//*****************************************************************************
// Class BinaryDataReader
//*****************************************************************************

class BinaryDataReader: public BinaryReader
{
protected:
	unsigned int		m_offset;

public:
	unsigned char const *m_data;
	unsigned int		m_dataSize;

	BinaryDataReader			(unsigned char const *_data, unsigned int _dataSize, 
								 char const *_filename);
	~BinaryDataReader			();

    bool			IsOpen		();

	signed char		ReadS8		();
	short			ReadS16		();
	int				ReadS32		();
	unsigned char	ReadU8		();

	unsigned int	ReadBytes	(unsigned int _count, unsigned char *_buffer);

	int				Seek		(int _offset, int _origin);
	int				Tell		();
};


#endif

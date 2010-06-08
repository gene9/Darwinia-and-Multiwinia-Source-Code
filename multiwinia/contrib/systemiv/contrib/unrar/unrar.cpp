#include "array.h"
#include "cmddata.h"
#include "consio.h"
#include "extract.h"
#include "file.h"
#include "log.h"
#include "unrar.h"
#include "rarfn.h"
#include "system.h"


//*****************************************************************************
// Class MemMappedFile
//*****************************************************************************

MemMappedFile::MemMappedFile(char const *_filename, unsigned int _size)
:	m_size(_size)
{
	m_data = new unsigned char[_size];
	m_filename = strdup(_filename);
	unsigned int len = strlen(_filename);
	for (unsigned int i = 0; i < len; ++i)
	{
		if (m_filename[i] == '\\') 
		{
			m_filename[i] = '/';
		}
	}
}


MemMappedFile::~MemMappedFile()
{
	m_size = 0;
	delete [] m_data;
	free (m_filename);
}


//*****************************************************************************
// Class UncompressedArchive
//*****************************************************************************

UncompressedArchive::UncompressedArchive(char const *_filename, char const *_password)
:	m_numFiles(0)
{
	char *filename = strdup(_filename);

	ErrHandler.SetSignalHandlers(true);
	RARInitData();
#ifdef TARGET_OS_WINDOWS
	SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif
	CommandData command;
	command.ParseArg("x", NULL);
	command.ParseArg(filename, NULL);
	if (_password)
	{
		char buf[128];
		strcpy(buf, "-p");
		strncat(buf, _password, 125);
		command.ParseArg(buf, NULL);
	}
	command.ParseDone();

	InitConsoleOptions(command.MsgStream, command.Sound);
	InitSystemOptions(command.SleepTime);
	InitLogOptions(command.LogName);
	ErrHandler.SetSilent(command.AllYes || command.MsgStream==MSG_NULL);
	ErrHandler.SetShutdown(command.Shutdown);

//	command.OutTitle();
//	command.ProcessCommand();
	command.AddArcName(filename, NULL/*FindData.NameW(*/);
	CmdExtract Extract;
	Extract.DoExtract(&command, this);

	File::RemoveCreated();

	free(filename); // was from strdup(_filename)
}


MemMappedFile *UncompressedArchive::AllocNewFile(char const *_filename, unsigned int _size)
{
	if (m_numFiles == MAX_FILES) return NULL;

	m_files[m_numFiles] = new MemMappedFile(_filename, _size);
	m_numFiles++;

	return m_files[m_numFiles-1];
}

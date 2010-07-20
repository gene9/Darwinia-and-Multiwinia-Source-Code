#include "lib/universal_include.h"
#include "lib/debug_logging.h"

DarwiniaLogBuffer g_log;

DarwiniaLogBuffer::DarwiniaLogBuffer(int _size)
	: m_size(_size), m_head(0), m_used(0), m_msgs(new DarwiniaLogMsg *[_size])
{
	for (int i = 0; i < m_size; i++)
		m_msgs[i] = NULL;
}

void DarwiniaLogBuffer::Display()
{
	int start = (m_head - m_used + m_size) % m_size;
	for (int i = 0; i < m_used; i++) {
		DarwiniaLogMsg *msg = m_msgs[(start + i) % m_size];
		msg->Display();
	}
}


void DisplayLog()
{
	// Suitable for calling from gdb if you have the symbols
	// Otherwise send Darwinia a HUP signal and this
	// routine will be printed.
	g_log.Display();
}


const char *SkipRelativePathPrefix(const char *_filename)
{
	const char *filename = _filename;

	while (*filename == '.' || *filename == '/')
		filename++;

	return filename;
}


void DarwiniaLogMsg::Display() {
	printf("%s:%d:%s:", SkipRelativePathPrefix(m_filename), m_line, m_function);
	DisplayCall();
	printf("\n");
}

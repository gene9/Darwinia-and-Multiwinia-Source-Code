#ifndef __DEBUG_LOGGING_H
#define __DEBUG_LOGGING_H

class DarwiniaLogMsg {
public:
	DarwiniaLogMsg(const char *_filename, int _line, const char *_function)
		: m_filename(_filename), m_line(_line), m_function(_function)
	{
	}

	virtual ~DarwiniaLogMsg()
	{
	}

	void Display();

protected:
	virtual void DisplayCall() = 0;

private:
	const char *m_filename;
	int m_line;
	const char *m_function;
};

class DarwiniaLogBuffer {
public:
	DarwiniaLogBuffer(int _size = 100);

	// Add only DarwiniaLogMsg created with new,
	// DarwiniaLogBuffer will delete the msgs as appropriate.
	void Add( DarwiniaLogMsg *_msg ) {
		if (m_used == m_size)
			delete m_msgs[m_head];
		else
			m_used++;
		m_msgs[m_head] = _msg;
		m_head = (m_head + 1) % m_size;
	}

	void Display();

private:
	int m_size, m_head, m_used;
	DarwiniaLogMsg **m_msgs;
};

extern DarwiniaLogBuffer g_log;

#endif
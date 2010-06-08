#include "lib/universal_include.h"
#include "lib/debug_logging.h"

#undef glEnable
#undef glDisable
#undef glFogf
#undef glFogi
#undef glFogfv
#undef glFogiv

#ifdef FOG_LOGGING

bool g_fogLogging = false;

static const char *GLCapToString(GLenum _cap) {
	switch(_cap) {
		case GL_FOG: return "GL_FOG";
		default: {
			static char buf[64];
			sprintf(buf, "%d", (int) _cap);
			return buf;
		}
	}
}

static const char *GLFogParamToString(GLenum _pname)
{
	static char buf[64];
	switch (_pname) {
		case GL_FOG_MODE:		return "GL_FOG_MODE";
		case GL_FOG_DENSITY:	return "GL_FOG_DENSITY";
		case GL_FOG_START:		return "GL_FOG_START";
		case GL_FOG_END:		return "GL_FOG_END";
		case GL_FOG_INDEX:		return "GL_FOG_INDEX";
		case GL_FOG_COLOR:		return "GL_FOG_COLOR";
		default: 
			sprintf(buf, "Unknown(%d)", (int) _pname);
			return buf;
	}
}

static const char *GLFogModeToString(int _mode)
{
	switch (_mode) {
		case GL_LINEAR:		return "GL_LINEAR";
		case GL_EXP:		return "GL_EXP";
		case GL_EXP2:		return "GL_EXP2";
		default: {
			static char buf[64];
			printf(buf, "Unknown(%d)", (int) _mode);
			return buf;
		}
	}
}


class DarwiniaGLEnableLogMsg : public DarwiniaLogMsg {
public:
	DarwiniaGLEnableLogMsg(const char *_filename, int _line, const char *_function,
				           bool _enable, GLenum _cap)
		: DarwiniaLogMsg(_filename, _line, _function),
		  m_enable(_enable), m_cap(_cap)
	{
	}
	
protected:
	void DisplayCall() {
		printf("gl%sable(%s)", m_enable ? "En" : "Dis", GLCapToString(m_cap));
	}
	
private:
	bool m_enable;
	GLenum m_cap;
};


template<class T, bool t_isFloat> 
inline
void DisplayFogValue(GLenum _param, T _value) 
{
	if (_param != GL_FOG_MODE) 
		printf(t_isFloat ? "%.1f" : "%d", _value);	
	else 
		printf("%s", GLFogModeToString( (int) _value ) );
}


template<class T, bool t_isFloat> 
class DarwiniaGLFogLogMsg : public DarwiniaLogMsg {
public:
	DarwiniaGLFogLogMsg(const char *_filename, int _line, const char *_function,
			            GLenum _param, T _value)
		: DarwiniaLogMsg(_filename, _line, _function),
		  m_param(_param), m_value(_value)
	{
	}
	
protected:
	void DisplayCall() {
		printf("glFog%s(%s, ", t_isFloat ? "f" : "i", GLFogParamToString(m_param));
		DisplayFogValue<T, t_isFloat>(m_param, m_value);
		printf(")");
	}

private:
	GLenum m_param;
	T m_value;
};


template<class T, bool t_isFloat> 
class DarwiniaGLFogvLogMsg : public DarwiniaLogMsg {
public:
	DarwiniaGLFogvLogMsg(const char *_filename, int _line, const char *_function,
			            GLenum _param, const T *_value)
		: DarwiniaLogMsg(_filename, _line, _function),
		  m_param(_param)
	{
		for (int i = 0; i < 4; i++) 
			m_value[i] = _value[i];
	}
	
protected:
	void DisplayCall() {
		printf("glFog%sv(%s, [", t_isFloat ? "f" : "i", GLFogParamToString(m_param));
		for (int i = 0; i < 4; i++) {
			DisplayFogValue<T, t_isFloat>(m_param, m_value[i]);
			if (i < 3)
				printf(", ");
		}
		printf("])");
	}

private:
	GLenum m_param;
	T m_value[4];
};

#endif // FOG_LOGGING


bool g_fogEnabled = true;

void glLogEnable(const char *_filename, int _line, const char *_function, GLenum _cap)
{
	if (_cap == GL_FOG) {
		if (g_fogEnabled) {
#ifdef FOG_LOGGING
			if (g_fogLogging) 
				g_log.Add( new DarwiniaGLEnableLogMsg( _filename, _line, _function, true, _cap) );
#endif
			glEnable(_cap);
		}
	}
	else 
		glEnable(_cap);
}


void glLogDisable(const char *_filename, int _line, const char *_function, GLenum _cap)
{
	if (_cap == GL_FOG) {
		if (g_fogEnabled) {
#ifdef FOG_LOGGING
			if (g_fogLogging) 
				g_log.Add( new DarwiniaGLEnableLogMsg( _filename, _line, _function, false, _cap) );
#endif
			glDisable(_cap);
		}
	}
	else 
		glDisable(_cap);
}


void glLogFogf(const char *_filename, int _line, const char *_function, GLenum _pname, GLfloat _param )
{
	if (g_fogEnabled) {
#ifdef FOG_LOGGING
		if (g_fogLogging) 
			g_log.Add( new DarwiniaGLFogLogMsg<float, true>( _filename, _line, _function, _pname, _param ) );
#endif
		glFogf(_pname, _param);
	}
}

void glLogFogi(const char *_filename, int _line, const char *_function, GLenum _pname, GLint _param )
{
	if (g_fogEnabled) {
#ifdef FOG_LOGGING
		if (g_fogLogging) 
			g_log.Add( new DarwiniaGLFogLogMsg<int, false>( _filename, _line, _function, _pname, _param ) );
#endif
		glFogi(_pname, _param);
	}
}


void glLogFogfv(const char *_filename, int _line, const char *_function, GLenum _pname, const GLfloat *_params )
{
	if (g_fogEnabled) {
#ifdef FOG_LOGGING
		if (g_fogLogging) 
			g_log.Add( new DarwiniaGLFogvLogMsg<GLfloat, true>( _filename, _line, _function, _pname, _params ) );		
#endif
		glFogfv(_pname, _params);
	}
}


void glLogFogiv(const char *_filename, int _line, const char *_function, GLenum _pname, const GLint *_params )
{
	if (g_fogEnabled) {
#ifdef FOG_LOGGING
		if (g_fogLogging) 
			g_log.Add( new DarwiniaGLFogvLogMsg<GLint, false>( _filename, _line, _function, _pname, _params ) );		
#endif
		glFogiv(_pname, _params);
	}
}

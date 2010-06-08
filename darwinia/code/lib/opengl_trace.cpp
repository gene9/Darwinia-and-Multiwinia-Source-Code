#include "lib/universal_include.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_trace.h"
#include "ogl_extensions.h"

#include <stdarg.h>
#include <sstream>
#include <iomanip>

const char *rotatingStaticString(const char *s);

bool g_tracingOpenGL = false;
static bool s_numberOfCalls = 0;

void glTraceEnable( bool _enable)
{
	g_tracingOpenGL = _enable;
}

void glTrace(const char *_fmt, ...)
{
	static FILE *output = NULL;
	static unsigned line_number = 0;
	const unsigned MAX_LINES = 50000;

	s_numberOfCalls++;

	// Close the log after MAX_LINES worth of lines
	if (g_tracingOpenGL && line_number > MAX_LINES && output) {
		fprintf(output, "Log closed after %d lines\n", MAX_LINES);
		fclose(output);
		output = NULL;
		g_tracingOpenGL = false;
	}

	if (g_tracingOpenGL) {
		va_list ap;
		
		if (!output)
			output = fopen("gltrace.txt", "w");

		va_start(ap, _fmt);
		line_number ++;
		fprintf(output, "% 8d ", line_number);
		vfprintf(output, _fmt, ap);
		va_end(ap);
		fprintf(output, "\n");
		fflush(output);
	}
}


const char *rotatingStaticString(const char *_s)
{
	const int MAX_STRINGS = 32;
	static char *strings[MAX_STRINGS];
	static int string_sizes[MAX_STRINGS];
	static int index = -1;

	if (!_s[0])
		return "";

	if (index == -1) {
		memset(strings, 0, sizeof(char *) * MAX_STRINGS);
		memset(string_sizes, 0, sizeof(int) * MAX_STRINGS);
	}

	index = (index + 1) % MAX_STRINGS;

	int len = strlen(_s) + 1;
	if (len > string_sizes[index]) {
		delete[] strings[index];
		strings[index] = new char[len];
		string_sizes[index] = len;
	}

	return strcpy(strings[index], _s);	
}

const char *glEnumToString(GLenum id)
{
	switch (id) {
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM"; break;                   
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE"; break;                  
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION"; break;              
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW"; break;                 
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW"; break;                
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY"; break;                  
		case GL_POINTS: return "GL_POINTS"; break;                         
		case GL_LINES: return "GL_LINES"; break;                          
		case GL_LINE_LOOP: return "GL_LINE_LOOP"; break;                      
		case GL_TRIANGLES: return "GL_TRIANGLES"; break;                      
		case GL_TRIANGLE_STRIP: return "GL_TRIANGLE_STRIP"; break;                 
		case GL_TRIANGLE_FAN: return "GL_TRIANGLE_FAN"; break;                   
		case GL_QUADS: return "GL_QUADS"; break;                          
		case GL_QUAD_STRIP: return "GL_QUAD_STRIP"; break;                     
		case GL_FLAT: return "GL_FLAT"; break;							  
		case GL_SMOOTH: return "GL_SMOOTH"; break;						  
		case GL_TEXTURE_2D: return "GL_TEXTURE_2D"; break;					  
		case GL_LEQUAL: return "GL_LEQUAL"; break;                         
		case GL_GREATER: return "GL_GREATER"; break;                        
		case GL_MULT: return "GL_MULT"; break;                           
		case GL_TEXTURE_MAG_FILTER: return "GL_TEXTURE_MAG_FILTER"; break;             
		case GL_TEXTURE_MIN_FILTER: return "GL_TEXTURE_MIN_FILTER"; break;             
		case GL_TEXTURE_WRAP_S: return "GL_TEXTURE_WRAP_S"; break;                 
		case GL_TEXTURE_WRAP_T: return "GL_TEXTURE_WRAP_T"; break;                 
		case GL_TEXTURE_ENV: return "GL_TEXTURE_ENV"; break;                    
		case GL_TEXTURE_ENV_MODE: return "GL_TEXTURE_ENV_MODE"; break;               
		case GL_TEXTURE_ENV_COLOR: return "GL_TEXTURE_ENV_COLOR"; break;              
		case GL_MODULATE: return "GL_MODULATE"; break;                       
		case GL_DECAL: return "GL_DECAL"; break;                          
		case GL_REPLACE: return "GL_REPLACE"; break;                        
		case GL_NEAREST_MIPMAP_LINEAR: return "GL_NEAREST_MIPMAP_LINEAR"; break;          
		case GL_LINEAR_MIPMAP_LINEAR: return "GL_LINEAR_MIPMAP_LINEAR"; break;           
		case GL_NEAREST: return "GL_NEAREST"; break; 
		case GL_LINEAR: return "GL_LINEAR"; break;                         
		case GL_CLAMP: return "GL_CLAMP"; break;                          
		case GL_REPEAT: return "GL_REPEAT"; break;                         
		case GL_FRONT: return "GL_FRONT"; break;                          
		case GL_BACK: return "GL_BACK"; break;                           
		case GL_FRONT_AND_BACK: return "GL_FRONT_AND_BACK"; break;                 
		case GL_SHININESS: return "GL_SHININESS"; break;                      
		case GL_AMBIENT_AND_DIFFUSE: return "GL_AMBIENT_AND_DIFFUSE"; break;            
		case GL_LINE: return "GL_LINE"; break;                           
		case GL_FILL: return "GL_FILL"; break;                           
		case GL_POLYGON_SMOOTH_HINT: return "GL_POLYGON_SMOOTH_HINT"; break;            
		case GL_FOG_HINT: return "GL_FOG_HINT"; break;                       
		case GL_DONT_CARE: return "GL_DONT_CARE"; break;                                                
		case GL_SRC_COLOR: return "GL_SRC_COLOR"; break;                      
		case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR"; break;            
		case GL_SRC_ALPHA: return "GL_SRC_ALPHA"; break;                      
		case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA"; break;            
		case GL_RGB: return "GL_RGB"; break;                            
		case GL_RGBA: return "GL_RGBA"; break;                           
		case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE"; break;                  
		case GL_FLOAT: return "GL_FLOAT"; break;                          
		case GL_LINE_SMOOTH: return "GL_LINE_SMOOTH"; break;                    
		case GL_CULL_FACE: return "GL_CULL_FACE"; break;                      
		case GL_DEPTH_TEST: return "GL_DEPTH_TEST"; break;
		case GL_BLEND: return "GL_BLEND"; break;                          
		case GL_LIGHTING: return "GL_LIGHTING"; break;                       
		case GL_COLOR_MATERIAL: return "GL_COLOR_MATERIAL"; break;                 
		case GL_POLYGON_MODE: return "GL_POLYGON_MODE"; break;                   
		case GL_FRONT_FACE: return "GL_FRONT_FACE"; break;                     
		case GL_NORMALIZE: return "GL_NORMALIZE"; break;                      
		case GL_FOG: return "GL_FOG"; break;                            
		case GL_ALPHA_TEST: return "GL_ALPHA_TEST"; break;                     
		case GL_SCISSOR_TEST: return "GL_SCISSOR_TEST"; break;                   
		case GL_POINT_SMOOTH: return "GL_POINT_SMOOTH"; break;                   
		case GL_BLEND_DST: return "GL_BLEND_DST"; break;                      
		case GL_BLEND_SRC: return "GL_BLEND_SRC"; break;                      
		case GL_MATRIX_MODE: return "GL_MATRIX_MODE"; break;                    
		case GL_VIEWPORT: return "GL_VIEWPORT"; break;                       
		case GL_SHADE_MODEL: return "GL_SHADE_MODEL"; break;                    
		case GL_COLOR_MATERIAL_FACE: return "GL_COLOR_MATERIAL_FACE"; break;            
		case GL_COLOR_MATERIAL_PARAMETER: return "GL_COLOR_MATERIAL_PARAMETER"; break;       
		case GL_DEPTH_FUNC: return "GL_DEPTH_FUNC"; break;                     
		case GL_ALPHA_TEST_FUNC: return "GL_ALPHA_TEST_FUNC"; break;                                   
		case GL_DEPTH_WRITEMASK: return "GL_DEPTH_WRITEMASK"; break;                
		case GL_ALPHA_TEST_REF: return "GL_ALPHA_TEST_REF"; break;                 
		case GL_LIGHT_MODEL_AMBIENT: return "GL_LIGHT_MODEL_AMBIENT"; break;            
		case GL_CW: return "GL_CW"; break;                             
		case GL_CCW: return "GL_CCW"; break;
		case GL_FOG_DENSITY: return "GL_FOG_DENSITY"; break;                    
		case GL_FOG_START: return "GL_FOG_START"; break;                      
		case GL_FOG_END: return "GL_FOG_END"; break;                        
		case GL_FOG_MODE: return "GL_FOG_MODE"; break;                       
		case GL_FOG_COLOR: return "GL_FOG_COLOR"; break;                      
		case GL_LIGHT0: return "GL_LIGHT0"; break;                         
		case GL_LIGHT1: return "GL_LIGHT1"; break;                         
		case GL_LIGHT2: return "GL_LIGHT2"; break;                         
		case GL_LIGHT3: return "GL_LIGHT3"; break;                         
		case GL_LIGHT4: return "GL_LIGHT4"; break;                         
		case GL_LIGHT5: return "GL_LIGHT5"; break;                         
		case GL_LIGHT6: return "GL_LIGHT6"; break;                         
		case GL_LIGHT7: return "GL_LIGHT7"; break;                         
		case GL_AMBIENT: return "GL_AMBIENT"; break;                        
		case GL_DIFFUSE: return "GL_DIFFUSE"; break;                        
		case GL_SPECULAR: return "GL_SPECULAR"; break;                       
		case GL_POSITION: return "GL_POSITION"; break;                       
		case GL_MODELVIEW: return "GL_MODELVIEW"; break;                      
		case GL_PROJECTION: return "GL_PROJECTION"; break;                     
		case GL_MODELVIEW_MATRIX: return "GL_MODELVIEW_MATRIX"; break;               
		case GL_PROJECTION_MATRIX: return "GL_PROJECTION_MATRIX"; break;              
		case GL_CLIP_PLANE0: return "GL_CLIP_PLANE0"; break;                    
		case GL_CLIP_PLANE1: return "GL_CLIP_PLANE1"; break;                    
		case GL_CLIP_PLANE2: return "GL_CLIP_PLANE2"; break;
		case GL_COMPILE: return "GL_COMPILE"; break;                        
		case GL_VENDOR: return "GL_VENDOR"; break;                         
		case GL_RENDERER: return "GL_RENDERER"; break;                       
		case GL_VERSION: return "GL_VERSION"; break;                        
		case GL_EXTENSIONS: return "GL_EXTENSIONS"; break;                     
		case GL_VERTEX_ARRAY: return "GL_VERTEX_ARRAY"; break;                   
		case GL_NORMAL_ARRAY: return "GL_NORMAL_ARRAY"; break;                   
		case GL_COLOR_ARRAY: return "GL_COLOR_ARRAY"; break;                    
		case GL_TEXTURE_COORD_ARRAY: return "GL_TEXTURE_COORD_ARRAY"; break;  

		// OpenGL extensions
		case GL_COMBINE_RGB_EXT: return "GL_COMBINE_RGB_EXT"; break;
		case GL_COMBINE_ALPHA_EXT: return "GL_COMBINE_ALPHA_EXT"; break;
		case GL_ADD_SIGNED_EXT: return "GL_ADD_SIGNED_EXT"; break;
		case GL_INTERPOLATE_EXT: return "GL_INTERPOLATE_EXT"; break;
		case GL_CONSTANT_EXT: return "GL_CONSTANT_EXT"; break;
		case GL_SOURCE0_ALPHA_ARB: return "GL_SOURCE0_ALPHA_ARB"; break;
		case GL_TEXTURE0_ARB: return "GL_TEXTURE0_ARB"; break;
		case GL_TEXTURE1_ARB: return "GL_TEXTURE1_ARB"; break;

		default: return "(unknown enum)";
	}
}

const char *glBooleanToString( bool value )
{
	return value ? "TRUE" : "FALSE";
}

template <class T>
const char *glArrayToString(const T *x, int length)
{
	std::ostringstream s;

	s << "[";
	for (int i = 0; i < length; i++) {
		if (i > 0)
			s << ", ";
		s << x[i];
	}
	s << "]";

	return rotatingStaticString(s.str().c_str());
}

const char *glArrayToString(const GLubyte  *x, int length)
{
	std::ostringstream s;

	s << "[" << std::hex << std::setfill('0');
	for (int i = 0; i < length; i++) {
		if (i > 0)
			s << ", ";
		s << "0x" << std::setw(2) << x[i];
	}
	s << "]";

	return rotatingStaticString(s.str().c_str());
}

template const char *glArrayToString<double>(const double *x, int length);
template const char *glArrayToString<float>(const float *x, int length);
template const char *glArrayToString<int>(const int *x, int length);


void glBitfieldToString1(char *buffer, GLbitfield &f, GLbitfield mask, const char *name)
{
	if (f & mask) {
		if (buffer[0]) strcat(buffer, "|");
		strcat(buffer, name);
		f &= ~mask;
	}
}

const char *glBitfieldToString( GLbitfield f )
{
	char data[128];

	data[0] = '\0';
	glBitfieldToString1(data, f, GL_CURRENT_BIT, "GL_CURRENT_BIT");
	glBitfieldToString1(data, f, GL_DEPTH_BUFFER_BIT, "GL_DEPTH_BUFFER_BIT");
	glBitfieldToString1(data, f, GL_ENABLE_BIT, "GL_ENABLE_BIT");
	glBitfieldToString1(data, f, GL_COLOR_BUFFER_BIT, "GL_COLOR_BUFFER_BIT");
	glBitfieldToString1(data, f, GL_TEXTURE_BIT, "GL_TEXTURE_BIT");
	glBitfieldToString1(data, f, ~0, "(unknown_bit)");

	return rotatingStaticString(data);
}
#endif // USE_DIRECT3D

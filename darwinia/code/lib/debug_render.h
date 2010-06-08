#ifndef _included_debug_render_h
#define _included_debug_render_h

#ifdef DEBUG_RENDER_ENABLED

#include "rgb_colour.h"
#include "vector3.h"

void RenderSquare2d(float x, float y, float size, RGBAColour const &_col=RGBAColour(255,255,255));

void RenderCube(Vector3 const &_centre, float _sizeX, float _sizeY, float _sizeZ, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderSphereRings(Vector3 const &_centre, float _radius, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderSphere(Vector3 const &_centre, float _radius, RGBAColour const &_col=RGBAColour(255,255,255));

void RenderVerticalCylinder(Vector3 const &_centreBase, Vector3 const &_verticalAxis,
							float _height, float _radius,
							RGBAColour const &_col=RGBAColour(255,255,255));

void RenderArrow(Vector3 const &start, Vector3 const &end, float width, RGBAColour const &_col=RGBAColour(255,255,255));
void RenderPointMarker(Vector3 const &point, char const *text, ...);

void PrintMatrix( const char *_name, GLenum _whichMatrix );
void PrintMatrices( const char *_title );

#endif // DEBUG_RENDER_ENABLED

#endif

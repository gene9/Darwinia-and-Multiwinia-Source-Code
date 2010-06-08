#include "lib/universal_include.h"
#ifdef USE_DIRECT3D
#include "lib/opengl_directx_matrix_stack.h"
#include "lib/debug_utils.h"
#include "lib/opengl_directx_dlist_dev.h"
#include "lib/opengl_directx_internals.h"

using namespace OpenGLD3D;

// MatrixStack

MatrixStack::MatrixStack( D3DTRANSFORMSTATETYPE _transformType )
:	m_transformType( _transformType ),
	m_modified( false )
{
}

MatrixStack::~MatrixStack()
{
}

// MatrixStackActual (for immediate rendering)

// JAK TODO: Possible Optimisation
//	Since we're using pre-multiplication, we can actually optimise this
//	to use the device's MultiplyTransform function. This way m_modified
//	will always be false, and FastSetTransform will be very fast indeed

MatrixStackActual::MatrixStackActual(D3DTRANSFORMSTATETYPE _transformType )
:	MatrixStack( _transformType )
{
	D3DXCreateMatrixStack( 0, &m_matrixStack );
}

MatrixStackActual::~MatrixStackActual()
{
	m_matrixStack->Release();
}

void MatrixStackActual::Load( D3DXMATRIX const &_m )
{
	m_matrixStack->LoadMatrix( &_m );
	m_modified = true;
}

void MatrixStackActual::Multiply( D3DXMATRIX const &_m )
{
	// OpenGL does post-multiplies. However, Direct3D is built around
	// pre-multiplication. (All the utility functions give matrices that
	// are suitable for pre-multiplication).

	// There is a correspondence between pre-multiplication and transposition.
	//
	// OpenGL:
	//		v'    = Mv
	//
	// Direct3D:
	//        T      T T
	//		v'    = v M
	//
	// So, whenever we give the import or export matrices from OpenGL, we transpose them
	// so that the OpenGL application is none the wiser.

	m_matrixStack->MultMatrixLocal( &_m );
	m_modified = true;
}

void MatrixStackActual::SetTransform()
{
	D3DXMATRIX *top = m_matrixStack->GetTop();
	// JAK HACK
	//D3DXMATRIX copy;
	//D3DXMatrixTranspose( &copy, top );
	g_pd3dDevice->SetTransform( m_transformType, top );
	m_modified = false;
}

D3DXMATRIX const &MatrixStackActual::GetTransform() const
{
	return *m_matrixStack->GetTop();
}

void MatrixStackActual::Push()
{
	m_matrixStack->Push();
}

void MatrixStackActual::Pop()
{
	m_matrixStack->Pop();
	m_modified = true;
}

// MatrixStackDisplayList

MatrixStackDisplayList::MatrixStackDisplayList( D3DTRANSFORMSTATETYPE _transformType, DisplayListDevice *_device, MatrixStack *_actual )
:	MatrixStack( _transformType ),
	m_device( _device ),
	m_actual( _actual )
{
}

MatrixStackDisplayList::~MatrixStackDisplayList()
{
}

void MatrixStackDisplayList::Load( D3DXMATRIX const &_m )
{
	m_device->RecordLoadTransform( m_actual, _m );
}

void MatrixStackDisplayList::Multiply( D3DXMATRIX const &_m )
{
	m_device->RecordMultMatrix( m_actual, _m );
}

void MatrixStackDisplayList::SetTransform()
{
}

D3DXMATRIX const &MatrixStackDisplayList::GetTransform() const
{
	// Shouldn't be querying this state during a display list
	static D3DXMATRIX identity;
	DarwiniaDebugAssert( false );
	return identity;
}

void MatrixStackDisplayList::Push()
{
	m_device->RecordPushMatrix( m_actual );
}

void MatrixStackDisplayList::Pop()
{
	m_device->RecordPopMatrix( m_actual );
}
#endif // USE_DIRECT3D

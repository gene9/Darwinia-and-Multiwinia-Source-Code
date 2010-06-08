#ifndef __OPENGL_DIRECTX_MATRIX_STACK
#define __OPENGL_DIRECTX_MATRIX_STACK

#include <d3d9.h>
#include <d3dx9.h>

namespace OpenGLD3D {

	class MatrixStack {
	public:
		MatrixStack( D3DTRANSFORMSTATETYPE _transformType );
		virtual ~MatrixStack();

		virtual void Load( D3DXMATRIX const &_m ) = 0;
		virtual void Multiply( D3DXMATRIX const &_m ) = 0;
		virtual void Push() = 0;
		virtual void Pop() = 0;

		virtual void SetTransform() = 0;
		virtual D3DXMATRIX const &GetTransform() const = 0;

		void FastSetTransform();
		bool Modified() const;

	public:
		D3DTRANSFORMSTATETYPE m_transformType;
		bool m_modified;
	};


	class MatrixStackActual : public MatrixStack {
	public:
		MatrixStackActual( D3DTRANSFORMSTATETYPE _transformType );
		~MatrixStackActual();

		void Load( D3DXMATRIX const &_m );
		void Multiply( D3DXMATRIX const &_m );
		void Push();
		void Pop();

		void SetTransform();
		bool Modified() const;
		D3DXMATRIX const &GetTransform() const;

	private:
		LPD3DXMATRIXSTACK m_matrixStack;
	};

	class DisplayListDevice;

	class MatrixStackDisplayList : public MatrixStack {
	public:
		MatrixStackDisplayList( D3DTRANSFORMSTATETYPE _transformType, DisplayListDevice *_device, MatrixStack *_actual );
		~MatrixStackDisplayList();

		void Load( D3DXMATRIX const &_m );
		void Multiply( D3DXMATRIX const &_m );
		void Push();
		void Pop();

		void SetTransform();
		bool Modified() const;
		D3DXMATRIX const &GetTransform() const;

	private:
		LPD3DXMATRIXSTACK m_matrixStack;
		DisplayListDevice *m_device;
		MatrixStack *m_actual;
	};

	// Inlines

	inline bool MatrixStack::Modified() const
	{
		return m_modified;
	}

	inline void MatrixStack::FastSetTransform()
	{
		if (m_modified)
			SetTransform();
	}

};

#endif // __OPENGL_DIRECTX_MATRIX_STACK

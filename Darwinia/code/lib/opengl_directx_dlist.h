#ifndef OPENGL_DIRECTX_DLIST_H
#define OPENGL_DIRECTX_DLIST_H

#include <d3d9.h>
#include <d3dx9.h>

#include <vector>
#include "lib\opengl_directx_internals.h"

namespace OpenGLD3D {

	class MatrixStack;

	class Command {
	public:
		virtual void Execute() = 0;
	};

	class CommandDrawPrimitive : public Command {
	public:
		CommandDrawPrimitive(D3DPRIMITIVETYPE _primitiveType, unsigned _startVertex, unsigned _primitiveCount);
		void Execute();
	private:
		D3DPRIMITIVETYPE m_primitiveType;
		unsigned m_startVertex, m_primitiveCount;
	};

	class CommandLoadTransform : public Command {
	public:
		CommandLoadTransform(MatrixStack *_matrixStack, D3DMATRIX const &_atrix);
		void Execute();
	private:
		MatrixStack *m_matrixStack;
		D3DXMATRIX m_matrix;
	};

	class CommandMultiplyTransform : public Command {
	public:
		CommandMultiplyTransform(MatrixStack *_matrixStack, D3DXMATRIX const & _matrix);
		void Execute();
	private:
		MatrixStack *m_matrixStack;
		D3DXMATRIX m_matrix;
	};

	class CommandPushMatrix : public Command {
	public:
		CommandPushMatrix( MatrixStack *_matrixStack );
		void Execute();
	private:
		MatrixStack *m_matrixStack;
	};

	class CommandPopMatrix : public Command {
	public:
		CommandPopMatrix( MatrixStack *_matrixStack );
		void Execute();
	private:
		MatrixStack *m_matrixStack;
	};

	class DisplayList {
	public:
		DisplayList( std::vector<Command *> const &_commands, std::vector<CustomVertex> const &_vertices );
		~DisplayList();
		void Draw();

	private:

		IDirect3DVertexBuffer9 *m_pVertexBuffer;
		Command **m_commands;
	};


}

#endif // OPENGL_DIRECTX_DLIST_H

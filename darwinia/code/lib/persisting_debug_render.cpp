#include "lib/universal_include.h"

#include <stdarg.h>
#include <string.h>

#include "lib/debug_render.h"
#include "lib/persisting_debug_render.h"


#ifdef DEBUG_RENDER_ENABLED


PersistingDebugRenderer g_debugRenderer;


PersistingDebugRenderer::PersistingDebugRenderer()
{
	m_items.SetSize(100);
}


PersistRenderItem *PersistingDebugRenderer::FindItem(char const *_label)
{
	for (unsigned int i = 0; i < m_items.Size(); ++i)
	{
		if (!m_items.ValidIndex(i)) continue;

		if (strnicmp(_label, m_items[i].m_label, sizeof(m_items[i].m_label) - 1) == 0)
		{
			return &m_items[i];
		}
	}

	return NULL;
}


void PersistingDebugRenderer::Square2d(float x, float y, float _size, unsigned int _life, char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
	buf[63] = '\0';

	PersistRenderItem *item = FindItem(buf);
	if (!item)
	{
		item = m_items.GetPointer();
	}

	item->m_life = _life;
	item->m_vect1.x = x;
	item->m_vect1.y = y;
	item->m_size1 = _size;
	item->m_type = TypeSquare2d;

	strncpy(item->m_label, buf, sizeof(item->m_label) - 1);
}


void PersistingDebugRenderer::PointMarker(Vector3 const &_point, unsigned int _life, char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
	buf[63] = '\0';

	PersistRenderItem *item = FindItem(buf);
	if (!item)
	{
		item = m_items.GetPointer();
	}

	item->m_life = _life;
	item->m_vect1 = _point;
	item->m_type = TypePointMarker;

	strncpy(item->m_label, buf, sizeof(item->m_label) - 1);
}


void PersistingDebugRenderer::Sphere(Vector3 const &_centre, float _radius, int _life, char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
	buf[63] = '\0';

	PersistRenderItem *item = FindItem(buf);
	if (!item)
	{
		item = m_items.GetPointer();
	}

	item->m_life = _life;
	item->m_vect1 = _centre;
	item->m_size1 = _radius;
	item->m_type = TypeSphere;
}


void PersistingDebugRenderer::Vector(Vector3 const &_start, Vector3 const &_end, int _life, char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
	buf[63] = '\0';

	PersistRenderItem *item = FindItem(buf);
	if (!item)
	{
		item = m_items.GetPointer();
	}

	item->m_life = _life;
	item->m_vect1 = _start;
	item->m_vect2 = _end;
	item->m_type = TypeVector;
}


void PersistingDebugRenderer::Render()
{
	for (unsigned int i = 0; i < m_items.Size(); ++i)
	{
		if (!m_items.ValidIndex(i)) continue;

		PersistRenderItem *item = &m_items[i];

		switch (item->m_type)
		{
			case TypeSquare2d:
				RenderSquare2d(item->m_vect1.x, item->m_vect1.y, item->m_size1);
				break;
			case TypePointMarker:
				RenderPointMarker(item->m_vect1, item->m_label);
				break;
			case TypeSphere:
				RenderSphere(item->m_vect1, item->m_size1);
				break;
			case TypeVector:
				RenderArrow(item->m_vect1, item->m_vect2, 2.0f);
				break;
		}

		if (item->m_life > 0)
		{
			if (item->m_life == 1)
			{
				m_items.MarkNotUsed(i);
			}
			else
			{
				item->m_life--;
			}
		}
	}
}


#endif // ifdef DEBUG_RENDER_ENABLED

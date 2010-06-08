#include <string.h>
#include <stdlib.h> // for free()

#include "debug_utils.h"
#include "hash_table.h"


//*****************************************************************************
// Protected Functions
//*****************************************************************************

template <class T>
unsigned int HashTable<T>::HashFunc(char const *_key) const
{
	unsigned short rv = 0;

	while (_key[0] && _key[1])
	{
		rv += _key[0] & 0xDF;
		rv += ~(_key[1] & 0xDF);	// 0xDF removes the case bit
		_key += 2;
	}

	if (_key[0])
	{
		rv += *_key & 0xDF;
	}

	return rv & m_mask;
}

//template <class T>
//unsigned int HashTable<T>::HashFunc(char const *_key) const
//{
//	unsigned int rv = 0;
//	unsigned int x = 0;
//
//	while (_key[0] != '\0')
//	{
//		rv += x;
//		x = 0;
//
//		if (_key[0] == '\0') break;
//		x += _key[0];
//		if (_key[1] == '\0') break;
//		x += _key[1] << 8;
//		if (_key[2] == '\0') break;
//		x += _key[2] << 16;
//		if (_key[3] == '\0') break;
//		x += _key[3] << 24;
//
//		_key += 4;
//	}
//
//	return (rv + x) % m_size;
////	return rv & m_mask;
//}


template <class T>
void HashTable<T>::Grow()
{
	DarwiniaReleaseAssert(m_size < 65536, "Hashtable grew too large");
//	if (m_numCollisions * 4 > m_size)
//	{
//		DumpKeys();
//	}

	unsigned int oldSize = m_size;
	m_size *= 2;
	m_mask = m_size - 1;
	char **oldKeys = m_keys;
	m_keys = new char *[m_size];
	T *oldData = m_data;
	m_data = new T [m_size];

	m_numCollisions = 0;

	// Set all m_keys' pointers to NULL
	memset(m_keys, 0, sizeof(char *) * m_size);
	memset(m_data, 0, sizeof(T) * m_size);

	for (unsigned int i = 0; i < oldSize; ++i)
	{
		if (oldKeys[i])
		{
			unsigned int newIndex = GetInsertPos(oldKeys[i]);
			m_keys[newIndex] = oldKeys[i];
			m_data[newIndex] = oldData[i];
		}
	}

	m_slotsFree += m_size - oldSize;

	delete [] oldKeys;
	delete [] oldData;
}


template <class T>
unsigned int HashTable<T>::GetInsertPos(char const *_key) const
{
	unsigned int index = HashFunc(_key);

	// Test if the target slot is empty, if not increment until we
	// find an empty one
	while (m_keys[index] != NULL)
	{
		DarwiniaDebugAssert(stricmp(m_keys[index], _key) != 0);
		index++;
		index &= m_mask;
		m_numCollisions++;
	}

	return index;
}



//*****************************************************************************
// Public Functions
//*****************************************************************************

template <class T>
HashTable<T>::HashTable()
:	m_keys(NULL),
	m_size(4),
	m_numCollisions(0)
{
	m_mask = m_size - 1;
	m_slotsFree = m_size;
	m_keys = new char *[m_size];
	m_data = new T [m_size];

	// Set all m_keys' pointers to NULL
	memset(m_keys, 0, sizeof(char *) * m_size);
	memset(m_data, 0, sizeof(T) * m_size);
}


template <class T>
HashTable<T>::~HashTable()
{
	Empty();

	delete [] m_keys;
	delete [] m_data;
}


template <class T>
void HashTable<T>::Empty()
{
	for (unsigned int i = 0; i < m_size; ++i)
	{
		free(m_keys[i]);
	}
	memset(m_keys, 0, sizeof(char *) * m_size);
	memset(m_data, 0, sizeof(T) * m_size);
}


template <class T>
void HashTable<T>::EmptyAndDelete()
{
	for (unsigned int i = 0; i < m_size; ++i)
	{
		if (m_keys[i])
		{
			delete m_data[i];
		}
	}

	Empty();
}


template <class T>
int HashTable<T>::GetIndex(char const *_key) const
{
	unsigned int index = HashFunc(_key);		// At last profile, was taking an avrg of 550 cycles

	if (m_keys[index] == NULL)
	{
		return -1;
	}

	while (stricmp(m_keys[index], _key) != 0)
	{
		index++;
		index &= m_mask;

		if (m_keys[index] == NULL)
		{
			return -1;
		}
	}

	return index;
}


template <class T>
int HashTable<T>::PutData(char const *_key, T const &_data)
{
	//
	// Make sure the table is big enough

	if (m_slotsFree * 2 <= m_size)
	{
		Grow();
	}


	//
	// Do the main insert

	unsigned int index = GetInsertPos(_key);
	DarwiniaDebugAssert(m_keys[index] == NULL);
	m_keys[index] = strdup(_key);
	m_data[index] = _data;
	m_slotsFree--;

	return index;
}


template <class T>
T HashTable<T>::GetData(char const *_key, T const &_default) const
{
	int index = GetIndex(_key);
	if (index >= 0)
	{
		return m_data[index];
	}

	return _default;
}


template <class T>
T HashTable<T>::GetData(unsigned int _index) const
{
	return m_data[_index];
}


template <class T>
T *HashTable<T>::GetPointer(char const *_key) const
{
	int index = GetIndex(_key);
	if (index >= 0)
	{
		return &m_data[index];
	}

	return NULL;
}


template <class T>
T *HashTable<T>::GetPointer(unsigned int _index) const
{
	return &m_data[_index];
}


template <class T>
void HashTable<T>::RemoveData(char const *_key)
{
	int index = GetIndex(_key);
	if (index >= 0)
	{
		RemoveData(index);
	}
}


template <class T>
void HashTable<T>::RemoveData(unsigned int _index)
{
	//
	// Remove data

	delete [] m_keys[_index];
	m_keys[_index] = NULL;
	m_slotsFree++;
}


template <class T>
bool HashTable<T>::ValidIndex(unsigned int _x) const
{
	return m_keys[_x] != NULL;
}


template <class T>
unsigned int HashTable<T>::Size() const
{
	return m_size;
}


template <class T>
unsigned int HashTable<T>::NumUsed() const
{
	return m_size - m_slotsFree;
}


template <class T>
T HashTable<T>::operator [] (unsigned int _index) const
{
	return m_data[_index];
}


template <class T>
char const *HashTable<T>::GetName(unsigned int _index) const
{
	return m_keys[_index];
}


template <class T>
void HashTable<T>::DumpKeys() const
{
	for (int i = 0; i < m_size; ++i)
	{
		if (m_keys[i])
		{
			int hash = HashFunc(m_keys[i]);
			int numCollisions = (i - hash + m_size) & m_mask;
			DebugOut("%03d: %s - %d\n", i, m_keys[i], numCollisions);
		}
	}
}

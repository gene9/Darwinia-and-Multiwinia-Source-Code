//*****************************************************************************
// Class LListItem
//*****************************************************************************

template <class T>
LListItem<T>::LListItem()
{
    m_next = NULL;
    m_previous = NULL;
}


template <class T>
LListItem<T>::~LListItem()
{
}


//*****************************************************************************
// Class LList
//*****************************************************************************

template <class T>
LList<T>::LList()
{
    m_first = NULL;
    m_last = NULL;
    m_numItems = 0;
    m_previous = NULL;
    m_previousIndex = -1;
}


template <class T>
LList<T>::~LList()
{
    Empty();
}

template <class T>
LList<T>::LList( const LList<T> &source )
	: m_first(NULL), m_last(NULL), 
	  m_previous(NULL), m_previousIndex(-1), 
	  m_numItems(0)
{
	// Deep copy: copy the links, and the items.
	for (int i = 0; i < source.Size(); i++)
		PutDataAtEnd( source.GetData(i) );
}


template <class T>
LList<T> &LList<T>::operator = (const LList<T> &source)
{
	Empty();
	// Deep copy: copy the links, and the items.
	for (int i = 0; i < source.Size(); i++)
	{
		PutDataAtEnd( source.GetData(i) );
	}

	return *this;
}


template <class T>
void LList<T>::PutData( const T &newdata )
{
    PutDataAtEnd( newdata );
}


template <class T>
void LList<T>::PutDataAtEnd( const T &newdata )
{
    // Create the new data entry
    LListItem <T> *li = new LListItem <T> ();
    li->m_data = newdata;
    li->m_next = NULL;
    ++m_numItems;

    if ( m_last == NULL ) 
	{
        // No items added yet - this is the only one

        li->m_previous = NULL;
        m_first = li;
        m_last = li;
    }
    else 
	{
        // Add the data onto the end
        m_last->m_next = li;
        li->m_previous = m_last;
        m_last = li;		
    }

    m_previousIndex = -1;
    m_previous = NULL;
}


template <class T>
void LList<T>::PutDataAtStart( const T &newdata )
{
    // Create the new data entry
    LListItem <T> *li = new LListItem <T> ();
    li->m_data = newdata;
    li->m_previous = NULL;
    ++m_numItems;

    if ( m_last == NULL ) 
	{
        // No items added yet - this is the only one

        li->m_next = NULL;
        m_first = li;
        m_last = li;
    }
    else 
	{
        // Add the data onto the start
        m_first->m_previous = li;
        li->m_next = m_first;
        m_first = li;
    }

    m_previousIndex = -1;
    m_previous = NULL;
}


template <class T>
void LList<T>::PutDataAtIndex( const T &newdata, int index )
{
    if ( index == 0 ) 
	{
        PutDataAtStart ( newdata );
    }
    else if ( index == m_numItems ) 
	{
        PutDataAtEnd ( newdata );
    }
    else 
	{
        // Create the new data entry
        LListItem <T> *li = new LListItem <T> ();
        li->m_data = newdata;		

        LListItem <T> *current = m_first;

        for ( int i = 0; i < index - 1; ++i ) 
		{
            if ( !current )	return;
            current = current->m_next;
        }

        if ( !current )	return;
		
        li->m_previous = current;
        li->m_next = current->m_next;
        if ( current->m_next ) current->m_next->m_previous = li;
        current->m_next = li;		
        ++m_numItems;
        m_previousIndex = -1;
        m_previous = NULL;
    }
}


template <class T>
int LList<T>::Size() const
{
    return m_numItems;
}


template <class T>
T LList<T>::GetData( int index ) const
{
    if ( !ValidIndex(index) ) return (T) 0;

    if ( m_previous && m_previousIndex != -1 && index == m_previousIndex + 1 ) 
	{
        // Last access was to the previous index, so we can access 
        // this one much faster
		
        m_previous = m_previous->m_next;
        m_previousIndex++;

        if ( m_previous == NULL ) return (T) 0;
        else return m_previous->m_data;
    }
    else if ( m_previous && m_previousIndex != -1 && index == m_previousIndex ) 
	{
        // Last access was to this index, so we can 
        // access it directly

        return m_previous->m_data;
    }
    else {
        // We must find the data the long way

        LListItem <T> *current = m_first;

        for ( int i = 0; i < index; ++i ) 
		{
            if ( current == NULL )
                return (T) 0;

            current = current->m_next;
        }

        m_previous = current;	

        if ( current == NULL ) 
		{
            m_previousIndex = -1;
            return (T) 0;
        }
        else 
		{
            m_previousIndex = index;
            return current->m_data;
        }
    }
}


template <class T>
T *LList<T>::GetPointer( int index )
{
    if( !ValidIndex(index) ) return NULL;

    if ( m_previous && m_previousIndex != -1 && index == m_previousIndex + 1 ) 
	{
        // Last access was to the previous index, so we can access 
        // this one much faster
		
        m_previous = m_previous->m_next;
        m_previousIndex++;

        if ( m_previous == NULL ) return NULL;
        else return &(m_previous->m_data);
    }
    else if ( m_previous && m_previousIndex != -1 && index == m_previousIndex ) 
	{
        // Last access was to this index, so we can 
        // access it directly

        return &(m_previous->m_data);
    }
    else 
	{
        // We must find the data the long way

        LListItem <T> *current = m_first;

        for ( int i = 0; i < index; ++i )
        {
            if ( current == NULL ) return NULL;

            current = current->m_next;
        }

        m_previous = current;	

        if ( current == NULL ) 
		{
            m_previousIndex = -1;
            return NULL;
        }
        else 
		{
            m_previousIndex = index;
            return &(current->m_data);
        }
    }
}


template <class T>
T LList<T>::operator [] (int index)
{
    return GetData(index);    
}


template <class T>
bool LList<T>::ValidIndex( int index ) const
{
    return ( index >= 0 && index < m_numItems );
}


template <class T>
void LList<T>::Empty()
{
/*
    LListItem <T> *m_next = 0;
    for (LListItem <T> *p = m_first; p; p = m_next) {
        m_next = p->m_next;
        delete p;
    }
*/

    LListItem <T> *current = m_first;
    while ( current ) 
	{
        LListItem <T> *m_next = current->m_next;
        delete current;
        current = m_next;
    }

    m_first = NULL;
    m_last = NULL;
    m_numItems = 0;
    m_previous = NULL;
    m_previousIndex = -1;
}


template <class T>
void LList<T>::EmptyAndDelete()
{
    while( Size() > 0 )
    {
        T theData = GetData(0);
        RemoveData(0);
        delete theData;
    }
}

template <class T>
void LList<T>::EmptyAndDeleteArray()
{
	while( Size() > 0 )
	{
		T theData = GetData(0);
		RemoveData(0);
		delete[] theData;
	}
}


template <class T>
void LList<T>::RemoveData( int index )
{
    LListItem <T> *current = m_first;

    for ( int i = 0; i < index; ++i ) 
	{
        if ( current == NULL ) return;

        current = current->m_next;
    }

    if ( current == NULL )				return;
    if ( current->m_previous == NULL )	m_first = current->m_next;
    if ( current->m_next == NULL )      m_last = current->m_previous;
    if ( current->m_previous )			current->m_previous->m_next = current->m_next;
    if ( current->m_next )				current->m_next->m_previous = current->m_previous;

    delete current;

    m_previousIndex = -1;  
    m_previous = NULL;

    --m_numItems;
}


template <class T>
int LList<T>::FindData( const T &data )
{
	int const size = Size();
    for ( int i = 0; i < size; ++i ) 
	{
        if ( GetData(i) == data ) 
		{
            return i;
		}
	}

    return -1;
}

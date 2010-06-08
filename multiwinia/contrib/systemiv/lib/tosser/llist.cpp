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
LList<T>::LList(const LList<T> &source)
	: m_first(NULL), m_last(NULL), 
	  m_previous(NULL), m_previousIndex(-1), 
	  m_numItems(0)
{
	// Deep copy: copy the links, and the items.
	for (int i = 0; i < source.Size(); i++)
	{
		PutDataAtEnd(source.GetData(i));
	}
}


template <class T>
LList<T> &LList<T>::operator = (const LList<T> &source)
{
	Empty();
	// Deep copy: copy the links, and the items.
	for (int i = 0; i < source.Size(); i++)
	{
		PutDataAtEnd(source.GetData(i));
	}

	return *this;
}


template <class T>
void LList<T>::PutData(const T &newdata)
{
    PutDataAtEnd(newdata);
}


template <class T>
void LList<T>::PutDataAtEnd(const T &newdata)
{
    // Create the new data entry
    LListItem <T> *li = new LListItem <T> ();
    li->m_data = newdata;
    li->m_next = NULL;
    ++m_numItems;

    if (m_last == NULL) 
	{
        // No items added yet - this is the only one

        li->m_previous = NULL;
        m_first = li;
        m_last = li;

		m_previous = li;
		m_previousIndex = 0;
    }
    else 
	{
        // Add the data onto the end
        m_last->m_next = li;
        li->m_previous = m_last;
        m_last = li;		
    }
}


template <class T>
void LList<T>::PutDataAtStart(const T &newdata)
{
    // Create the new data entry
    LListItem <T> *li = new LListItem <T> ();
    li->m_data = newdata;
    li->m_previous = NULL;
    ++m_numItems;

    if (m_last == NULL) 
	{
        // No items added yet - this is the only one

        li->m_next = NULL;
        m_first = li;
        m_last = li;

		m_previous = li;
		m_previousIndex = 0;
    }
    else 
	{
        // Add the data onto the start
        m_first->m_previous = li;
        li->m_next = m_first;
        m_first = li;

		m_previousIndex++;
    }
}


template <class T>
void LList<T>::PutDataAtIndex(const T &newdata, int index)
{
    if (index == 0) 
	{
        PutDataAtStart (newdata);
    }
    else if (index == m_numItems) 
	{
        PutDataAtEnd (newdata);
    }
    else 
	{
        LListItem <T> *current = m_first;

        for (int i = 0; i < index - 1; ++i) 
		{
            if (!current)	return;
            current = current->m_next;
        }

        if (!current)	return;
		
        // Create the new data entry
        LListItem <T> *li = new LListItem <T> ();
        li->m_data = newdata;		
        li->m_previous = current;
        li->m_next = current->m_next;
        if (current->m_next) current->m_next->m_previous = li;
        current->m_next = li;		
        ++m_numItems;
        
		m_previousIndex = 0;
        m_previous = m_first;
    }
}


template <class T>
int LList<T>::Size() const
{
    return m_numItems;
}


template <class T>
T LList<T>::GetData(int index) const
{
    LListItem<T> const *item = GetItem(index);
    if (item)
	{
	    return item->m_data;
	}

	return (T) 0;
}


template <class T>
T *LList<T>::GetPointer(int index) const
{
    LListItem<T> *item = GetItem(index);
    if (item)
	{
	    return &item->m_data;
	}

	return NULL;
}


template <class T>
LListItem<T> *LList<T>::GetItem(int index) const
{
    if (!ValidIndex(index)) return NULL;


	//
	// Choose a place for which to start walking the list
	
	// Best place to start is either; m_first, m_previous or m_last
	//
	// m_first                m_previous                                   m_last
    //     |----------:-----------|---------------------:--------------------|
	//            mid-point 1                      mid-point 2
    //     
	// If index is less than mid-point 1, then m_first is nearest.
	// If index is greater than mid-point 2, then m_last is nearest.
	// Otherwise m_previous is nearest.
	// The two if statements below test for these conditions.

	if (index <= (m_previousIndex >> 1))
	{
		// m_first is nearest target
		m_previous = m_first;
		m_previousIndex = 0;
	}
	else if ((index - m_previousIndex) > (m_numItems - index))
	{
		// m_last is nearest target
		m_previous = m_last;
		m_previousIndex = m_numItems - 1;
	}
	// Otherwise m_previous is nearest

    
    // QUICK FIX BY CHRIS
    // Note : SOmetimes it would reach this point and m_previous == NULL
    // which would cause a crash

    if( !m_previous )
    {
        m_previous = m_first;
        m_previousIndex = 0;
    }


	//
	// Now walk up or down the list until we find our target
	
	while (index > m_previousIndex) 
	{
		m_previous = m_previous->m_next;
		m_previousIndex++;
	}
	while (index < m_previousIndex) 
	{
		m_previous = m_previous->m_previous;
		m_previousIndex--;
	}


	return m_previous;
}


template <class T>
T LList<T>::operator [] (int index)
{
    return GetData(index);    
}


template <class T>
bool LList<T>::ValidIndex(int index) const
{
    return (index >= 0 && index < m_numItems);
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
    while (current) 
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
    while(Size() > 0)
    {
        T theData = GetData(0);
        RemoveData(0);
        delete theData;
    }
}


template <class T>
void LList<T>::RemoveData(int index)
{
    LListItem <T> *current = GetItem(index);

    if (current == NULL)				return;
    
	if (current->m_previous == NULL)	m_first = current->m_next;
    else								current->m_previous->m_next = current->m_next;
    
	if (current->m_next == NULL)		m_last = current->m_previous;
    else								current->m_next->m_previous = current->m_previous;

	if (index == m_previousIndex)
	{
		if (m_numItems == 1)
		{
			m_previousIndex = -1;  
			m_previous = NULL;
		}
		else if (index > 0)
		{
			m_previousIndex--;
			m_previous = current->m_previous;
		}
		else
		{
			m_previous = current->m_next;
		}
	}
	else if (index < m_previousIndex)
	{
		m_previousIndex--;
	}

    delete current;
    --m_numItems;
}


template <class T>
void LList<T>::RemoveDataAtEnd()
{
	RemoveData(m_numItems - 1);
}


template <class T>
int LList<T>::FindData(const T &data)
{
	int const size = Size();
    for (int i = 0; i < size; ++i) 
	{
        if (GetData(i) == data) 
		{
            return i;
		}
	}

    return -1;
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


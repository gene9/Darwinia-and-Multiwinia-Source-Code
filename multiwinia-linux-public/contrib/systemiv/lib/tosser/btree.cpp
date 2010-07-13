#include <string.h>
#include <iostream>

#include "lib/debug_utils.h"
#include "btree.h"

// No stricmp on linux
#ifdef TARGET_OS_LINUX
#define stricmp strcasecmp
#endif


template <class T>
BTree<T>::BTree()
{
    ltree = NULL;
    rtree = NULL;
    id = NULL;
    data = NULL;
}


template <class T>
BTree<T>::BTree( const char *newid, const T &newdata )
{
    ltree = NULL;
    rtree = NULL;
    id = new char[ strlen(newid) + 1 ];
	
#ifdef _WIN32
#pragma warning( suppress : 4996 )
#endif
	
    strcpy( id, newid );
    data = newdata;
}


template <class T>
BTree<T>::BTree( const BTree<T> &copy )
{
    Copy( copy );
}


template <class T>
void BTree<T>::Copy( const BTree<T> &copy )
{
    if(copy.ltree)	ltree = new BTree( *copy.ltree );
    else ltree = NULL;
    
    if(copy.rtree)	rtree = new BTree( *copy.rtree );
    else rtree = NULL;
    
    if(copy.id) {
		id = new char[ strlen(copy.id) + 1 ];
		strcpy( id, copy.id );
    }
    else 
		id = NULL;
    
    data = copy.data;
}


template <class T>
BTree<T>::~BTree()
{
    Empty();
}


template <class T>
void BTree<T>::Empty()
{
    delete ltree;
    delete rtree;
    delete id;
    
    ltree = rtree = NULL;
    id = NULL;
}


template <class T>
void BTree<T>::PutData( const char *newid, const T &newdata )
{
    if( !id ) 
	{
        id = new char[ strlen( newid ) + 1 ];
#ifdef _WIN32
#pragma warning( suppress : 4996 )
#endif
        strcpy( id, newid );
        data = newdata;
	 	 
		return;
    }

    int compareResult = stricmp( newid, id );

    if( compareResult <= 0 ) 
	{
        if(ltree)
            ltree->PutData( newid, newdata );
        else
            ltree = new BTree( newid, newdata );
    }
    else if( compareResult > 0 ) 
	{
        if(rtree)
            rtree->PutData( newid, newdata );
        else
            rtree = new BTree( newid, newdata );
    }
}


template <class T>
void BTree<T>::RemoveData( const char *newid )
{
    /*
      Deletes an element from the list
      By replacing the element with it's own left node, then appending
      its own right node onto the extreme right of itself.
      */
    
    AppAssert(newid);
    
    int compareResult = stricmp( newid, id );

    if( compareResult == 0 ) 
	{
		//var tempright : pointer to node := data->right
		BTree<T> *tempright = Right();       
			
			//data := data->left
		if( Left() ) 
		{
			id = new char[strlen(Left()->id) + 1];
#ifdef WIN32
			#pragma warning( suppress : 4996 )
#endif
			strcpy( id, Left()->id );
			data = Left()->data;                  // This bit requires a good copy constructor
			rtree = Left()->Right();
			ltree = Left()->Left();	
			
			AppendRight( tempright );
		}
		else 
		{
			//append_right( data, tempright )
			
			if( Right() ) 
			{
				id = new char[strlen(Right()->id) + 1];
#ifdef WIN32
				#pragma warning( suppress : 4996 )
#endif
				strcpy( id, Right()->id );
				data = Right()->data;                  // This bit requires a good copy constructor
				ltree = Right()->Left();	
				rtree = Right()->Right();
			}
			else 
			{
				id = NULL;                              // Hopefully this is the root node
			}	    
		}
    }                                                   //elsif Name < data->name then
    else if( compareResult < 0 ) 
    {
        if( Left() )
        {
            if( stricmp( Left()->id, newid ) == 0 && !Left()->Left() && !Left()->Right() )
                ltree = NULL;
            else
		        Left()->RemoveData( newid );
        }
    }                    
    else                                                //elsif Name > data->name then
    {
	    if( Right() )
        {
            if( stricmp( Right()->id, newid ) == 0 && !Right()->Left() && !Right()->Right() )
                rtree = NULL;
            else
		        Right()->RemoveData( newid );
        }
    }
                             
}


template <class T>
T BTree<T>::GetData( const char *searchid )
{
	BTree<T> *btree = LookupTree( searchid );

	if( btree ) return btree->data;
	else return 0;
}


template <class T>
void BTree<T>::AppendRight( BTree<T> *tempright )
{
    if( !Right() )
		rtree = tempright;
    
    else
		Right()->AppendRight( tempright );	
}


template <class T>
BTree<T> *BTree<T>::LookupTree( const char *searchid )
{
    if(!id)
		return NULL;
    
    int compareResult = stricmp( searchid,id );

    if( compareResult == 0 )
        return this;

    else if( ltree && compareResult < 0 )
        return ltree->LookupTree( searchid );

    else if( rtree && compareResult > 0 )
        return rtree->LookupTree( searchid );
    
    else return NULL;
}


template <class T>
int BTree<T>::Size() const
{
    unsigned int subsize =(id) ? 1 : 0;
    
    if(ltree) subsize += ltree->Size();
    if(rtree) subsize += rtree->Size();

    return subsize;
}


template <class T>
void BTree<T>::Print()
{
    if(ltree) ltree->Print();
    if(id) std::cout << id << " : " << data << "\n";       
    if(rtree) rtree->Print();
}

    
template <class T>
BTree<T> *BTree<T>::Left() const
{
    return(BTree<T>*) ltree;
}


template <class T>
BTree<T> *BTree<T>::Right() const
{
    return(BTree<T>*) rtree;
}


template <class T>
DArray <T> *BTree<T>::ConvertToDArray()
{
    DArray <T> *darray = new DArray <T>;
	darray->SetSize( Size() );
    RecursiveConvertToDArray( darray, this );
    
    return darray;
}


template <class T>
DArray <char *> *BTree<T>::ConvertIndexToDArray()
{
    DArray <char *> *darray = new DArray <char *>;
	darray->SetSize( Size() );
    RecursiveConvertIndexToDArray( darray, this );
    
    return darray;
}


template <class T>
void BTree<T>::RecursiveConvertToDArray( DArray <T> *darray, BTree<T> *btree )
{
    AppAssert(darray);
    
    if( !btree ) return;            // Base case
    
    if( btree->id ) darray->PutData( btree->data );
    
    RecursiveConvertToDArray( darray, btree->Left () );
    RecursiveConvertToDArray( darray, btree->Right() );
}


template <class T>
void BTree<T>::RecursiveConvertIndexToDArray( DArray <char *> *darray, BTree<T> *btree )
{
    AppAssert(darray);
    
    if( !btree ) return;            // Base case
    
    if( btree->id ) darray->PutData( btree->id );
    
    RecursiveConvertIndexToDArray( darray, btree->Left () );
    RecursiveConvertIndexToDArray( darray, btree->Right() );
}

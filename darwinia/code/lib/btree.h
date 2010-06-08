//===============================================================//
//                        BINARY TREE                            //
//                                                               //
//                   By Christopher Delay                        //
//                           V1.3                                //
//===============================================================//

#ifndef _included_binary_tree_h
#define _included_binary_tree_h


#include "darray.h"


//=================================================================
// Binary tree object
// Use : A sorted dynamic data structure
// Every data item has a string id which is used for ordering
// Allows very fast data lookups

template <class T>
class BTree
{
protected:
    BTree *ltree;
    BTree *rtree;

	bool m_caseSensitive;
		
    void RecursiveConvertToDArray ( DArray <T> *darray, BTree <T> *btree );
    void RecursiveConvertIndexToDArray ( DArray <char *> *darray, BTree <T> *btree );
    
    void AppendRight ( BTree <T> *tempright );                            // Used by Remove
    
public:
    char *id;
    T data;

    BTree ();
    BTree ( const char *newid, const T &newdata );
    BTree ( const BTree<T> &copy );

    ~BTree ();
    void Copy( const BTree<T> &copy );

    void PutData ( const char *newid, const T &newdata );
    void RemoveData ( const char *newid );                     // Requires a solid copy constructor in T class
    T GetData ( const char *searchid );

    BTree *LookupTree( const char *searchid );
    
    void Empty ();
    
    int Size () const;							 // Returns the size in elements
    
    void Print ();                              // Prints this tree to stdout
    
    inline BTree *Left () const;
    inline BTree *Right () const;
    
    DArray <T> *ConvertToDArray ();
    DArray <char *> *ConvertIndexToDArray ();
};



//  ===================================================================

#include "btree.cpp"

#endif

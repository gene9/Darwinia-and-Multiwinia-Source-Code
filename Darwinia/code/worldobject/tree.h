
#ifndef _included_tree_h
#define _included_tree_h

#include "worldobject/building.h"


class Tree : public Building
{
protected:
    int     m_branchDisplayListId;
    int     m_leafDisplayListId;
    void    RenderBranch   ( Vector3 _from, Vector3 _to, int _iterations,
                             bool _calcRadius, bool _renderBranch, bool _renderLeaf );

    Vector3 m_hitcheckCentre;
    float   m_hitcheckRadius;
    int     m_numLeafs;

    float   m_fireDamage;
    float   m_onFire;
    bool    m_burnSoundPlaying;

    float   GetActualHeight( float _predictionTime );

	unsigned char m_leafColourArray[4];
	unsigned char m_branchColourArray[4];

public:
    float   m_height;
    float   m_budsize;
    float   m_pushUp;
    float   m_pushOut;
    int     m_iterations;
    int     m_seed;
    int     m_leafColour;
    int     m_branchColour;
	int		m_leafDropRate;
	int		m_spiritDropRate;

public:
    Tree();
	~Tree();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );

    bool Advance        ();

	void DeleteDisplayLists();
    void Generate       ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );
    void RenderHitCheck ();

    bool PerformDepthSort( Vector3 &_centrePos );

    void Damage         ( float _damage );

    bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir,
                                 float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    void ListSoundEvents( LList<char *> *_list );

    void Read           ( TextReader *_in, bool _dynamic );
    void Write          ( FileWriter *_out );
};


#endif

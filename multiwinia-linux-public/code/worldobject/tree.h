
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

	void	GenerateLeaves();
	void	GenerateBranches();

    Vector3 m_hitcheckCentre;
    double   m_hitcheckRadius;
    int     m_numLeafs;

    double   m_fireDamage;
    double   m_onFire;
    bool    m_burnSoundPlaying;
    
    double   GetActualHeight( double _predictionTime );

	unsigned char m_leafColourArray[4];
	unsigned char m_branchColourArray[4];
	
public:
    double   m_height;
    double   m_budsize;
    double   m_pushUp;
    double   m_pushOut;
    int     m_iterations;
    int     m_seed;
    int     m_leafColour;
    int     m_branchColour;
	int		m_leafDropRate;
    int     m_spiritDropRate;
    
    int     m_spawnsRemaining;                       // Time to spread and create another tree
    double  m_spawnTimer;
    bool    m_destroyable;
    bool    m_evil;
    float   m_corruptCheckTimer;
    bool    m_renderCorruptShadow;
    bool    m_corrupted;

public:
    Tree();
	~Tree();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );
    void Clone          ( Tree *tree );

    bool Advance        ();

	void DeleteDisplayLists();
    void Generate       ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );
    void RenderHitCheck ();

    bool PerformDepthSort( Vector3 &_centrePos );

    void Damage         ( double _damage );
    void SetFireAmount  ( double _amount );
    double GetFireAmount ();
    bool IsOnFire();
    double GetBurnRange();

    void CreateAnotherTree();

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    void ListSoundEvents( LList<char *> *_list );

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );	

    char *LogState       ( char *message=NULL );

    bool IsInView();
};


#endif

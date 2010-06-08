
/*
 * =======
 * GESTURE
 * =======
 *
 * A gesture recognition system designed
 * to convert mouse movements into symbols
 *
 */

#ifndef _included_gesture_h
#define _included_gesture_h

#include "lib/tosser/darray.h"

#define GESTURE_MAX_MOUSE_SAMPLES       1024
#define GESTURE_MAX_FEATURES            13
#define GESTURE_MAX_TRAINERS            32
#define GESTURE_MIN_SAMPLE_DISTANCE     6.0

#define GESTURE_CONFIDENCETHRESHOLD     0.5
#define GESTURE_MAHALANOBISTHRESHOLD    3000.0


// struct MouseSample
struct MouseSample
{   
    int             x;
    int             y;
    double          time;
};


// class FeatureSet
class FeatureSet
{
public:
    FeatureSet      ();
    double          f[GESTURE_MAX_FEATURES+1];          // Features are numbered 1 to MAX_FEATURES inclusive          
};


// class Classification
class Classification
{
public:
    Classification  ();
    FeatureSet      m_trainers[GESTURE_MAX_TRAINERS];                  // Pre-provided examples
    int             m_nextTrainer;
    FeatureSet      m_weights;                                        // Weight values for each result
};


// class GestureSymbol
class GestureSymbol
{
public:
    GestureSymbol   ();
    void            ClearResults();
    
    FeatureSet      m_averages;
    Classification  m_classes;
    double          m_results;
    double          m_confidence;
    double          m_mahalanobis;
};


// ============================================================================


class Gesture
{
protected:

    MouseSample     m_mouseSamples[GESTURE_MAX_MOUSE_SAMPLES];
    int             m_nextMouseSample;
    bool            m_recordingGesture;

    FeatureSet      m_featureSet;
    double          m_invertedCovarianceMatrix[GESTURE_MAX_FEATURES+1][GESTURE_MAX_FEATURES+1];

    DArray          <GestureSymbol> m_symbols;

    bool            CalculateFeatureSet();
    void            TrainSystem();
    void            RunComparison();

public:
    
    Gesture( char *_filename );

    //
    // Training

    void LoadTrainingData    ( char *filename );
    void SaveTrainingData    ( char *filename );
    void AddTrainingSample   ( int symbolID );
    int  GetNumTrainers      ( int symbolID );
    int  GetNumSymbols       ();
    void NewSymbol           ();

    //
    // Recording samples

    void ClearMouseSamples   ();
    void ClearResults        ();
    void BeginGesture        ();
    void AddSample           ( int x, int y );
    bool IsRecordingGesture  ();
    void EndGesture          ();

    //
    // Getting the results

    int      GetSymbolID     ();
    bool     GestureSuccess  ();
    double   GetConfidence   ();
    double   GetMahalanobis  ();

    //
    // Low level access

    int  GetNumMouseSamples  ();
    void GetMouseSample      ( int i, int *x, int *y, int *t=NULL );
    void GetMouseSample      ( int i, float *x, float *y, float *t=NULL );

    double GetCalculatedResult       ( int symbolID );
    double GetCalculatedConfidence   ( int symbolID );
    double GetMahalanobisDistance    ( int symbolID );

};



#endif

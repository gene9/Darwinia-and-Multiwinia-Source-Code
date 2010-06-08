#include "lib/universal_include.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/invert_matrix.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"

#include "app.h"
#include "gesture.h"


/*
 * Mouse Sample
 * Represents a single sampled mouse point
 * Also time stamped
 */


/*
 * Feature Set
 * A set of calculated data that represents 
 * a complete gesture.
 *
    double   f1;                         // Cosine of angle between sample[0] and sample[2]
    double   f2;                         // Sine of angle between sample[0] and sample[2]
    double   f3;                         // Length of bounding box diagonal
    double   f4;                         // Angle of bounding box diagonal
    double   f5;                         // Distance between first and last point
    double   f6;                         // Cosine of the angle of the line between first and last point
    double   f7;                         // Sine of the angle of the line between first and last point
    double   f8;                         // Total gesture length
    double   f9;                         // Total angle traversed
    double   f10;                        // Sum of the absolute value of the angle at each mouse Sample
    double   f11;                        // Sum of the squared value of the angle at each mouse Sample
    double   f12;                        // Maximum speed of the gesture (squared)
    double   f13;                        // Total duration of the gesture
 */



/*
 * Classification
 *
 * Data representing a trained classification
 * with all data needed for additional training
 */


/*
 * DATA
 *
 * All of our internal static data
 */

FeatureSet::FeatureSet()
{
    memset( f, 0, (GESTURE_MAX_FEATURES+1) * sizeof(double) );
}

Classification::Classification()
{
    m_nextTrainer = 0;
}

GestureSymbol::GestureSymbol()
{
    ClearResults();
}

void GestureSymbol::ClearResults()
{
    m_results = 0.0f;
    m_confidence = 0.0f;
    m_mahalanobis = 0.0f;
}


/*
 * Calculate Feature Set
 * Looks at the set of mouse samples,
 * Performs calculations on them and fills in the
 * feature set
 */
bool Gesture::CalculateFeatureSet()
{

    if( m_nextMouseSample < 3 )
    {
        printf( "Sorry, not enough mouse samples\n" );
        return false;
    }


    //
    // Compute f1 : cosine of angle between sample[0] and sample[2]
    // (using first and third point is apparently less noisy than first and second point)
    double startDX = double (m_mouseSamples[2].x - m_mouseSamples[0].x);
    double startDY = double (m_mouseSamples[2].y - m_mouseSamples[0].y);
    m_featureSet.f[1] = startDX / sqrt( startDX * startDX + startDY * startDY );
    

    //
    // Compute f2 : sine of angle between sample[0] and sample[2]
    m_featureSet.f[2] = startDY / sqrt( startDX * startDX + startDY * startDY );


    //
    // Compute f3 : Length of bounding box diagonal
    
    double boundL = (double) 999999;
    double boundR = 0.0f;
    double boundT = (double) 999999;
    double boundB = 0.0f;
    for( int i = 0; i < m_nextMouseSample; ++i )
    {
        if( m_mouseSamples[i].x < boundL ) boundL = m_mouseSamples[i].x;
        if( m_mouseSamples[i].x > boundR ) boundR = m_mouseSamples[i].y;
        if( m_mouseSamples[i].y < boundT ) boundT = m_mouseSamples[i].y;
        if( m_mouseSamples[i].y > boundB ) boundB = m_mouseSamples[i].y;
    }
    m_featureSet.f[3] = sqrt( (boundR - boundL) * (boundR - boundL) + (boundB - boundT) * (boundB - boundT) );


    //
    // Compute f4 : angle of bounding box diagonal
    m_featureSet.f[4] = atan( (boundB - boundT) / (boundR - boundL) );


    //
    // Compute f5 : Distance between first and last point;
    double firstToLastDX = double (m_mouseSamples[m_nextMouseSample-1].x - m_mouseSamples[0].x);
    double firstToLastDY = double (m_mouseSamples[m_nextMouseSample-1].y - m_mouseSamples[0].y);
    m_featureSet.f[5] = sqrt( firstToLastDX * firstToLastDX + firstToLastDY * firstToLastDY );


    // 
    // Compute f6 : Cosine of the angle of the line between first and last point
    m_featureSet.f[6] = firstToLastDX / m_featureSet.f[5];


    //
    // Compute f7 : Sine of the angle of the line between first and last point
    m_featureSet.f[7] = firstToLastDY / m_featureSet.f[5];


    //
    // Compute f8 : Total gesture length
    double totalGestureLength = 0.0f;
    for( int j = 0; j < m_nextMouseSample-2; ++j )
    {
        double dX = m_mouseSamples[j+1].x - m_mouseSamples[j].x;
        double dY = m_mouseSamples[j+1].y - m_mouseSamples[j].y;
        totalGestureLength += sqrt( dX * dX + dY * dY );
    }
    m_featureSet.f[8] = totalGestureLength;


    //
    // Compute f9 : Total Angle traversed;
    // Compute f10 : Sum of the absolute values of those angles
    // Compute f11 : Sum of the squared values of those angles
    double totalAngleTraversed = 0.0f;
    double sumAbsoluteAngles = 0.0f;
    double sumSquaredAngles = 0.0f;
    for( int k = 1; k < m_nextMouseSample-1; ++k )
    {
        double a_dX = m_mouseSamples[k+1].x - m_mouseSamples[k].x;
        double a_dY = m_mouseSamples[k+1].y - m_mouseSamples[k].y;
        double b_dX = m_mouseSamples[k].x - m_mouseSamples[k-1].x;
        double b_dY = m_mouseSamples[k].y - m_mouseSamples[k-1].y;

        double angle = atan( (a_dX * b_dY - b_dX * a_dY ) / 
                            (a_dX * b_dX + a_dY * b_dY ) );
        
        totalAngleTraversed += angle;
        sumAbsoluteAngles += fabs(angle);
        sumSquaredAngles += (angle * angle);
    }
    m_featureSet.f[9] = totalAngleTraversed;
    m_featureSet.f[10] = sumAbsoluteAngles;
    m_featureSet.f[11] = sumSquaredAngles;


    //
    // Compute f12 : Maximum speed of the gesture(squared)
    // Compute f13 : Total duration of the gesture
    double maxSpeed = 0.0;
    double totalDuration = 0.0;
    for( int l = 0; l < m_nextMouseSample-1; ++l )
    {
        double dX = m_mouseSamples[l+1].x - m_mouseSamples[l].x;
        double dY = m_mouseSamples[l+1].y - m_mouseSamples[l].y;
        double dT = m_mouseSamples[l+1].time - m_mouseSamples[l].time;
        double thisSpeed = ( dX * dX + dY * dY ) / 
                          ( dT * dT );
        if( dT == 0.0 )
            continue;
        if( thisSpeed > maxSpeed ) maxSpeed = thisSpeed;
        totalDuration += dT;
    }
    m_featureSet.f[12] = maxSpeed;
    m_featureSet.f[13] = totalDuration;


    return true;
}

/*
 * Train System
 * Runs through all the recorded sample data
 * and sets up the weights that train the system
 */
void Gesture::TrainSystem()
{

    //
    // Calculate matrices and averages for each class

    int numSymbols = GetNumSymbols();

    for( int i = 0; i < numSymbols; ++i )
    {
        memset( &m_symbols[i].m_averages, 0, sizeof(FeatureSet) );
    }

    struct coVarianceElement
    {
        double z[GESTURE_MAX_FEATURES+1][GESTURE_MAX_FEATURES+1];
    };

    coVarianceElement *coVarianceMatrix = new coVarianceElement[numSymbols];
    memset( coVarianceMatrix, 0, numSymbols * sizeof(coVarianceElement) );    

    double commonCovarianceMatrix[GESTURE_MAX_FEATURES+1][GESTURE_MAX_FEATURES+1];
    memset( commonCovarianceMatrix, 0, (GESTURE_MAX_FEATURES+1) * (GESTURE_MAX_FEATURES+1) * sizeof(double) );

    memset( m_invertedCovarianceMatrix, 0, (GESTURE_MAX_FEATURES+1) * (GESTURE_MAX_FEATURES+1) * sizeof(double) );


    for( int c = 0; c < numSymbols; ++c )
    {

        Classification *theClass = &m_symbols[c].m_classes;

        //
        // Recalculate our averages

        for( int i = 0; i <= GESTURE_MAX_FEATURES; ++i )
        {
            double result = 0.0f;
            for( int e = 0; e < theClass->m_nextTrainer; ++e )
            {
                result += theClass->m_trainers[e].f[i];
            }
            result /= theClass->m_nextTrainer;

            m_symbols[c].m_averages.f[i] = result;
        }


        //
        // Calculate the sample estimate of the Covariance matrix for each Class

        for( int i = 0; i <= GESTURE_MAX_FEATURES; ++i )
        {
            for( int j = 0; j <= GESTURE_MAX_FEATURES; ++j )
            {
                double result = 0.0f;
        
                for( int e = 0; e < theClass->m_nextTrainer; ++e )            
                {
                    result += ( (theClass->m_trainers[e].f[i] - m_symbols[c].m_averages.f[i]) *
                                (theClass->m_trainers[e].f[j] - m_symbols[c].m_averages.f[j]) );
                }

                coVarianceMatrix[c].z[i][j] = result;
            }
        }
    }

    //
    // Calculate the common coVariance Matrix

    double trainerSum = -numSymbols;
    for( int c = 0; c < numSymbols; ++c )
    {
        trainerSum += m_symbols[c].m_classes.m_nextTrainer;
    }

    for( int i = 0; i <= GESTURE_MAX_FEATURES; ++i )
    {
        for( int j = 0; j <= GESTURE_MAX_FEATURES; ++j )
        {
            double coVarianceSum = 0.0f;
            for( int c = 0; c < numSymbols; ++c )
            {
                coVarianceSum += coVarianceMatrix[c].z[i][j] /
                                 (double) m_symbols[c].m_classes.m_nextTrainer-1.0f;
            }
            commonCovarianceMatrix[i][j] = coVarianceSum / trainerSum;
        }
    }

    //
    // Invert the common coVariance Matrix

    InvertMatrix( (double *) commonCovarianceMatrix, 
                  (double *) m_invertedCovarianceMatrix, 
                  GESTURE_MAX_FEATURES+1, GESTURE_MAX_FEATURES+1 );

    //
    // Use that inverted matrix to calculate our weights

    for( int c = 0; c < numSymbols; ++c )
    {
        for( int j = 1; j <= GESTURE_MAX_FEATURES; ++j )
        {
            double total = 0.0f;
            for( int i = 1; i <= GESTURE_MAX_FEATURES; ++i )
            {
                total += ( m_invertedCovarianceMatrix[i][j] *
                           m_symbols[c].m_averages.f[i] );
            }                
            m_symbols[c].m_classes.m_weights.f[j] = total;
        }
    }

    
    for( int c = 0; c < numSymbols; ++c )
    {
        // Calculate weight 0
        double total = 0.0f;
        for( int i = 1; i <= GESTURE_MAX_FEATURES; ++i )
        {
            total += ( m_symbols[c].m_classes.m_weights.f[i] *
                       m_symbols[c].m_averages.f[i] );
        }
        total *= -0.5f;
        m_symbols[c].m_classes.m_weights.f[0] = total;
    }    

	delete coVarianceMatrix;
}


void Gesture::AddTrainingSample( int symbolID )
{
    Classification *newDataClass = &m_symbols[symbolID].m_classes;
    
    //
    // Put the data into the Classification

    newDataClass->m_trainers[newDataClass->m_nextTrainer] = m_featureSet;
    newDataClass->m_nextTrainer++;
    DarwiniaDebugAssert( newDataClass->m_nextTrainer < GESTURE_MAX_TRAINERS );

    TrainSystem();
}

/*
 * Run Comparison
 * Compares the currently recorded feature set against all classes.
 */
void Gesture::RunComparison()
{
    int numSymbols = GetNumSymbols();

    for( int c = 0; c < numSymbols; ++c )
    {
        Classification *theClass = &m_symbols[c].m_classes;
        if( theClass->m_nextTrainer > 0 )
        {
            double thisResult = 0.0;
            thisResult += theClass->m_weights.f[0];

            for( int i = 1; i <= GESTURE_MAX_FEATURES; ++i )
            {
                thisResult += theClass->m_weights.f[i] * m_featureSet.f[i];                
            }
            
            m_symbols[c].m_results = thisResult;
        }
        else
        {
            m_symbols[c].m_results = 0.0;
        }
    }

    for( int i = 0; i < numSymbols; ++i )
 

    
    //
    // Calculate confidence of results

    for( int i = 0; i < numSymbols; ++i )
    {
        double total = 0.0;
        for( int j = 0; j < numSymbols; ++j )
        {
            double diff = m_symbols[j].m_results - m_symbols[i].m_results;                
            double expResult = exp( diff / 500.0 );
            total += expResult;  
        }

        m_symbols[i].m_confidence = 1.0 / total;
    }

    
    //
    // Calculate Mahalanobis distance of results

    for( int i = 0; i < numSymbols; ++i )
    {
        double total = 0;
        for( int j = 1; j < GESTURE_MAX_FEATURES; ++j )
        {
            for( int k = 1; k < GESTURE_MAX_FEATURES; ++k )
            {
                total += m_invertedCovarianceMatrix[j][k] *
                         ( m_featureSet.f[j] - m_symbols[i].m_averages.f[j] ) *
                         ( m_featureSet.f[k] - m_symbols[i].m_averages.f[k] );
            }
        }
        m_symbols[i].m_mahalanobis = total;
    }
}

void Gesture::LoadTrainingData( char *filename )
{
    TextReader *reader = g_app->m_resource->GetTextReader(filename);
    DarwiniaReleaseAssert(reader && reader->IsOpen(), "Couldn't open Gesture data file");

	reader->ReadLine();
    int numSymbols = atoi(reader->GetNextToken());
    m_symbols.SetSize( numSymbols );
    
    for( int c = 0; c < numSymbols; ++c )
    {        
        m_symbols.PutData( GestureSymbol(), c );
        Classification *theClass = &m_symbols[c].m_classes;

		reader->ReadLine();
        theClass->m_nextTrainer = atoi(reader->GetNextToken());
		if( theClass->m_nextTrainer > 0 )
        {
            reader->ReadLine();
            for( int t = 0; t < theClass->m_nextTrainer; ++t )
            {
                FeatureSet *fs = &theClass->m_trainers[t];
                for( int f = 0; f <= GESTURE_MAX_FEATURES; ++f )
                {
                    fs->f[f] = atof(reader->GetNextToken());
                }
            }
        }
    }

    delete reader;

    TrainSystem();
}

void Gesture::SaveTrainingData( char *filename )
{
    FileWriter *file = g_app->m_resource->GetFileWriter( filename, false );

    int numSymbols = GetNumSymbols();
    file->printf( "%d\n", numSymbols );

    for( int c = 0; c < numSymbols; ++c )
    {
        Classification *theClass = &m_symbols[c].m_classes;

        file->printf( "%d\n", theClass->m_nextTrainer );
        for( int t = 0; t < theClass->m_nextTrainer; ++t )
        {
            FeatureSet *fs = &theClass->m_trainers[t];
            for( int f = 0; f <= GESTURE_MAX_FEATURES; ++f )
            {
                float temp = (float) fs->f[f];
                file->printf( "%f ", temp );
            }
        }

        file->printf( "\n" );
    }

    delete file;
}

Gesture::Gesture( char *_filename )
{

    //
    // Set up our Mouse sample data

    memset( m_mouseSamples, 0, GESTURE_MAX_MOUSE_SAMPLES * sizeof(MouseSample) );
    memset( &m_featureSet, 0, sizeof(FeatureSet) );
        
    m_nextMouseSample = 0;
    m_recordingGesture = false;


    //
    // Load data

    if( _filename )
    {
        LoadTrainingData( _filename );
    }
}

void Gesture::ClearMouseSamples()
{
    m_nextMouseSample = 0;
}

void Gesture::ClearResults()
{
    memset( &m_featureSet, 0, sizeof(FeatureSet) );

    for( int i = 0; i < GetNumSymbols(); ++i )
    {
        m_symbols[i].ClearResults();
    }
}

void Gesture::BeginGesture()
{
    ClearMouseSamples();
    ClearResults();
    m_recordingGesture = true;
}

void Gesture::AddSample( int x, int y )
{
    bool tooClose = false;

    if( m_nextMouseSample != 0 )
    {
        double dX = double (x - m_mouseSamples[m_nextMouseSample-1].x);
        double dY = double (y - m_mouseSamples[m_nextMouseSample-1].y);
        double D = sqrt( dX * dX + dY * dY );
        if( D < GESTURE_MIN_SAMPLE_DISTANCE ) tooClose = true;
    }

    if( !tooClose )
    {
        m_mouseSamples[m_nextMouseSample].x = x;
        m_mouseSamples[m_nextMouseSample].y = y;
        m_mouseSamples[m_nextMouseSample].time = GetHighResTime() * 1000.0;
        ++m_nextMouseSample;
        DarwiniaDebugAssert( m_nextMouseSample < GESTURE_MAX_MOUSE_SAMPLES );
    }
}

bool Gesture::IsRecordingGesture()
{
    return m_recordingGesture;
}

void Gesture::EndGesture()
{
    m_recordingGesture = false;
    CalculateFeatureSet();
    RunComparison();
}

int Gesture::GetNumMouseSamples()
{
    return m_nextMouseSample;
}

void Gesture::GetMouseSample( int i, int *x, int *y, int *t )
{
    *x = m_mouseSamples[i].x;
    *y = m_mouseSamples[i].y;

    if( t ) *t = m_mouseSamples[i].time / 1000;
}

void Gesture::GetMouseSample( int i, float *x, float *y, float *t )
{
    int ix, iy, it;
    GetMouseSample( i, &ix, &iy, &it );

    *x = ix;
    *y = iy;

    if( t ) *t = it;
}

int Gesture::GetNumSymbols()
{
    return m_symbols.Size();
}

void Gesture::NewSymbol()
{
    GestureSymbol symbol;
    m_symbols.PutData( symbol );
}

int Gesture::GetNumTrainers( int symbolID )
{
    return m_symbols[symbolID].m_classes.m_nextTrainer;
}

double Gesture::GetCalculatedResult( int symbolID )
{
    if( symbolID >= 0 && symbolID < GetNumSymbols() )
    {
        return m_symbols[symbolID].m_results;
    }
    else
    {
        return -1.0;
    }
}
    
double Gesture::GetCalculatedConfidence( int symbolID )
{
    if( symbolID >= 0 && symbolID < GetNumSymbols() )
    {
        return m_symbols[symbolID].m_confidence;
    }
    else
    {
        return -1.0;
    }
}

double Gesture::GetMahalanobisDistance( int symbolID )
{
    if( symbolID >= 0 && symbolID < GetNumSymbols() )
    {
        return m_symbols[symbolID].m_mahalanobis;
    }
    else
    {
        return -1.0;
    }
}
    
int Gesture::GetSymbolID()
{
    int symbolMax = -1;

    for( int i = 0; i < GetNumSymbols(); ++i )
    {
        if( symbolMax == -1 || 
            m_symbols[i].m_results > m_symbols[symbolMax].m_results )
        {
            symbolMax = i;
        }
    }

    return symbolMax;
}

bool Gesture::GestureSuccess()
{
    return ( GetConfidence() >= GESTURE_CONFIDENCETHRESHOLD &&
             GetMahalanobis() <= GESTURE_MAHALANOBISTHRESHOLD );
}

double Gesture::GetConfidence()
{
    int symbolID = GetSymbolID();
    return GetCalculatedConfidence(symbolID);
}

double Gesture::GetMahalanobis()
{
    int symbolID = GetSymbolID();
    return GetMahalanobisDistance(symbolID);
}
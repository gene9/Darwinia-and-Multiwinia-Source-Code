#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"

class Matrix {

public:

// Default Constructor. Creates a 1 by 1 matrix; sets value to zero. 
Matrix () {
  nRow_ = 1; nCol_ = 1;
  data_ = new double [1];  // Allocate memory
  set(0.0);                // Set value of data_[0] to 0.0
}

// Regular Constructor. Creates an nR by nC matrix; sets values to zero.
// If number of columns is not specified, it is set to 1.
Matrix(int nR, int nC = 1) {
  DarwiniaDebugAssert(nR > 0 && nC > 0);    // Check that nC and nR both > 0.
  nRow_ = nR; nCol_ = nC;
  data_ = new double [nR*nC];  // Allocate memory
  DarwiniaDebugAssert(data_ != 0);          // Check that memory was allocated
  set(0.0);                    // Set values of data_[] to 0.0
}

// Copy Constructor.
// Used when a copy of an object is produced 
// (e.g., passing to a function by value)
Matrix(const Matrix& mat) {
  this->copy(mat);   // Call private copy function.
}

// Destructor. Called when a Matrix object goes out of scope.
~Matrix() {
  delete [] data_;   // Release allocated memory
}

// Assignment operator function.
// Overloads the equal sign operator to work with
// Matrix objects.
Matrix& operator=(const Matrix& mat) {
  if( this == &mat ) return *this;  // If two sides equal, do nothing.
  delete [] data_;                  // Delete data on left hand side
  this->copy(mat);                  // Copy right hand side to l.h.s.
  return *this;
}

// Simple "get" functions. Return number of rows or columns.
int nRow() const { return nRow_; }
int nCol() const { return nCol_; }

// Parenthesis operator function.
// Allows access to values of Matrix via (i,j) pair.
// Example: a(1,1) = 2*b(2,3); 
// If column is unspecified, take as 1.
double& operator() (int i, int j = 1) {
  DarwiniaDebugAssert(i > 0 && i <= nRow_);          // Bounds checking for rows
  DarwiniaDebugAssert(j > 0 && j <= nCol_);          // Bounds checking for columns
  return data_[ nCol_*(i-1) + (j-1) ];  // Access appropriate value
}

// Parenthesis operator function (const version).
const double& operator() (int i, int j = 1) const{
  DarwiniaDebugAssert(i > 0 && i <= nRow_);          // Bounds checking for rows
  DarwiniaDebugAssert(j > 0 && j <= nCol_);          // Bounds checking for columns
  return data_[ nCol_*(i-1) + (j-1) ];  // Access appropriate value
}

// Set function. Sets all elements of a matrix to a given value.
void set(double value) {
  int i, iData = nRow_*nCol_;
  for( i=0; i<iData; i++ )
    data_[i] = value;
}

//*********************************************************************
private:

// Matrix data.
int nRow_, nCol_;  // Number of rows, columns
double* data_;     // Pointer used to allocate memory for data.

// Private copy function.
// Copies values from one Matrix object to another.
void copy(const Matrix& mat) {
  nRow_ = mat.nRow_;
  nCol_ = mat.nCol_;
  int i, iData = nRow_*nCol_;
  data_ = new double [iData];
  for(i = 0; i<iData; i++ )
    data_[i] = mat.data_[i];
}

}; // Class Matrix


// Compute inverse of matrix
double inv(Matrix A, Matrix& Ainv) 
// Input
//    A    -    Matrix A (N by N)
// Outputs
//   Ainv  -    Inverse of matrix A (N by N)
//  determ -    Determinant of matrix A	(return value)
{

  int N = A.nRow();
  DarwiniaDebugAssert( N == A.nCol() );
  
  Ainv = A;  // Copy matrix to ensure Ainv is same size
    
  int i, j, k;
  Matrix scale(N), b(N,N);	 // Scale factor and work array
  int *index;  index = new int [N+1];

  //* Matrix b is initialized to the identity matrix
  b.set(0.0);
  for( i=1; i<=N; i++ )
    b(i,i) = 1.0;

  //* Set scale factor, scale(i) = max( |a(i,j)| ), for each row
  for( i=1; i<=N; i++ ) {
    index[i] = i;			  // Initialize row index list
    double scalemax = 0.;
    for( j=1; j<=N; j++ ) 
      scalemax = (scalemax > fabs(A(i,j))) ? scalemax : fabs(A(i,j));
    scale(i) = scalemax;
  }

  //* Loop over rows k = 1, ..., (N-1)
  int signDet = 1;
  for( k=1; k<=N-1; k++ ) {
	//* Select pivot row from max( |a(j,k)/s(j)| )
    double ratiomax = 0.0;
	int jPivot = k;
    for( i=k; i<=N; i++ ) {
      double ratio = fabs(A(index[i],k))/scale(index[i]);
      if( ratio > ratiomax ) {
        jPivot=i;
        ratiomax = ratio;
      }
    }
	//* Perform pivoting using row index list
	int indexJ = index[k];
	if( jPivot != k ) {	          // Pivot
      indexJ = index[jPivot];
      index[jPivot] = index[k];   // Swap index jPivot and k
      index[k] = indexJ;
	  signDet *= -1;			  // Flip sign of determinant
	}
	//* Perform forward elimination
    for( i=k+1; i<=N; i++ ) {
      double coeff = A(index[i],k)/A(indexJ,k);
      for( j=k+1; j<=N; j++ )
        A(index[i],j) -= coeff*A(indexJ,j);
      A(index[i],k) = coeff;
      for( j=1; j<=N; j++ ) 
        b(index[i],j) -= A(index[i],k)*b(indexJ,j);
    }
  }
  //* Compute determinant as product of diagonal elements
  double determ = signDet;	   // Sign of determinant
  for( i=1; i<=N; i++ )
	determ *= A(index[i],i);

  //* Perform backsubstitution
  for( k=1; k<=N; k++ ) {
    Ainv(N,k) = b(index[N],k)/A(index[N],N);
    for( i=N-1; i>=1; i--) {
      double sum = b(index[i],k);
      for( j=i+1; j<=N; j++ )
        sum -= A(index[i],j)*Ainv(j,k);
      Ainv(i,k) = sum/A(index[i],i);
    }
  }

  delete [] index;	// Release allocated memory
  return( determ );        
}


void InvertMatrix( double *matrixIn, double *matrixOut, int rows, int cols )
{
    Matrix m( rows, cols );

    //
    // Fill in the matrix with our data

    {
        for( int i = 0; i < rows; ++i )
        {
            for( int j = 0; j < cols; ++j )
            {
                m(i+1,j+1) = matrixIn[i * cols + j];
            }
        }
    }

    //
    // Invert the sucker

    Matrix invert( rows, cols );
    inv( m, invert );

    //
    // Copy the result into our matrix Out
    {
        for( int i = 0; i < rows; ++i )
        {
            for( int j = 0; j < cols; ++j )
            {
                matrixOut[i * cols + j] = invert(i+1, j+1);
            }
        }
    }
    
}

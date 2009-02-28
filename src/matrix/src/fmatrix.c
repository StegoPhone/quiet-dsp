//
// Floating-point matrix
// 

#include "matrix_internal.h"

#define MATRIX(name)    LIQUID_CONCAT(fmatrix, name)
#define MATRIX_NAME     "fmatrix"
#define T               float
#define MATRIX_PRINT_ELEMENT(x,m,n) \
    printf("%4.2f\t", matrix_fast_access(x,m,n));

#include "matrix.c"


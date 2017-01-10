#include <windows.h>
#include <stdio.h>
#include "mex.h"

#define KERNEL_64BIT

#include "DMATransfer.h"


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
    /* Check number of input and output arguments. */
    if (nrhs != 1) {
        mexPrintf("Unsupported number of input arguments: %d\n", nrhs);
        return;
    }
    if (nlhs > 1) {
        mexPrintf("Unsupported number of output arguments: %d\n", nrhs);
        return;
    }

    /* Check that input argument is a matrix. */
    if (mxIsComplex(prhs[0]) || mxGetNumberOfDimensions(prhs[0]) != 2 || mxIsSparse(prhs[0]) || !mxIsDouble(prhs[0])) {
        mexPrintf("Input must be a scalar of type double.\n");
        return;
    }

    int M = mxGetM(prhs[0]);
    int N = mxGetN(prhs[0]);
    
    int ASizeInBytes = M * N * mxGetElementSize(prhs[0]);
    double* APtr = mxGetPr(prhs[0]);
    
    plhs[0] = mxCreateDoubleMatrix(M, N, mxREAL);
    double* BPtr = mxGetPr(plhs[0]);
    int BSizeInBytes = M * N * mxGetElementSize(plhs[0]);

    DMATransfer((UCHAR*)APtr, ASizeInBytes, (UCHAR*)BPtr, BSizeInBytes);
}

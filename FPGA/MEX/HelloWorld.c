#include "mex.h"

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
    mexPrintf("Hello, world! %d\n", (int)mxGetScalar(prhs[0]));
    return;
}

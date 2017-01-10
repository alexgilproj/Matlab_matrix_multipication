////////////////////////////////////////////////////////////////////////////
// Function name:   matrixMulMexCUBLAS_streams_2
// Description:     -Function will multiply matrix_a with matrix_b using the
//
// Input:           l_matrix(M x K x mat_num), r_matrix(K x N x mat_num),stream_num)
// Return Value:    outMatrix(M x N x mat_num)
////////////////////////////////////////////////////////////////////////////

// Utilities and system includes
#include <assert.h>
#include <windows.h>
#include "mex.h"
#include "blas.h"
#include <cublasXt.h>


#if !defined(_WIN32)
#define sgemm sgemm_
#endif


// CUDA runtime
#include <stdio.h> 
#include <stdlib.h> 
#include <math.h> 

#include <cuda.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>
//#include "cublas_v2.h"
#include <pthread.h>
#include <DMATransfer.h>

// #include "cblas.h"

// global variables
int thread_num 		= 0;
float fpga_ratio 	= 0.0;
float cpu_ratio 	= 1.0;

typedef struct _thread_data_t {
	int tid;
// 	enum CBLAS_ORDER Order;
    char* TransA;
    char* TransB; 
    ptrdiff_t M;
    ptrdiff_t N;
    ptrdiff_t K; 
    float alpha;
    float *A;
    ptrdiff_t lda;
    float *B;
    ptrdiff_t ldb;
    float beta;
    float *C;
    ptrdiff_t ldc;
} thread_data_t;

typedef struct _matrixSize {    // Optional Command-line multiplier for matrix sizes
    unsigned int uiWA, uiHA, uiWB, uiHB, uiWC, uiHC;
} sMatrixSize;


void* run_cpu_blas(void *threadarg) {
    thread_data_t *d = (thread_data_t*)threadarg;
    ptrdiff_t *m = &(d->M);
    ptrdiff_t *n = &(d->N);
    ptrdiff_t *k = &(d->K);
    ptrdiff_t *LDA = &(d->lda);
    ptrdiff_t *LDB = &(d->ldb);
    ptrdiff_t *LDC = &(d->ldc);
    sgemm( 
        d->TransA, 
        d->TransA, 
        m, 
        n, 
        k, 
        &(d->alpha), 
        d->A, 
        LDA, 
        d->B, 
        LDB, 
        &(d->beta), 
        d->C, 
        LDC);   
    //printf("finished cpu calc\n");
    pthread_exit(NULL);
    return NULL;
}


void* run_fpga_blas(void *threadarg) {
    // DWORD BUF_SIZE = 536870912;
	// UCHAR* baseWriteBuffer = new UCHAR[BUF_SIZE];
	// UCHAR* baseReadBuffer = new UCHAR[BUF_SIZE];
    // DMATransfer(baseWriteBuffer, BUF_SIZE, baseReadBuffer, BUF_SIZE);
    // delete[] baseWriteBuffer;
	// delete[] baseReadBuffer;
    thread_data_t *d = (thread_data_t*)threadarg;
    
    
    ptrdiff_t *m = &(d->M);
    ptrdiff_t *n = &(d->N);
    ptrdiff_t *k = &(d->K);
    ptrdiff_t *LDA = &(d->lda);
    ptrdiff_t *LDB = &(d->ldb);
    ptrdiff_t *LDC = &(d->ldc);
    sgemm( 
        d->TransA, 
        d->TransA, 
        m, 
        n, 
        k, 
        &(d->alpha), 
        d->A, 
        LDA, 
        d->B, 
        LDB, 
        &(d->beta), 
        d->C, 
        LDC);
    return NULL;
}

void cblas_sgemm_wrapper( 
    char *transa, 
    char *transb, 
    int  *m_p, 
    int  *n_p, 
    int  *k_p, 
    float *alpha_p, 
    float *h_A, 
    int *lda_p, 
    float *h_B, 
    int *ldb_p, 
    float *beta_p, 
    float *h_C, 
    int *ldc_p) {               


    ptrdiff_t m,n,k;
    m = *m_p;       
    n = *n_p;
    k = *k_p;
    ptrdiff_t lda,ldb,ldc;
    lda = *lda_p;
    ldb = *ldb_p;
    ldc = *ldc_p;
    
    float alpha = *alpha_p;
    float beta = *beta_p;

	// set up the division between cpu and fpga
	float cpu_inner_ratio 	= cpu_ratio / (fpga_ratio + cpu_ratio);
	float fpga_inner_ratio 	= 1.0 - cpu_inner_ratio;
	ptrdiff_t eff_fpga_n_all	= (ptrdiff_t) ((float)fpga_inner_ratio * (float)n);
    ptrdiff_t eff_fpga_n 		= (thread_num == 1)     ? 0 : (ptrdiff_t) (ptrdiff_t)eff_fpga_n_all / (ptrdiff_t)(thread_num - 1);
	ptrdiff_t eff_cpu_n 		= (eff_fpga_n_all == 0) ? n : (ptrdiff_t) (n - eff_fpga_n_all) + (ptrdiff_t) (eff_fpga_n_all % (ptrdiff_t)(thread_num - 1));	
    
	pthread_t* threads = (pthread_t*)mxMalloc(sizeof(pthread_t)*thread_num);
    if (threads==NULL){
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Out of system memory for threads.");
        return;
    }
    
    thread_data_t* thr_data = (thread_data_t*)mxMalloc(sizeof(thread_data_t)*thread_num);
    if (thr_data==NULL){
        mxFree(threads);
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Out of system memory for thread structure.");
        return;
    }
    
	for (ptrdiff_t t = 0; t < (ptrdiff_t)thread_num; t++) {//set data for pthread
        thr_data[t].tid     = t;	
        thr_data[t].TransA 	= transa;
        thr_data[t].TransB 	= transb;
        thr_data[t].M 		= m;
		// divide matricies and give the leftover to the last thread
        thr_data[t].N		= (t == (size_t)thread_num - 1) ? eff_cpu_n :
															  eff_fpga_n;
        thr_data[t].K 		= k;
        thr_data[t].alpha	= alpha;
        thr_data[t].A		= h_A ;
        thr_data[t].lda		= lda;
        thr_data[t].B		= (h_B + (int)((ptrdiff_t)t * (ptrdiff_t)ldb * (ptrdiff_t)eff_fpga_n));
        thr_data[t].ldb		= ldb;
        thr_data[t].beta	= beta;
        thr_data[t].C		= (h_C + (int)((ptrdiff_t)t * (ptrdiff_t)ldc * (ptrdiff_t)eff_fpga_n));
        thr_data[t].ldc		= ldc;
    }
    
	int rc; //error code for thread createion 
    for (int t = 0; t < thread_num; t++) {
        rc = (t == thread_num - 1) ? pthread_create(&threads[t], NULL, run_cpu_blas, &thr_data[t]) : 
                                     pthread_create(&threads[t], NULL, run_fpga_blas, &thr_data[t]);
        if (rc) {
            mxFree(threads);
            mxFree(thr_data);            
            mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Bad return code for pthread_create");
            return;
        }
    }
     
	for (int t = 0; t < thread_num; t++) {
		pthread_join(threads[t], NULL);
	}

    mxFree(threads);
    mxFree(thr_data);
    return;        
}

// function to create an int aray of on system gpus (NVIDIA only)
int* InitiateGpuId(int *gpu_num){
	//initilize variables
    int gpu_max_num     = 0;
    cudaError cuda_error_check;
    
	// get devices number
    cuda_error_check    = cudaGetDeviceCount(&gpu_max_num);
    if (cuda_error_check != cudaSuccess || gpu_max_num == 0){
        mexWarnMsgTxt ("No active GPU on system.");
        return NULL;
    }
    
	// assign memory for the gpu numbers array
    int *gpu_id = (int*)mxMalloc(sizeof(int)*gpu_max_num);
    if (gpu_id==NULL){
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Out of system memory for gpu array.");
        return NULL;
    }
	
	// assign gpu ids to the array
	for(int i = 0; i < gpu_max_num; i++){
        gpu_id[i] = i;
    }
    gpu_max_num = *gpu_num < gpu_max_num ? *gpu_num : gpu_max_num;
	*gpu_num    = (gpu_max_num == 0) ? 1 : gpu_max_num;
    for(int i = 0; i < gpu_max_num; i++){
        gpu_id[i] = i;
    }
	
    return gpu_id;
}

int initializeCUDA(int &devID, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], sMatrixSize &matrix_size)
{
    matrix_size.uiWA = *(mxGetDimensions(prhs[0]) + 1);
    matrix_size.uiHA = *(mxGetDimensions(prhs[0]) + 0);
    matrix_size.uiWB = *(mxGetDimensions(prhs[1]) + 1);
    matrix_size.uiHB = *(mxGetDimensions(prhs[1]) + 0);
    matrix_size.uiWC = *(mxGetDimensions(prhs[1]) + 1);
    matrix_size.uiHC = *(mxGetDimensions(prhs[0]) + 0);
    return 0;
}

int matrixMultiply_wrapper(float* h_A, float* h_B, float* h_C, int devID, sMatrixSize &matrix_size, int gpu_num, int block_dim)
{
	// initialize variables
    cublasStatus_t  cublas_error_check; // error checker
    float alpha = 1.0f;					// constant for BLAS routine
    float beta  = 0.0f;					// constant for BLAS routine
    cublasXtHandle_t handle;			// handle for cuBLAS_XT routine
	int *gpu_id = NULL;					// gpu array DIM	
	
    if (CUBLAS_STATUS_SUCCESS != (cublasXtCreate(&handle))){
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Could not create CUBLAS handle.");
        return 1;
    }    
    
	// create an INT array of the gpus in system (NVIDIA only)
    gpu_id = InitiateGpuId(&gpu_num);
    if (gpu_id == NULL && gpu_num != 0){
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Could not set up available gpus array.");
		return 1;
	}

	// create xt handle, this is data for the XT routine
    cublas_error_check = cublasXtDeviceSelect(handle, gpu_num, gpu_id);
    if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
        mxFree(gpu_id);
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Could not set gpu device for cuBLAS_XT.");
        return 0;
    }     

	// set the cpu routine
    cublas_error_check = cublasXtSetCpuRoutine(handle, CUBLASXT_GEMM, CUBLASXT_FLOAT, cblas_sgemm_wrapper);
    if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
		mxFree(gpu_id);
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Cannot set cpu and fpga routine.");        
        return 0;
    }
    
	// set CPU and fpga routine ratio
    cublas_error_check = cublasXtSetCpuRatio(handle, CUBLASXT_GEMM, CUBLASXT_FLOAT, cpu_ratio + fpga_ratio);   
    if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
		mxFree(gpu_id);	
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Cannot set cpu and fpga offload.");        
        return 0;
    } 
    
	// set clock dim if recieved
	if (block_dim != 0){
		cublas_error_check = cublasXtSetBlockDim(handle , block_dim);
		if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
			mxFree(gpu_id);	
			mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Cannot set cpu and fpga offload.");        
        return 0;
    } 
	}
	
	// pin the memory
    cublas_error_check = cublasXtSetPinningMemMode(handle,CUBLASXT_PINNING_ENABLED);
        if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
        mxFree(gpu_id);
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Cannot pin memory.");        
        return 0;
    } 
	
	// call cuBLAS_XT routine to perform the multipictaion (matricies shoud be in COLUMN MAJOR FORMAT)
    cublas_error_check = (cublasXtSgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N,
                                      matrix_size.uiHA, matrix_size.uiWB, matrix_size.uiWA, &alpha,
                                      (h_A), matrix_size.uiHA,
                                      (h_B), matrix_size.uiHB,
                                      &beta,
                                      (h_C), matrix_size.uiHA));
    if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
        mxFree(gpu_id);
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Could not run device CUBLAS_XT kernal.");        
        return 0;
    }


	// Destroy the handle.
    cublas_error_check = (cublasXtDestroy(handle));
    if (cublas_error_check != CUBLAS_STATUS_SUCCESS){
		mxFree(gpu_id);
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Could not destroy CUBLAS handle.");        
        return 0;
    }    
    mxFree(gpu_id);
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])	
{
    
    int devID = 0;
    sMatrixSize matrix_size;

	// check for proper number of arguments
    if(nrhs > 7 || nrhs < 2) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Bad input.");
    }
    if(nlhs!=1) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","One output required.");
    }
	
	// Mex reading arguments
    float* l_matrix;               	// left matrix
	float* r_matrix;               	// right matrix
	float* outMatrix;              	// output matrix
    cpu_ratio  			= 1.0;      // cpu usage ratio, global variable
	fpga_ratio 			= 0.0;		// fpga useage raio, global variable
	int block_dim 		= 0;		// block dimensions for cuBLAS_XT algorithem
    int gpu_num 		= 0;        // number of GPU to use with XT    
	thread_num			= 1; 		// thread number, global variable
	
	// Assign inputs to corresponding variables
	switch ( nrhs ) {
		case 7:
			block_dim 		= *(int*)mxGetData(prhs[6]);
		case 6:			
			fpga_ratio		= *(float*)mxGetData(prhs[5]);
		case 5:
			cpu_ratio       = *(float*)mxGetData(prhs[4]);
			if (fpga_ratio + cpu_ratio > 1.0){
				mexWarnMsgTxt("matrixMulMex_1_0 : Fpga and cpu ratio inputs exceed 1.0, using 1.0 cpu instead.");
				fpga_ratio 	= 0.0; //glogal variable 
				cpu_ratio	= 1.0;
			}
		case 4:
			gpu_num         = *(int*)mxGetData(prhs[3]);			
		case 3:
			thread_num      = *(int*)mxGetData(prhs[2]) == 0 ? 1 : 
                                                               *(int*)mxGetData(prhs[2]); //glogal variable 
            if (fpga_ratio != 0 && cpu_ratio != 0 && thread_num < 2){
				mexWarnMsgTxt("matrixMulMex_1_0 : not enough threads inputted, using one thread instead, cpu only.");
				fpga_ratio 	= 0.0; //glogal variable 
				cpu_ratio	= 1.0;
                thread_num  = 1;
			}
		default:
			r_matrix        = (float*)mxGetData(prhs[1]);
			l_matrix        = (float*)mxGetData(prhs[0]);			
			break;
	}

    // check that number of rows in matricies are legal 
    if(*(mxGetDimensions(prhs[0]) + 1) != *(mxGetDimensions(prhs[1]) + 0)) {
        mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Input must be legal.");
    }
    
	// build a struct with the dimensions of all the matricies
    if (1 == initializeCUDA(devID, nlhs, plhs, nrhs, prhs, matrix_size)){
		mexErrMsgIdAndTxt("MyToolbox:arrayProduct:notRowVector","Unknown issue with matricies.");
		return;
	}
    
	// create memory fo the left hand side result
	mwSize dims[2];	
	dims[0] = matrix_size.uiHA;
	dims[1] = matrix_size.uiWB;
    plhs[0] = mxCreateNumericArray(2, dims, mxSINGLE_CLASS, mxREAL);
    
    // get a pointer to the real data in the output matrix.
    outMatrix = (float*)mxGetData(plhs[0]);

	// call the cuBLAS_XT wrapper function.
    int matrix_result = matrixMultiply_wrapper(	l_matrix, 
												r_matrix, 
												outMatrix, 
												devID,
												matrix_size, 
												gpu_num, 
												block_dim
												);	
	// initzilize the global variables
	cpu_ratio  = 1.0; //global variable
	fpga_ratio = 0.0; //global variable 
    thread_num = 0; //global variable 
    return;
}

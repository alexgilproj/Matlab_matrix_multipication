function [ C ] = MatrixMulEnhanced( A , B , thread_num , gpu_num , cpu_usage , fpga_usage , xt_blk_dim )
% Function is an enhancment to the regular matlab matrix multipictaion read
% about parameters for more info:
% inputs:
% A             - left side matrix, must be single type NOT double.
% B             - right side matrix, must be single type NOT double.
% thread_num    - Number of threads to use, 1 is defualt if cpu percentage
%                 is entered. if you wish to use the fpgas on system, enter
%                 (SYSTEM_FPGA_NUMBER + 1) as this parameter.
% gpu_num       - number of on system GPUs to use.
% cpu_usage     - precentage load of cpu to use while performing the operation.
% fpga_usage    - precentage load of fpgas to use while performing the operation.
% xt_blk_dim    - block dimensions for blas_xt routine, for more infor check CUDA documentation.

if ( ~isa(A,'single') || ~isa(B,'single') )
    error('MatrixMulEnhanced:BadInput','Input matricies A and B must be of single type');
end
   
if     nargin == 7
    C = matrixMulMex_1_0(A ,B,int32(thread_num),int32(gpu_num),single(cpu_usage),single(fpga_usage),int32(xt_blk_dim));
elseif nargin == 6
    C = matrixMulMex_1_0(A ,B,int32(thread_num),int32(gpu_num),single(cpu_usage),single(fpga_usage));
elseif nargin == 5
    C = matrixMulMex_1_0(A ,B,int32(thread_num),int32(gpu_num),single(cpu_usage));
elseif nargin == 4
    C = matrixMulMex_1_0(A ,B,int32(thread_num),int32(gpu_num));
elseif nargin == 3
    C = matrixMulMex_1_0(A ,B,int32(thread_num));
elseif nargin == 2
    C = matrixMulMex_1_0(A ,B);
else
    error('MatrixMulEnhanced:BadInput','Bad input number');
end


end


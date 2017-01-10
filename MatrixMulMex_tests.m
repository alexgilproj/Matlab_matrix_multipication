%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Usage Instructions:
% *Use the environment set up only when the userpath is where this scripts
% is at.
%
% Compiling:
% 1. Install CUDA (v7.0 and higher, change env path below if higher
% version or different path).
% 2. Install Visual Studio 2010 OR 2013
% 3. copy the xml from the VS folder to this dir
% 4. run the environment set up
% 5. run the compilation line found at the end of the environment set up
% section
% 
% Using the function:
% 1. Run the environment Set up
% 2. use MatrixMulEnhanced like normal MATLAB function see referent in 
% function usage reference section (see help of wrapper function for more 
% details)
% 
% Run Tests to Find Optimal CPU usage:
% 1. see Run section
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Environment Set UP
% run this to set up the environment that lets the function run properly, of
% compile if you wish.
close all;
clear all;
clc;

current_folder = [pwd '\'];

blaslib     = fullfile(matlabroot,'extern','lib',computer('arch'),'microsoft',...
  'libmwblas.lib');
fpgalib     = [current_folder, 'FPGA\VC709DMA.lib'];
driverlib   = [current_folder, 'FPGA\DriverMgr.lib'];

pthreadsInstallFolder = [current_folder 'Pthreads_Win32\'];  % change this as needed

% change the cuda install path if needed(if version 7.0 isn't used).
setenv('CUDA_LIB_PATH', 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v7.0\lib\x64');
setenv('CUDA_BIN_PATH', 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v7.0\bin');
setenv('PATH',   [getenv('PATH')    ';' pthreadsInstallFolder 'dll\x64']);
setenv('LIB',    [getenv('LIB')     ';' pthreadsInstallFolder 'lib\x64']);
setenv('INCLUDE',[getenv('INCLUDE') ';' pthreadsInstallFolder 'include']);
setenv('INCLUDE',[getenv('INCLUDE') ';' current_folder 'FPGA']);

% use these if you wish to compile with VS2013
% setenv('LINKFLAGS', '/NODEFAULTLIB:LIBCMTD');
% setenv('VCROOT','C:\Program Files (x86)\Microsoft Visual Studio 12.0\');\
% setenv('PATH',   [getenv('PATH')    ';' 'C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\amd64\']);

%2015-06-10 compilation line: 
% mex('-lpthreadVC2','-largeArrayDims','matrixMulMex_1_0.cu',blaslib, fpgalib, driverlib);

%% Function Usage Reference

THREAD_NUM = 1;
TEST_FACTOR = 10;

 A = single(rand(2^TEST_FACTOR));
 B = single(rand(2^TEST_FACTOR));
c_cpu = A*B;
% desired calculation, press ctrl+F1 to see inputs or see function info while the cursor is in the brackets. 
c_gpu = MatrixMulEnhanced(A,B,1,1,0.2);
% error checking
err_gpu = round(max(max(100*(abs(c_cpu - c_gpu))./c_cpu)))

%%
% calculate the average runtime of matrix multiplication on a gpu and a cpu
% on this computer with varying NxN matrix multiplications.

% testing parameters.
SAMPELS                 = 10;   %number of samples per average.
M                       = 2^13;
ITERATION_NUM           = 50;
GPU_NUM                 = 1;
N                       = 2^13;
K                       = 2^13;
% initializing the random matrixes.
matrix_A                = single(rand(M,K));
matrix_B                = single(rand(K,N));
temp                    = 0*matrix_A;
percentage              = linspace(0,1,ITERATION_NUM);
power                   = linspace(5,13,ITERATION_NUM);

gpu_xt_calc_result      = zeros(1,SAMPELS);
avg_gpu_xt_calc_result  = zeros(1,ITERATION_NUM);
cpu_calc_result         = zeros(1,SAMPELS);
avg_cpu_calc_result     = zeros(1,ITERATION_NUM);
cpu_xt_calc_result      = zeros(1,SAMPELS);
avg_cpu_xt_calc_result  = zeros(1,ITERATION_NUM);
%% Complete test
% Section will calculate the run times of gpus Vs. cpu usage(in
% percentage). define parameter as needed per system.
for k = 1:ITERATION_NUM
    temp                    = 0*matrix_A;
    for i=1:SAMPELS
        matrix_A                = single(rand(M,K));
        matrix_B                = single(rand(K,N));
        tic
        temp                = MatrixMulEnhanced(matrix_A,matrix_B,1, GPU_NUM, percentage(k));
        gpu_xt_calc_result(i)  = toc;
    end
    
    avg_gpu_xt_calc_result(k)          = mean(gpu_xt_calc_result);
end
[fastest_result l ]= min(avg_gpu_xt_calc_result)
fasters_percentage = avg_gpu_xt_calc_result(l)
% plot the dependencies of the calculation times of the gpu and the cpu 
% based on matrix sizes.
figure(2);

m_size_vec = 1:ITERATION_NUM;
plot    (percentage, avg_gpu_xt_calc_result,'k')
legend  ('GPU Xt Calculation Times');
ylim    ([0 1.1*max(avg_gpu_xt_calc_result)]);
xlabel  ('CPU use percentage');
ylabel  ('Calculation Time [Sec]');
title   (['GPU And CPU Matrix Multiplication (combined) (',num2str(M),' over ',num2str(M),') X (' ,num2str(M),' over ',num2str(M),') VS. cpu usage']);
% hold on;
%% test of GPU vs. CPU only
% Section will calculate the run times of gpus Vs. cpu usage(in
% percentage). define parameter as needed per system.
for k = 1:ITERATION_NUM
    temp                    = 0*matrix_A;
    for i=1:SAMPELS
        matrix_A                = single(rand(floor(M^(power(k)))));
        matrix_B                = single(rand(floor(M^(power(k)))));
        tic
        temp                = MatrixMulEnhanced(matrix_A,matrix_B,0, GPU_NUM, 0);
        gpu_xt_calc_result(i)  = toc;
        tic
        temp                = matrix_A*matrix_B;
        cpu_calc_result(i)  = toc;
        tic
        temp                = MatrixMulEnhanced(matrix_A,matrix_B,1, GPU_NUM, 1);
        cpu_xt_calc_result(i)  = toc;        
    end
    
    avg_gpu_xt_calc_result(k)   = mean(gpu_xt_calc_result);
    avg_cpu_calc_result(k)      = mean(cpu_calc_result);
    avg_cpu_xt_calc_result(k)   = mean(cpu_xt_calc_result);
end

figure(1);

plot(power,avg_gpu_xt_calc_result,power,avg_cpu_calc_result,power,avg_cpu_xt_calc_result);
legend  ('GPU CuBLAS-XT only','CPU MATLAB Default','CPU CuBLAS-XT Only');
ylim    ([0 0.3*max(avg_gpu_xt_calc_result)]);
xlim    ([5 0.9*max(power)])
xlabel  ('M number [2^{index}]');
ylabel  ('Calculation Time [Sec]');
title   ('Matrices Multiplication calculation Times Vs Matrices Size (MxM * MxM)');
%% Binary minimum search on the slope
% fastest way to find the best cpu usage percentage. algorithm assumes a
% single global minimum and tries to find it with as little calculations as
% possible. matrices must be large (4e3 X 4e3 at least).
tic
L = 1;
R = length(percentage);
Mid = floor(( L + R ) / 2)
while L < R
    if (Mid == 0 || Mid == length(percentage))
        break
    end
    Mid = floor(( L + R ) / 2)
    tic
    temp = MatrixMulEnhanced(A,B,1, GPU_NUM, percentage(Mid - 1));
    gpu_M_Minus_1 = toc;
    tic
    temp = MatrixMulEnhanced(A,B,1, GPU_NUM, percentage(Mid));
    gpu_M = toc;
    if ( gpu_M_Minus_1 < gpu_M)
        R = Mid;
    else
        tic
        temp = MatrixMulEnhanced(A,B,1, GPU_NUM, percentage(Mid + 1));
        gpu_M_Plus_1 = toc;
         if ( gpu_M_Plus_1 < gpu_M)
            L = Mid;
         else
            break
         end
    end    
end
percentage(Mid)

toc
%%
%

#include <stdlib.h>
#include <cufft.h>
#include <cufftXt.h>
#include <stdio.h>
     #define N_SIGS 32
  #define SIG_LEN 1024


// libcudart
// cudaMalloc cudaMemcpy

// libcufft
// cufftPlanMany  cufftExecC2C

//gcc verify_cuda.c -lcufft -lcudart -lm -ocuda

   int main(){
   int err;
   cuFloatComplex *h_signal, *d_signal, *h_result, *d_result;
   
   h_signal = (cuFloatComplex *)malloc(N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
   h_result = (cuFloatComplex *)malloc(N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
   for (int i = 0; i < N_SIGS; i ++)
   for (int j = 0; j < SIG_LEN; j++)
   h_signal[(i*SIG_LEN) + j] = make_cuFloatComplex(sin((i+1)*6.283*j/SIG_LEN), 0);
   cudaMalloc((void**)&d_signal, N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
   cudaMalloc((void**)&d_result, N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
   
   cudaMemcpy(d_signal, h_signal, N_SIGS*SIG_LEN*sizeof(cuFloatComplex), cudaMemcpyHostToDevice);
      cufftHandle plan;
   int n[1] = {SIG_LEN};
   
      cufftResult res = cufftPlanMany(&plan, 1, n,
      NULL, 1, SIG_LEN,  //advanced data layout, NULL shuts it off
   NULL, 1, SIG_LEN,  //advanced data layout, NULL shuts it off
      CUFFT_C2C, N_SIGS);
      if (res != CUFFT_SUCCESS) {printf("plan create fail\n"); return 1;}
      
      res = cufftExecC2C(plan, d_signal, d_result, CUFFT_FORWARD);
      if (res != CUFFT_SUCCESS) {printf("forward transform fail\n"); return 1;}
      cudaMemcpy(h_result, d_result, N_SIGS*SIG_LEN*sizeof(cuFloatComplex), cudaMemcpyDeviceToHost);
      
      for (int i = 0; i < N_SIGS; i++){
        for (int j = 0; j < 10; j++)
      printf("%.3f ", cuCrealf(h_result[(i*SIG_LEN)+j]));
     printf("\n"); }
     
        return 0;
      }

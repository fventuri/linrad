#if HAVE_CUFFT == 1

#include "osnum.h"
#include <stdlib.h>
#include <cufft.h>
#include <cufftXt.h>
#include <stdio.h>
#include "globdef.h"

#define N_SIGS 16
#define SIG_LEN 256


// libcudart
// cudaMalloc cudaMemcpy

// libcufft
// cufftPlanMany  cufftExecC2C

//int cudaMalloc(void **p, size_t s);
//int cudaMemcpy(cuFloatComplex  *p, cuFloatComplex *q, size_t s, int par);    

float *z;
float2 *zz;
cuFloatComplex *h_signal, *h_result;
cuFloatComplex *d_signal, *d_result;
float *zout;
float2 *zzout;
cufftHandle plan;


void initmem(void)
{
cufftResult res;
int n[1];
n[0]=SIG_LEN;
z=malloc(2*N_SIGS*SIG_LEN*sizeof(float));
zz=(cuFloatComplex*)z;
h_signal=zz;
zout=malloc(2*N_SIGS*SIG_LEN*sizeof(float));
zzout=(cuFloatComplex*)zout;
h_result=zzout;
cudaMalloc((void**)&d_signal, N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
cudaMalloc((void**)&d_result, N_SIGS*SIG_LEN*sizeof(cuFloatComplex));
res = cufftPlanMany(&plan, 1, n,
                     NULL, 1, SIG_LEN,  //advanced data layout, NULL shuts it off
                     NULL, 1, SIG_LEN,  //advanced data layout, NULL shuts it off
                     CUFFT_C2C, N_SIGS);
if (res != CUFFT_SUCCESS)printf("plan create fail\n");
}


int test_cuda(void)
{
float t1,t2;
int i, j, k;
cufftResult res;
initmem();
for(k=1; k<4; k++)
  { 
  for (i = 0; i < N_SIGS; i ++)
    {
    t1=0;
    t2=2*PI_L*i/SIG_LEN;
    for (j = 0; j < SIG_LEN; j++)
      {
      z[2*(j+i*SIG_LEN)  ]=k*cos(t1)/SIG_LEN;
      z[2*(j+i*SIG_LEN)+1]=k*sin(t1)/SIG_LEN;
      t1+=t2;
      }
    }  
  cudaMemcpy(d_signal, h_signal, N_SIGS*SIG_LEN*sizeof(cuFloatComplex), cudaMemcpyHostToDevice);
  res = cufftExecC2C(plan, d_signal, d_result, CUFFT_FORWARD);
  if (res != CUFFT_SUCCESS) {printf("forward transform fail\n"); return 1;}
  cudaMemcpy(h_result, d_result, N_SIGS*SIG_LEN*sizeof(cuFloatComplex), cudaMemcpyDeviceToHost);
  for (i = 0; i < N_SIGS; i++)
    {
    for (j = 0; j < 10; j++)
      {
      printf("%6.3f ", cuCrealf(h_result[(i*SIG_LEN)+j]));
      }
    printf("\n"); 
    }
  printf("\n"); 
  }     
return 0;
}
#endif

//
// oclprogs.h --- signal processing algorithms in OpenCL
//
// Copyright (c) 2015 Juergen Kahrs
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef _OCLPROGS_H
#define _OCLPROGS_H 1
typedef void * oclHandleType;

#if OPENCL_PRESENT == 1
#if OSNUM == OSNUM_LINUX
// Define an opaque data type that hides clFFT plans behind syntactic sugar. 

#include <CL/cl.h>

#define NUMBER_OF_KERNELS 4
#define MAX_NUMBER_OF_DEVICES 10
#define MAX_NUMBER_OF_PLATFORMS 10
#define MAX_NUMBER_OF_DIMENSIONS 10
#define NAME_MAX_LENGTH 40

extern oclHandleType ocl_handle_fft1b[MAX_FFT1_THREADS];

// Variables describing the GPU platform found at run-time.
typedef struct {
  /* The number of platforms found at run-time. */
  cl_uint platforms;
  cl_platform_id platform[MAX_NUMBER_OF_PLATFORMS];
  /* The number of devices found on each platform. */
  cl_uint devices [MAX_NUMBER_OF_PLATFORMS];
  cl_device_id device[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES];
  float gflops[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES];
  /* All kernels, compiled for each platform on each device. */
  cl_kernel kernel_binary[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES][NUMBER_OF_KERNELS];
  cl_program prog[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES][NUMBER_OF_KERNELS];
  /* One context and queue for each platform on each device. */
  cl_context context[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES];
  cl_command_queue cq[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES];
  /* Workgroup sizes for each platform on each device. */
  size_t maxDimension[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES];
  size_t maxWorkGroupSize[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES][MAX_NUMBER_OF_DIMENSIONS];
  size_t maxWorkItemSizes[MAX_NUMBER_OF_PLATFORMS][MAX_NUMBER_OF_DEVICES][MAX_NUMBER_OF_DIMENSIONS];
} OCL_GLOBALS;
extern OCL_GLOBALS ocl_globals;


// We must not include clFFT.h in more than one file...
void do_clfftTeardown(void);

// This file specifies the interface to the signal processing
// algorithms that run on a graphics card (GPU). The interface is
// implemented as a library that is built upon the clFFT and the
// OpenCL libraries. This specification and its library try to
// shield the user of the library from all GPU terminology by
// encapsulating the GPU-internals inside the library.

void ocl_compute_gflops(unsigned int p_index, unsigned int d_index);

// You must invoke start_ocl before using the library.
int start_ocl(int verbose);
// After invoking stop_ocl the library cannot be used any more.
void stop_ocl(void);
// info_ocl runs several tests and prints the results to stdout.
void info_ocl(void);

 
// You must invoke create_clFFT_plan before doing signal processing.
// The following limitations are preliminary and some will be resolved later.
// All plans use dimension 1.
// All numbers are floats.
// All data arrays are complex interleaved.
oclHandleType create_clFFT_plan(
  size_t array_length,    // number of complex samples, not number of bytes
  size_t batch_size,      // number of arrays to process simultaneously
  float scale,            // factor to multiply on each resulting value
  unsigned short platform,// 0, enumerates OpenCL drivers by manufacturer
  unsigned short device   // 0, enumerates cards of the same manufacturer
);

// Perform FFT on input data. The result is returned in output array.
// Pass identical input and output pointers if you want results in-place.
// Pass unequal input and output pointers if you want results out-of-place.
int execute_clFFT_plan(
  oclHandleType handle,
  int direction,     // -1 = forward, +1 = backward
  void * input,
  void * output
);

int destroy_clFFT_plan(oclHandleType * handle_p);

// A functional tests for basic OpenCL and clFFT features.
int ocl_main(void);

extern int ocl_active;
extern cl_context_properties fft1_ocl_properties[3];
extern cl_command_queue fft1_ocl_cq;
extern cl_context fft1_ocl_context;
#endif /* _OCLPROGS_H */
#endif
#endif

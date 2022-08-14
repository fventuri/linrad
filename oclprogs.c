//
// oclprogs.c --- signal processing algorithms in OpenCL
//
// Copyright (c) 2013 Juergen Kahrs
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
#if OPENCL_PRESENT == 1

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <sys/time.h>
#include "osnum.h"
#include "globdef.h"  
#include "uidef.h"

#include <clFFT.h>
#include "thrdef.h"
#include "oclprogs.h"
// TODO: Windowing shall be done at the first butterfly stage.
// TODO: Coyping the ADC integer samples into complex samples.
// TODO: test case for processing a batch of simultaneous FFTs

int ocl_active;
cl_context_properties fft1_ocl_properties[3];
cl_command_queue fft1_ocl_cq;
oclHandleType ocl_handle_fft1b[MAX_FFT1_THREADS];
OCL_GLOBALS ocl_globals;

// A trivial string reversal algorithm demonstrates how to work with the
// OpenCL run-time environment.
char KernelSource_string_reversal[] = "\n" \
  "__kernel void string_reversal(                                         \n" \
  "    __global char* input,                                              \n" \
  "    __global char* output                                              \n" \
  ")                                                                      \n" \
  "{                                                                      \n" \
  "  int id    = get_global_id(0);                                        \n" \
  "  int len   = get_global_size(0);                                      \n" \
  "  output[id] = input[len-1-id];                                        \n" \
  "};                                                                     \n" \
  "\n";

// Inspired by http://ochafik.com/blog/?p=501
// The idea is to let each GPU core calculate one line (frequency) of the spectrum.
// This transform works with all positive numbers of input samples, not just powers of 2.
// Don't expect a good performance since this is NOT a "fast fourier transform".
// But this short example leads the way to efficient implementations.
char KernelSource_naive_fourier_transform[] = "\n" \
  "#pragma OPENCL EXTENSION cl_khr_fp64: enable                                            \n" \
  "__kernel void naive_fourier_transform(                                                  \n" \
  "  __global const float2 * in,        // complex values input                            \n" \
  "  __global       float2 * out,       // complex values output                           \n" \
  "  const unsigned length,             // number of input and output values               \n" \
  "  const int      sign,               // sign modifier in the exponential :              \n" \
  "                                     // 1 for forward transform, -1 for backward.       \n" \
  "  const float    scale               // output scale factor                             \n" \
  ") {                                                                                     \n" \
  "  // i is the index of the spectral to be calculated in parallel.                       \n" \
  "  int i = get_global_id(0);                                                             \n" \
  "  // Initialize sum and inner arguments                                                 \n" \
  "  float2 tot = 0;                                                                       \n" \
  "  float param = ((float) (-2 * sign * i)) * M_PI / (float)length;                       \n" \
  "  for (int k = 0; k < length; k++) {                                                    \n" \
  "    float2 value = in[k];                                                               \n" \
  "    // Compute sin and cos in a single call :                                           \n" \
  "    float c;                                                                            \n" \
  "    float s = sincos(k * param, &c);                                                    \n" \
  "    // This adds (value.x * c - value.y * s, value.x * s + value.y * c) to the sum :    \n" \
  "    tot += (float2)(                                                                    \n" \
  "      dot(value, (float2)(c, -s)),                                                      \n" \
  "      dot(value, (float2)(s,  c))                                                       \n" \
  "    );                                                                                  \n" \
  "  }                                                                                     \n" \
  "  out[i] = tot * scale;                                                                 \n" \
  "}                                                                                       \n" \
  "\n";


// Inspired by https://raw.github.com/miracle2121/hpc12-fp-rl1609/master/kernels/fft_radix2.cl
// The idea of Stockham's FFT is to perform every one of the logN stages
// of the FFT out-of-place for two reasons:
//   - avoid data-reordering
//   - allow threads to work concurrently on distinct data values
// The workgroup size must be N/2 threads for a complete spectrum.
// Stockham's original algorithm has 3 nested loops.
// The outermost loop (t) is implemented outside OpenCL by the wrapper function.
// As t goes from 1 to logN the sub-array size is
// 2^t for output and 2^(t-1) for input blocks. 
// Both inner loops (j and k) are implemented in the OpenCL kernel below.
// j loops over the number of output arrays N/2^t.
// k loops over the size   of input  arrays 2^(t-1).
// A canonical implementation of two nested loops with OpenCL would use
// one workgroup dimension (in clEnqueueNDRangeKernel) for each loop.
// Here we use the same workgroup dimension (index 0) for both loops and the kernel
// uses a bit mask to split up the global id (in dimension 0) into two fields that
// represent the two loop variables j and k of the original algorithm.
// This arrangement secures that all threads of a workgroup work on the same input array.
// This is essential for exploiting locality of global memory accesses.
// While the first workgroup works on the first array, the second workgroup
// works on the second array, located directly after the first array.
// A second workgroup dimension (index 1) is used to address thousands of input arrays
// that shall be processed at the same time.
char KernelSource_stockham_fft_radix_2[] = "\n" \
  "#pragma OPENCL EXTENSION cl_khr_fp64: enable                                            \n" \
  "__kernel void stockham_fft_radix_2(                                                     \n" \
  "  __global float2 * src,      // one array of input samples after the other             \n" \
  "  __global float2 * dst,                                                                \n" \
  "  const unsigned block_size,  // 2^(t-1)                                                \n" \
  "  const int      sign,        // sign and scale are needed for inverse transform        \n" \
  "  const float scale                                                                     \n" \
  ") {                                                                                     \n" \
  "  src             += 2*get_global_size(0) * get_global_id(1); // choose the right array \n" \
  "  dst             += 2*get_global_size(0) * get_global_id(1); // for this workgroup     \n" \
  "  // prefetch(src, 2*get_global_size(0)); // prefetch may slow down a CPU drastically   \n" \
  "  unsigned gid     = get_global_id(0);     // gid contains loop variables in a bitmask  \n" \
  "  // unsigned j    = gid / block_size;     // j is the index of the block, obsolete     \n" \
  "  unsigned    k    = gid & (block_size-1); // k is the index inside the block           \n" \
  "  src             += gid;                                                               \n" \
  "  dst             += 2 * gid - k;                                                       \n" \
  "  float2 in1   = src[0];                                                                \n" \
  "  float2 in2   = src[get_global_size(0)]; // the other input sample is N/2 away         \n" \
  "  float      theta = -M_PI * (float) (sign * (int) k) / (float) block_size;             \n" \
  "  float cs;                            // dont use native_cos() and native_sin()        \n" \
  "  float sn = sincos(theta, &cs);       // on some platforms they are unreliable         \n" \
  "  //float      sn    = native_sin(theta);   // native trigonometric functions are       \n" \
  "  //float      cs    = native_cos(theta);   // used inside their region of convergence  \n" \
  "  float2 temp  = (float2) (dot(in2, (float2)(cs,-sn)), dot(in2, (float2)(sn,cs)));      \n" \
  "  dst[         0]  = scale * (in1 + temp);    // scale down energy in inverse transform \n" \
  "  dst[block_size]  = scale * (in1 - temp);                                              \n" \
  "}                                                                                       \n" \
  "\n";

// Inspired by http://www.bealto.com/gpu-benchmarks_flops.html
// The number of floating point operations per second depends
// on some parameters: vector type, workgroup size, precision, number of iterations.
char KernelSource_measure_flops[] = "\n" \
  "__kernel void measure_flops(                                                            \n" \
  "  __global float * out,                                                                 \n" \
  "  const unsigned N_ROTATIONS                                                            \n" \
  ") {                                                                                     \n" \
  "  float4 x,y,cs,sn,xx,yy;                                                               \n" \
  "  x = (1.0f, 2.0f, 3.0f, 4.0f);                                                         \n" \
  "  y = (5.0f, 6.0f, 7.0f, 8.0f);                                                         \n" \
  "  cs = native_cos(2.0f);                                                                \n" \
  "  sn = native_sin(2.0f);                                                                \n" \
  "  for (int i=0;i<N_ROTATIONS;i++) {                                                     \n" \
  "    xx = cs * x - sn * y;                                                               \n" \
  "    yy = cs * y + sn * x;                                                               \n" \
  "    x = xx;                                                                             \n" \
  "    y = yy;                                                                             \n" \
  "  }                                                                                     \n" \
  "  out[get_global_id(0)] = dot(x,y);                                                     \n" \
  "}                                                                                       \n" \
  "\n";

// Taken from http://tom.scogland.com/blog/2013/03/29/opencl-errors
// Find short explanation in http://www.techdarting.com/2014/01/opencl-errors.html
const char * get_error_string(cl_int err){
  switch(err){
    case CL_SUCCESS                         /*   0 */: return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND                /*  -1 */: return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE            /*  -2 */: return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE          /*  -3 */: return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE   /*  -4 */: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES                /*  -5 */: return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY              /*  -6 */: return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE    /*  -7 */: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP                /*  -8 */: return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH           /*  -9 */: return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED      /* -10 */: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE           /* -11 */: return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE                     /* -12 */: return "CL_MAP_FAILURE";

    case CL_INVALID_VALUE                   /* -30 */: return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE             /* -31 */: return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM                /* -32 */: return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE                  /* -33 */: return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT                 /* -34 */: return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES        /* -35 */: return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE           /* -36 */: return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR                /* -37 */: return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT              /* -38 */: return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR /* -39 */: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE              /* -40 */: return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER                 /* -41 */: return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY                  /* -42 */: return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS           /* -43 */: return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM                 /* -44 */: return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE      /* -45 */: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME             /* -46 */: return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION       /* -47 */: return "CL_INVALID_KERNEL_DEFINITION";
    case CL_INVALID_KERNEL                  /* -48 */: return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX               /* -49 */: return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE               /* -50 */: return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE                /* -51 */: return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS             /* -52 */: return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION          /* -53 */: return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE         /* -54 */: return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE          /* -55 */: return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET           /* -56 */: return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST         /* -57 */: return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT                   /* -58 */: return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION               /* -59 */: return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT               /* -60 */: return "CL_INVALID_GL_OBJECT";
    case CL_INVALID_BUFFER_SIZE             /* -61 */: return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL               /* -62 */: return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE        /* -63 */: return "CL_INVALID_GLOBAL_WORK_SIZE";
    default : return "Unknown OpenCL error";
  }
}

// General error-checking of return values coming from OpenCL functions.
// Any error will result in termination of the main program.
void check_ocl(cl_int error, int lineno, char * message) {
  if (error != CL_SUCCESS) {
    printf("%s\nError number %d in line %d of %s\n%s\n",
      message, error, lineno, __FILE__, get_error_string(error));
    exit(EXIT_FAILURE);
  }
}

// Allocate all OpenCL ressources in ocl_globals.
int register_kernel_ocl(
  char * KernelSource,
  char * name_of_function,
  unsigned int k_index,
  unsigned short p_index,
  unsigned short d_index
) {
  cl_int error;

  if (k_index >= NUMBER_OF_KERNELS) {
    printf("k_index %d is out of range\n", k_index);
   return FALSE;
  }
  if (p_index >= ocl_globals.platforms) {
    printf("p_index %d is out of range\n", p_index);
   return FALSE;
  }
  if (d_index >= ocl_globals.devices[p_index]) {
    printf("d_index %d is out of range\n", d_index);
   return FALSE;
  }
  /* Insert each OpenCL kernel into the global data. */
  ocl_globals.prog[p_index][d_index][k_index]=clCreateProgramWithSource(ocl_globals.context[p_index][d_index], 1, (const char **) & KernelSource, NULL, &error);
  check_ocl(error, __LINE__, "clCreateProgramWithSource");
  if (clBuildProgram(ocl_globals.prog[p_index][d_index][k_index], 0, NULL, "", NULL, NULL) == CL_BUILD_PROGRAM_FAILURE) {
    printf("Error: building program (clBuildProgram)\n");
    size_t build_log_size;
    clGetProgramBuildInfo(ocl_globals.prog[p_index][d_index][k_index], ocl_globals.device[p_index][d_index], CL_PROGRAM_BUILD_LOG, 0, NULL, &build_log_size);
    char build_log[build_log_size+1];
    clGetProgramBuildInfo(ocl_globals.prog[p_index][d_index][k_index], ocl_globals.device[p_index][d_index], CL_PROGRAM_BUILD_LOG, build_log_size, build_log, NULL);
    printf("%s\n", build_log);
   return FALSE;
  }
  ocl_globals.kernel_binary[p_index][d_index][k_index]=clCreateKernel(ocl_globals.prog[p_index][d_index][k_index], name_of_function, &error);
  check_ocl(clGetDeviceInfo(ocl_globals.device[p_index][d_index], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(ocl_globals.maxDimension[p_index][d_index]), &ocl_globals.maxDimension[p_index][d_index], NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
  //printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t%u\n", (unsigned int) ocl_globals.maxDimension[p_index][d_index]);
  check_ocl(clGetDeviceInfo(ocl_globals.device[p_index][d_index], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(ocl_globals.maxWorkItemSizes[p_index][d_index]), &ocl_globals.maxWorkItemSizes[p_index][d_index], NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_SIZES");
  check_ocl(clGetKernelWorkGroupInfo(ocl_globals.kernel_binary[p_index][d_index][k_index], ocl_globals.device[p_index][d_index],
  	 CL_KERNEL_WORK_GROUP_SIZE, sizeof(ocl_globals.maxWorkGroupSize[p_index][d_index]), (void *) &ocl_globals.maxWorkGroupSize[p_index][d_index], NULL),
                __LINE__, "clGetKernelWorkGroupInfo CL_KERNEL_WORK_GROUP_SIZE");
  check_ocl(error, __LINE__, "clCreateKernel string_reversal");
return TRUE;
}

// Allocate all OpenCL ressources in ocl_globals.
// Find all platforms and for each platform find all devices.
// Register each kernel for each device.
int start_ocl(int verbose)
{
int err;
  cl_int error;
  cl_context_properties properties[3];
  cl_uint pi, di;
  error=clGetPlatformIDs(MAX_NUMBER_OF_PLATFORMS, 
                    ocl_globals.platform, &ocl_globals.platforms);
  if (error != CL_SUCCESS)
    {
    if(verbose)
      {
      printf("\nNo OpenCL platform found. Make sure you have installed");
      printf("\nthe appropriate packages.");
      printf("\n    Use clinfo to verify that your drivers are OK.");
      return FALSE;
      }
    }
  for (pi=0; pi<ocl_globals.platforms; pi++)
    {
    error=clGetDeviceIDs(ocl_globals.platform[pi], 
                      CL_DEVICE_TYPE_ALL, MAX_NUMBER_OF_DEVICES, 
                      ocl_globals.device[pi], &ocl_globals.devices[pi]);
    if (error != CL_SUCCESS)
      {
      if(verbose)
        {
        printf("\nFailed to get OpenCL device IDs ");
        return FALSE;
        }
      }
    for (di=0; di<ocl_globals.devices[pi]; di++) {
      properties[0] = CL_CONTEXT_PLATFORM;
      properties[1] = (cl_context_properties)ocl_globals.platform[pi];
      properties[2] = 0;
      ocl_globals.context[pi][di]=clCreateContext(properties, 1, &ocl_globals.device[pi][di], NULL, NULL, &error);
      check_ocl(error, __LINE__, "clCreateContext");
      cl_command_queue_properties props = 0;
      // Why use CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ?
      // Because enqueued jobs are faster executed out-of-order.
      // Why not use CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ?
      // 1. the user may have overlooked dependencies among enqueued jobs
      // 2. few OpenCL implementations support it
      // 3. some OpenCL implementations get it wrong and spoil the data
      // 4. overlapping work should better be done with multiple queues
      // props |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
      // clCreateCommandQueue is deprecated in OpenCL 2.
      // But we must use it in OpenCL 1 environments.
      // http://stackoverflow.com/questions/28500496/opencl-function-found-deprecated-by-visual-studio
      ocl_globals.cq[pi][di] = clCreateCommandQueue(ocl_globals.context[pi][di], ocl_globals.device[pi][di], props, &error);
      check_ocl(error, __LINE__, "clCreateCommandQueue");

      // This is the list of OpenCL kernels that can be used by the wrapper functions.
      err=register_kernel_ocl(KernelSource_string_reversal,                 "string_reversal",         0, pi, di);
      if(err == FALSE)goto regerr;
      err=register_kernel_ocl(KernelSource_naive_fourier_transform,         "naive_fourier_transform", 1, pi, di);
      if(err == FALSE)goto regerr;
      err=register_kernel_ocl(KernelSource_stockham_fft_radix_2,            "stockham_fft_radix_2",    2, pi, di);
      if(err == FALSE)goto regerr;
      err=register_kernel_ocl(KernelSource_measure_flops,                   "measure_flops",           3, pi, di);
      if(err == FALSE)goto regerr;
    }
  }
return TRUE;
regerr:;
printf("\nregister_kernel_ocl failed.");
return FALSE;
}

// Free all OpenCL ressources allocated in ocl_globals.
void stop_ocl(void) {
  cl_uint pi, di;

  for (pi=0; pi<ocl_globals.platforms; pi++) {
    for (di=0; di<ocl_globals.devices[pi]; di++) {
      check_ocl(clFinish(ocl_globals.cq[pi][di]), __LINE__, "clFinish");
      check_ocl(clReleaseCommandQueue(ocl_globals.cq[pi][di]), __LINE__, "clReleaseCommandQueue");
      check_ocl(clReleaseContext(ocl_globals.context[pi][di]), __LINE__, "clReleaseContext");
      int i;
      for(i=0; i<NUMBER_OF_KERNELS; i++) {
        check_ocl(clReleaseKernel(ocl_globals.kernel_binary[pi][di][i]), __LINE__, "clReleaseKernel");
        check_ocl(clReleaseProgram(ocl_globals.prog[pi][di][i]), __LINE__, "clReleaseProgram");
      }
    }
  }
}

// A wrapper function that can be invoked by users who are ignorant
// about the details of implementation. All details about the handling
// of OpenCL at run-time are hidden inside this function.
void string_reversal_ocl(
  char* buf_input,
  char* buf_output,
  const short p_index,
  const short d_index
) {
  size_t worksize=strlen(buf_input);
  cl_int error;
  cl_mem mem1, mem2;

  // Create buffers for handing over the input and the output string to the OpenCL kernel.
  mem1=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_READ_ONLY, worksize, NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem1");
  mem2=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_WRITE_ONLY, worksize, NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem2");
  // Set up the newly created buffers as arguments 1 and 2 of the kernel.
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][0], 0, sizeof(cl_mem), &mem1), __LINE__, "clSetKernelArg mem1");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][0], 1, sizeof(cl_mem), &mem2), __LINE__, "clSetKernelArg mem2");
  // Copy the input string onto the GPU memory and wait for completion.
  check_ocl(clEnqueueWriteBuffer(ocl_globals.cq[p_index][d_index], mem1, CL_TRUE, 0, worksize, buf_input, 0, NULL, NULL), __LINE__, "clEnqueueWriteBuffer mem1");
  cl_event events[1];
  // Let the GPU do the string reversal and wait for completion.
  check_ocl(clEnqueueNDRangeKernel(ocl_globals.cq[p_index][d_index], ocl_globals.kernel_binary[p_index][d_index][0], 1, NULL, &worksize, NULL, 0, NULL, &events[0]), __LINE__, "clEnqueueNDRangeKernel string_reversal");
  check_ocl(clWaitForEvents(1, &events[0]), __LINE__, "clWaitForEvents");
  check_ocl(clReleaseEvent(events[0]), __LINE__, "clReleaseEvent");
  // Copy the output string from the GPU memory and wait for completion.
  check_ocl(clEnqueueReadBuffer(ocl_globals.cq[p_index][d_index], mem2, CL_TRUE, 0, worksize, buf_output, 0, NULL, NULL), __LINE__, "clEnqueueReadBuffer mem2");
  check_ocl(clReleaseMemObject(mem1), __LINE__, "clReleaseMemObject mem1");
  check_ocl(clReleaseMemObject(mem2), __LINE__, "clReleaseMemObject mem2");
}

// A wrapper function for a naive (slow) discrete fourier transform.
void naive_fourier_transform_ocl(
  const float  * in,      // complex values input
  float        * out,     // complex values output
  const unsigned length,  // number of input and output values
  const int      sign,    // sign modifier in the exponential :
                          // 1 for forward transform, -1 for backward.
  const float    scale,   // output scale factor
  const short    p_index,
  const short    d_index
)
{
  cl_int error;
  cl_mem mem1, mem2;
  // Create buffers for handing over the input and the output array to the OpenCL kernel.
  mem1=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_READ_ONLY, 2*length*sizeof(float), NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem1");
  mem2=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_WRITE_ONLY, 2*length*sizeof(float), NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem2");
  // Set up the newly created buffers as arguments 1 and 2 of the kernel.
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][1], 0, sizeof(cl_mem),   &mem1),    __LINE__, "clSetKernelArg mem1");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][1], 1, sizeof(cl_mem),   &mem2),    __LINE__, "clSetKernelArg mem2");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][1], 2, sizeof(unsigned), &length),  __LINE__, "clSetKernelArg length");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][1], 3, sizeof(int),      &sign),    __LINE__, "clSetKernelArg sign");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][1], 4, sizeof(float),   &scale),   __LINE__, "clSetKernelArg scale");
  // Copy the input array onto the GPU memory and wait for completion.
  check_ocl(clEnqueueWriteBuffer(ocl_globals.cq[p_index][d_index], mem1, CL_TRUE, 0, 2*length*sizeof(float), in, 0, NULL, NULL), __LINE__, "clEnqueueWriteBuffer in");
  cl_event events[1];
  // Let the GPU do the discrete fourier transform and wait for completion.
  size_t worksize=length;
  check_ocl(clEnqueueNDRangeKernel(ocl_globals.cq[p_index][d_index], ocl_globals.kernel_binary[p_index][d_index][1], 1, NULL, &worksize, NULL, 0, NULL, &events[0]), __LINE__, "clEnqueueNDRangeKernel stockham_fft_radix_2");
  check_ocl(clWaitForEvents(1, &events[0]), __LINE__, "clWaitForEvents");
  check_ocl(clReleaseEvent(events[0]), __LINE__, "clReleaseEvent");
  // Copy the output array from the GPU memory and wait for completion.
  check_ocl(clEnqueueReadBuffer(ocl_globals.cq[p_index][d_index], mem2, CL_TRUE, 0, 2*length*sizeof(float), out, 0, NULL, NULL), __LINE__, "clEnqueueReadBuffer mem2");
  check_ocl(clReleaseMemObject(mem1), __LINE__, "clReleaseMemObject mem1");
  check_ocl(clReleaseMemObject(mem2), __LINE__, "clReleaseMemObject mem2");
}

// A wrapper function for stockham's fast fourier transform.
// Inspired by https://raw.github.com/miracle2121/hpc12-fp-rl1609/master/clfft.c
// When processing large amounts of data, almost all the processing time
// of this C function is spent in these function calls:
//   - clEnqueueWriteBuffer      copy data to the GPU
//   - clEnqueueNDRangeKernel    do the numerical calculations
//   - clEnqueueReadBuffer       copy data from the GPU
// These are the factors that put a limit on speed.
// There are several ways to escape the limit:
//   - All these functions can run asynchronously in the background
//     (with the of use cl_event). This allows to do useful things
//     while the copying/calculations take place.
//   - "Pinned Memory" allows faster copying of data.
//     It comes in two variants:
//       - copying data via DMA at the speed of the PCIe bus
//       - mapping physical memory pages instead of copying
//     In both cases the buffers need to be aligned to page boundaries (page-locked).
//     Mapping is definitely fastest (almost 0 delay) but it is difficult.
//     The pointers to the input and output buffers need to be allocated
//     by OpenCL clCreateBuffer with the flag CL_MEM_ALLOC_HOST_PTR.
//     While copying is done with clEnqueueWriteBuffer, mapping is done
//     with clEnqueueMapBuffer and clEnqueueUnmapMemObject.
//     http://www.nvidia.com/content/cudazone/CUDABrowser/downloads/papers/NVIDIA_OpenCL_BestPracticesGuide.pdf
//     http://developer.download.nvidia.com/compute/DevZone/OpenCL/Projects/oclBandwidthTest.tar.gz
//     http://software.intel.com/sites/products/documentation/ioclsdk/2013/OG/Mapping_Memory_Objects_(USE_HOST_PTR).htm
//     http://stackoverflow.com/questions/5209214/default-pinned-memory-vs-zero-copy-memory
void stockham_fft_radix_2_ocl(
  const float    * in,    // complex values input
  float          * out,   // complex values output
  const unsigned length,  // number of complex samples 
  const unsigned count,   // number of arrays 
  const int      sign,    // sign modifier in the exponential :
                          // 1 for forward transform, -1 for backward.
  const float    scale,   // output scale factor
  const short    p_index, // use platform p_index
  const short    d_index  // use device d_index of platform p_index
)
{
  cl_int error;
  cl_mem mem_gpu[2];
  unsigned short  in_index = 0;
  unsigned short out_index = 1;

  // Create buffers for handing over the input and the output array to the OpenCL kernel.
  mem_gpu[ in_index]=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_READ_WRITE, 2*length*count*sizeof(float), NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem_gpu[in_index]");
  mem_gpu[out_index]=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_READ_WRITE, 2*length*count*sizeof(float), NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem_gpu[out_index]");
  // Copy the input array onto the GPU memory and wait for completion.
  check_ocl(clEnqueueWriteBuffer(ocl_globals.cq[p_index][d_index], mem_gpu[in_index], CL_TRUE, 0, 2*length*count*sizeof(float), in, 0, NULL, NULL), __LINE__, "clEnqueueWriteBuffer in");
  cl_event events[1];
  unsigned logN = (unsigned) (log(length) / log(2.0));
  unsigned block_size = 1;
  size_t worksize = length / 2;
  unsigned t;

  // We use a 2D approach for work-group allocation.
  // See Section 3.2 Execution model (page 23) in the OpenCL 1.1 specification.
  size_t m_globalThreads[] = { worksize, count };
  size_t m_localThreads [] = { worksize,    1  };

  // If the number of data points is larger than the maximum size
  // of a workgroup then choose a different value for workgroup size.
  // Choose ever smaller values until the global size (number of data points)
  // is evenly divisible by the local size.
  size_t divisor = 1;
  while (m_localThreads [0] > ocl_globals.maxWorkItemSizes[p_index][d_index][2] ||
         worksize != (m_localThreads [0] * divisor )) {
    divisor ++ ;
    m_localThreads [0] = worksize / divisor;
  }
  // printf("m_globalThreads = {%ld, %ld}\n", m_globalThreads[0], m_globalThreads[1]);
  // printf("m_localThreads  = {%ld, %ld}\n", m_localThreads[0], m_localThreads[1]);

  // Wouldn't it be better to put the loop below into the OpenCL kernel ?
  // Two advantages come to mind:
  //   - only 1 kernel invokation, not logN invokations
  //   - in case of reverse transform the scaling of the output can be done in one step
  //     error propagation would be better with one scaling instead of logN scalings
  // Unfortunately, it turned out to be a bad idea.
  //   - the speedup was roughly 10% on a GPU, but on a CPU there was a severe slowdown
  //   - error propagation was indeed diminished and output more precise
  //   - but there is no way of synchronizing the work items after each t
  //     OpenCL has no concept for inter-workgroup synchronization
  //     the barrier() function synchronizes only intra-workgroup (inside)
  for (t = 1; t <= logN; t++) {
    // printf("t = %d block_size = %d worksize = %ld\n", t, block_size, worksize);
    // Set up the buffers as arguments 1 and 2 of the kernel.
    check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][2], 0, sizeof(cl_mem),   &mem_gpu[ in_index]), __LINE__, "clSetKernelArg mem_gpu[ in_index]");
    check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][2], 1, sizeof(cl_mem),   &mem_gpu[out_index]), __LINE__, "clSetKernelArg mem_gpu[out_index]");
    check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][2], 2, sizeof(unsigned), &block_size),         __LINE__, "clSetKernelArg block_size");
    check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][2], 3, sizeof(int),      &sign),               __LINE__, "clSetKernelArg sign");
    check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][2], 4, sizeof(float),&scale),              __LINE__, "clSetKernelArg scale");
    // Let the GPU do the discrete fourier transform and wait for completion.
    check_ocl(clEnqueueNDRangeKernel(ocl_globals.cq[p_index][d_index], ocl_globals.kernel_binary[p_index][d_index][2],
 2, NULL, m_globalThreads, m_localThreads, 0, NULL, &events[0]), __LINE__, "clEnqueueNDRangeKernel stockham_fft_radix_2_ocl");
    check_ocl(clWaitForEvents(1, &events[0]), __LINE__, "clWaitForEvents");
    check_ocl(clReleaseEvent(events[0]), __LINE__, "clReleaseEvent");
    // Swap in and out.
    // TODO: This is buggy. Works only with odd logN.
    unsigned short tmp_index =  in_index;
    in_index                 = out_index;
    out_index                = tmp_index;
    block_size   *= 2;
  }
  // Copy the output array from the GPU memory and wait for completion.
  check_ocl(clEnqueueReadBuffer(ocl_globals.cq[p_index][d_index], mem_gpu[in_index], CL_TRUE, 0, 2*length*count*sizeof(float), out, 0, NULL, NULL), __LINE__, "clEnqueueReadBuffer mem_gpu[out_index]");
  // Release the GPU memory.
  check_ocl(clReleaseMemObject(mem_gpu[ in_index]), __LINE__, "clReleaseMemObject mem_gpu[ in_index]");
  check_ocl(clReleaseMemObject(mem_gpu[out_index]), __LINE__, "clReleaseMemObject mem_gpu[out_index]");
}

void measure_flops_ocl(
  float * buffer,
  unsigned N,
  unsigned N_ROTATIONS,
  const short p_index,    // use platform p_index
  const short d_index     // use device d_index of platform p_index
) {
  size_t worksize=N;
  cl_int error;
  cl_mem mem1;

  mem1=clCreateBuffer(ocl_globals.context[p_index][d_index], CL_MEM_READ_WRITE, 2*N*sizeof(float), NULL, &error);
  check_ocl(error, __LINE__, "clCreateBuffer mem1");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][3], 0, sizeof(cl_mem), &mem1), __LINE__, "clSetKernelArg mem1");
  check_ocl(clSetKernelArg(ocl_globals.kernel_binary[p_index][d_index][3], 1, sizeof(unsigned), &N_ROTATIONS), __LINE__, "clSetKernelArg length");
  check_ocl(clEnqueueWriteBuffer(ocl_globals.cq[p_index][d_index], mem1, CL_TRUE, 0, worksize, buffer, 0, NULL, NULL), __LINE__, "clEnqueueWriteBuffer mem1");
  cl_event events[1];
  check_ocl(clEnqueueNDRangeKernel(ocl_globals.cq[p_index][d_index], ocl_globals.kernel_binary[p_index][d_index][3], 1, NULL, &worksize, NULL, 0, NULL, &events[0]), __LINE__, "clEnqueueNDRangeKernel string_reversal");
  check_ocl(clWaitForEvents(1, &events[0]), __LINE__, "clWaitForEvents");
  check_ocl(clReleaseEvent(events[0]), __LINE__, "clReleaseEvent");
  check_ocl(clReleaseMemObject(mem1), __LINE__, "clReleaseMemObject mem1");
}

// Describe the OpenCL run-time environment.
void info_ocl(void) {
  cl_platform_id platforms[100];
  cl_uint platforms_n = 0;
  unsigned int p_index, d_index;

  cl_int error = clGetPlatformIDs(100, platforms, &platforms_n);
  if (platforms_n == 0) {
    printf("\nNo OpenCL platform found\n");
    return;
  }
  check_ocl(error, __LINE__, "clGetPlatformIDs platforms");
  printf("====================================\n");
  printf("=== %d OpenCL platform(s) found: ===\n", platforms_n);
  printf("====================================\n\n");
  for (p_index=0; p_index<platforms_n; ++p_index) {
    cl_device_id devices[100];
    cl_uint devices_n = 0;
    char buffer[10240];

    printf("-----------------------------------------------\nPLATFORM: %d\n", p_index);
    check_ocl(clGetPlatformInfo(platforms[p_index], CL_PLATFORM_PROFILE, sizeof(buffer), buffer, NULL),
                  __LINE__, "clGetPlatformInfo CL_PLATFORM_PROFILE");
    printf("PROFILE = %s\n", buffer);
    check_ocl(clGetPlatformInfo(platforms[p_index], CL_PLATFORM_VERSION, sizeof(buffer), buffer, NULL),
                  __LINE__, "clGetPlatformInfo CL_PLATFORM_VERSION");
    printf("VERSION = %s\n", buffer);
    check_ocl(clGetPlatformInfo(platforms[p_index], CL_PLATFORM_NAME, sizeof(buffer), buffer, NULL),
                  __LINE__, "clGetPlatformInfo CL_PLATFORM_NAME");
    printf("NAME = %s\n", buffer);
    check_ocl(clGetPlatformInfo(platforms[p_index], CL_PLATFORM_VENDOR, sizeof(buffer), buffer, NULL),
                  __LINE__, "clGetPlatformInfo CL_PLATFORM_VENDOR");
    printf("VENDOR = %s\n", buffer);
    check_ocl(clGetPlatformInfo(platforms[p_index], CL_PLATFORM_EXTENSIONS, sizeof(buffer), buffer, NULL),
                  __LINE__, "clGetPlatformInfo CL_PLATFORM_EXTENSIONS");
    printf("EXTENSIONS = %s\n", buffer);
    
    cl_uint dtype = CL_DEVICE_TYPE_ALL;
    check_ocl(clGetDeviceIDs(platforms[p_index], dtype, 100, devices, &devices_n),
                  __LINE__, "clGetPlatformInfo devices");
    for (d_index=0; d_index<devices_n; d_index++) {
      cl_uint buf_uint;
      cl_ulong buf_ulong;
      printf("***********************************************\nPLATFORM: %d DEVICE: %d\n", p_index, d_index);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_NAME");
      printf("DEVICE_NAME = %s\n", buffer);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_VENDOR");
      printf("DEVICE_VENDOR = %s\n", buffer);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_VERSION");
      printf("DEVICE_VERSION = %s\n", buffer);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL),
                      __LINE__, "clGetDeviceInfo CL_DRIVER_VERSION");
      printf("DRIVER_VERSION = %s\n", buffer);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_COMPUTE_UNITS");
      printf("DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_CLOCK_FREQUENCY");
      printf("DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_GLOBAL_MEM_SIZE");
#if OSNUM == OSNUM_LINUX
#if IA64 == 1
      printf("DEVICE_GLOBAL_MEM_SIZE = %lu\n", (uint64_t)buf_ulong);
#else
      printf("DEVICE_GLOBAL_MEM_SIZE = %llu\n", (uint64_t)buf_ulong);
#endif
#endif
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(buf_ulong), &buf_ulong, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_WORK_GROUP_SIZE");
#if OSNUM == OSNUM_LINUX
#if IA64 == 1
      printf("DEVICE_MAX_WORK_GROUP_SIZE = %lu\n", (uint64_t)buf_ulong);
#else	  
      printf("DEVICE_MAX_WORK_GROUP_SIZE = %llu\n", (uint64_t)buf_ulong);
#endif

#endif
      size_t workitem_dims;
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(workitem_dims), &workitem_dims, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
      printf("CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t%u\n", (unsigned int) workitem_dims);
	  
      size_t workitem_size[3];
      check_ocl(clGetDeviceInfo(devices[d_index], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(workitem_size), &workitem_size, NULL),
                      __LINE__, "clGetDeviceInfo CL_DEVICE_MAX_WORK_ITEM_SIZES");
      printf("CL_DEVICE_MAX_WORK_ITEM_SIZES:\t%d / %d / %d \n", (int) workitem_size[0], (int) workitem_size[1], (int) workitem_size[2]);

      size_t param_value[3] = {0, 0, 0};
      check_ocl(clGetKernelWorkGroupInfo(ocl_globals.kernel_binary[p_index][d_index][0], ocl_globals.device[p_index][d_index],
  	 CL_KERNEL_WORK_GROUP_SIZE, sizeof(param_value), (void *) &param_value, NULL),
                __LINE__, "clGetKernelWorkGroupInfo CL_KERNEL_WORK_GROUP_SIZE");
      printf("CL_KERNEL_WORK_GROUP_SIZE:\t%d\n", (int) param_value[0]);

      // Let each device on each platform say hello.
      printf("platform %d device %d says: ", p_index, d_index);
      char bufc_input   []= "!dlroW ,olleH";
      char bufc_output  []= "             ";
      char bufc_reverse []= "             ";
      string_reversal_ocl(bufc_input , bufc_output , p_index, d_index);
      string_reversal_ocl(bufc_output, bufc_reverse, p_index, d_index);
      printf("%s\n", bufc_output);
        if (strcmp(bufc_input, bufc_reverse)==0)
          printf("string_reversal_ocl succeeded\n");
        else
          printf("string_reversal_ocl produced wrong string\n");

      {
        // A trivial FFT regression test inspired by
        // http://rosettacode.org/wiki/Fast_Fourier_transform#C
        float complex buf_input  [] = {1, 2, 3, 4, 0, 0, 0, 0};
        float complex buf_output [] = {0, 0, 0, 0, 0, 0, 0, 0};
        float complex buf_reverse[] = {0, 0, 0, 0, 0, 0, 0, 0};
        int i;
        naive_fourier_transform_ocl ((float *) & buf_input,  (float *) &buf_output,  8,  1, (float) 1.0,   p_index, d_index);
        naive_fourier_transform_ocl ((float *) & buf_output, (float *) &buf_reverse, 8, -1, (float) 0.125, p_index, d_index);
        int failed = 0;
        for (i = 0; i < 8; i++)
          failed |= cabs(buf_reverse[i] - buf_input[i]) > 0.00001;
        if (failed)
          {
          printf("naive_fourier_transform_ocl produced numerical error\n");
for (i=0; i<8; i++)
printf("\n%d %f  (%f, %f)  (%f, %f)",i,cabs(buf_reverse[i] - buf_input[i]),
creal(buf_reverse[i]),cimag(buf_reverse[i]),
creal(buf_input[i]),cimag(buf_input[i]));
printf("\n");
          }
        else
          printf("naive_fourier_transform_ocl succeeded\n");
      }
      {
        float complex buf_input  [] = {1, 2, 3, 4, 0, 0, 0, 0};
        float complex buf_output [] = {0, 0, 0, 0, 0, 0, 0, 0};
        float complex buf_reverse[] = {0, 0, 0, 0, 0, 0, 0, 0};
        int i;
        stockham_fft_radix_2_ocl((float *) & buf_input,  (float *) &buf_output,  8,  1, 1, (float) 1.0, p_index, d_index);
//        for (i = 0; i < 8; i++) {
//          if (!cimag(buf_output[i]))
//            printf("%g ", creal(buf_output[i]));
//          else
//            printf("(%g, %g) ", creal(buf_output[i]), cimag(buf_output[i]));
//        }
//        printf("\n");
        stockham_fft_radix_2_ocl((float *) & buf_output, (float *) &buf_reverse, 8, 1, -1, (float) 0.5, p_index, d_index);
        int failed = 0;
        for (i = 0; i < 8; i++) {
          failed |= cabs(buf_reverse[i] - buf_input[i]) > 0.00001;
//          if (!cimag(buf_reverse[i]))
//            printf("%g ", creal(buf_reverse[i]));
//          else
//            printf("(%g, %g) ", creal(buf_reverse[i]), cimag(buf_reverse[i]));
        }
//        printf("\n");
        if (failed)
          {
          printf("stockham_fft_radix_2_ocl produced numerical error\n");

for (i=0; i<8; i++)
printf("\n%d %f  (%f, %f)  (%f, %f)",i,cabs(buf_reverse[i] - buf_input[i]),
creal(buf_reverse[i]),cimag(buf_reverse[i]),
creal(buf_input[i]),cimag(buf_input[i]));
printf("\n");
          }
        else
          printf("stockham_fft_radix_2_ocl succeeded\n");
      }
      {
        // We want to work with a sample rate fs = 200 MHz.
        // We want to process blocks of 2^18 = 262144 data points in one FFT.
        // This results in a frequency resolution of 0.7 kHz per bin.
        // 16 data blocks of size 262144 take 21 milliseconds.
        // Real-time FFT must process 16 FFTs in less than 21 milliseconds.
        float    fs   = 2E8;                     // sample rate
        unsigned logN = 18;                      // the dual log of the size of one   array  of size N
        unsigned    M = 16;                      // the number M of arrays of size N
        unsigned    N = 1 << logN;               // the size of one array
        unsigned  N_M = N * M;                   // the size of all M arrays of size N
        float complex * buf_input   = malloc(N_M * sizeof(float complex));
        float complex * buf_output  = malloc(N_M * sizeof(float complex));
        float complex * buf_reverse = malloc(N_M * sizeof(float complex));
        // Fill all the blocks with 0.
        // For each block, put a unit impulse into one of the samples.
        // In block i, the impulse is in sample i.
        unsigned i, j;
        for (i=0; i< M; i++) {
          for (j=0; j< N; j++)
            buf_input[i*N + j] = 0.0f + 0.0f * I;
          buf_input  [i*N + i] = 1.0f + 0.0f * I;
        }
        oclHandleType handle    = create_clFFT_plan(N, M, 1.0f, p_index, d_index);
        // Run the forward FFT to initialize everything.
        // stockham_fft_radix_2_ocl((float *) buf_input,  (float *) buf_output,  N, M, 1, (float) 1.0, p_index, d_index);
        execute_clFFT_plan(handle, CLFFT_FORWARD , (void *) buf_input, (void *) buf_output);
        struct timeval  begin, end;
        gettimeofday(&begin, NULL);
        // Now that all buffers are initialized, do the timing measurement with the backward FFT.
        // stockham_fft_radix_2_ocl((float *) buf_output, (float *) buf_reverse, N, M, -1, (float ) 0.5, p_index, d_index);
        execute_clFFT_plan(handle, CLFFT_BACKWARD, (void *) buf_output, (void *) buf_reverse);
        gettimeofday(&end, NULL);
        float run_time= (float) (end.tv_usec - begin.tv_usec) / 1000000 +
                        (float) (end.tv_sec  - begin.tv_sec);
        int failed = 0;
        for (i=0; i< M; i++) {
          for (j=0; j< N; j++) {
            float delta = cabs(buf_reverse[i*N + j] - buf_input[i*N + j]);
            failed |= delta > 0.000001;
            if (delta > 0.000001)
              printf("found delta %6.2f in block %3d sample %3d\n", delta, i, j);
          }
        }
        printf("stockham_fft_radix_2_ocl of length 2^%d (delay %.0f ms, frequency resolution %.1f kHz) with %d data arrays ",
               logN, 1000.0 * N_M / fs, fs / N / 1000.0, M);
        if (failed)
          printf("produced numerical error ");
        else
          printf("succeeded ");
        printf("in %.0f milliseconds per block (%.0f%% of real-time) with %.1f GFLOPS\n",
                1000*run_time, 100*(N_M/fs)/run_time, (M * 14.0 * N * logN) / run_time / 1E9);
        destroy_clFFT_plan(&handle);
        free(buf_input);
        free(buf_output);
        free(buf_reverse);
      }

    ocl_compute_gflops(p_index, d_index);
      // Let each device on each platform calculate GFLOPS.
      printf("measure_flops_ocl reports %f GFLOPS can be reached in this device\n",
                            ocl_globals.gflops[p_index][d_index]);
    } // for
    printf("-----------------------------------------------\n\n");
  } // for
}

void ocl_compute_gflops(unsigned int p_index, unsigned int d_index)
{
// Let each device on each platform calculate GFLOPS.
unsigned N = 2048;
unsigned N_ROTATIONS = 1000*1000;
float * fbuffer   = malloc(N * sizeof(float));
struct timeval  begin, end;
gettimeofday(&begin, NULL);
measure_flops_ocl(fbuffer, N, N_ROTATIONS, p_index, d_index);
gettimeofday(&end, NULL);
float run_time= (float) (end.tv_usec - begin.tv_usec) / 1000000 +
                      (float) (end.tv_sec  - begin.tv_sec);
ocl_globals.gflops[p_index][d_index]=(4.0 * 6 * N * N_ROTATIONS + N + 2) / run_time / 1E9;
}

// End of OpenCL part.
/***********************************************************************/
// Beginning of clFFT part.

// Linrad uses clfftPlanHandle along with some other internal variables.
struct oclPlanType {
  clfftPlanHandle handle;
  size_t lengths[1];           // All our data is 1 dimensional data
  size_t batch_size;           // The number of data sets processed simultaneously 
  cl_context       context;    // Use context and queue as defined above.
  cl_command_queue queue;      // Use context and queue of one specific device
  cl_mem           cl_buffer;
  size_t           buflen;
  size_t           tmpBuffferSize;
  cl_mem           clMedBuffer;
};

oclHandleType create_clFFT_plan(
  size_t array_length,
  size_t batch_size,
  float scale,
  unsigned short platform,
  unsigned short device
) {
  cl_int status;
  size_t clStrides[2];
  if (platform >= ocl_globals.platforms         ||
      device   >= ocl_globals.devices[platform]  )return NULL;
  struct oclPlanType * plan = (struct oclPlanType *) malloc(sizeof(struct oclPlanType));
  if (plan == NULL)return NULL;
  plan->lengths[0] = array_length;
  plan->batch_size = batch_size;
  // Use the default context of the device, but don't use the default queue.
  plan->context    = ocl_globals.context[platform][device];
  // Every clFFT plan shall use its own queue.
  // This allows for overlapping work of all existing clFFT plans.
  plan->queue      = clCreateCommandQueue(ocl_globals.context[platform][device], ocl_globals.device[platform][device], 0, &status);
  check_ocl(status, __LINE__, "clCreateCommandQueue");
  plan->buflen     = 2 * plan->batch_size * plan->lengths[0] * sizeof(float);
  plan->cl_buffer  = clCreateBuffer(plan->context, CL_MEM_READ_WRITE, plan->buflen,  NULL, &status);
  check_ocl( status,  __LINE__, "clCreateBuffer cl_buffer");
  check_ocl( clfftCreateDefaultPlan(&plan->handle, plan->context, CLFFT_1D, plan->lengths), __LINE__, "clfftCreateDefaultPlan");
  check_ocl( clfftSetPlanBatchSize (plan->handle, plan->batch_size), __LINE__, "clfftCreateDefaultPlan");
  check_ocl( clfftSetPlanPrecision(plan->handle, CLFFT_SINGLE), __LINE__, "clfftSetPlanPrecision");
  check_ocl( clfftSetLayout(plan->handle, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED), __LINE__, "clfftSetLayout");
  check_ocl( clfftSetResultLocation(plan->handle, CLFFT_INPLACE), __LINE__, "clfftSetResultLocation");
  clStrides[0]=1;
  check_ocl( clfftSetPlanInStride(plan->handle, CLFFT_1D, clStrides), __LINE__, "clFFT set in Strides");
  check_ocl( clfftSetPlanOutStride(plan->handle, CLFFT_1D, clStrides), __LINE__, "clFFT set in Strides");
  check_ocl( clfftSetPlanScale(plan->handle, CLFFT_FORWARD, (cl_float) scale), __LINE__, "clfftSetPlanScale");
  check_ocl( clfftBakePlan(plan->handle, 1, &plan->queue, NULL, NULL), __LINE__, "clfftBakePlan");
  size_t tmpBuffersize=0;
  clfftGetTmpBufSize(plan->handle, &tmpBuffersize);
  plan->clMedBuffer=NULL;
  if (tmpBuffersize) {
    plan->clMedBuffer = clCreateBuffer (plan->context, CL_MEM_READ_WRITE, tmpBuffersize, 0, &status);
    check_ocl( status,  __LINE__, "clCreateBuffer clMedBuffer");
  }
  return (oclHandleType) plan;
}

int execute_clFFT_plan(
  oclHandleType handle,
  int direction,
  void * input,
  void * output)
{
  if (handle == NULL)
    return -1;
  struct oclPlanType * plan = (struct oclPlanType *) handle;
  check_ocl( clEnqueueWriteBuffer(plan->queue, plan->cl_buffer, CL_TRUE, 0, plan->buflen, input, 0, NULL, NULL), __LINE__, "clEnqueueWriteBuffer mem1");
  check_ocl( clfftEnqueueTransform(plan->handle, direction, 1, &plan->queue, 0, NULL, NULL, &plan->cl_buffer, NULL, NULL), __LINE__, "clfftEnqueueTransform");
  check_ocl( clFinish(plan->queue), __LINE__, "clFinish");
  check_ocl( clEnqueueReadBuffer(plan->queue, plan->cl_buffer, CL_TRUE, 0, plan->buflen, output, 0, NULL, NULL ), __LINE__, "clEnqueueReadBuffer");
  return 0;
}

int destroy_clFFT_plan(oclHandleType * handle_p)
{
  if (handle_p == NULL || *handle_p == NULL)
    return -1;
  struct oclPlanType * plan = (struct oclPlanType *) * handle_p;
  if (plan->cl_buffer)
    check_ocl( clReleaseMemObject(plan->cl_buffer), __LINE__, "clReleaseMemObject cl_buffer");
  if (plan->clMedBuffer)
    check_ocl(clReleaseMemObject(plan->clMedBuffer), __LINE__, "clReleaseMemObject mem1");
  check_ocl(clReleaseCommandQueue(plan->queue), __LINE__, "clReleaseCommandQueue");
  if (plan->handle)
    check_ocl( clfftDestroyPlan(&plan->handle), __LINE__, "clfftDestroyPlan");
  free(plan);
  *handle_p = NULL;
  return 0;
}

void do_clfftTeardown(void)
{
clfftTeardown();
}

// The source code of the function example_clFFT was inspired 
// by http://rosettacode.org/wiki/Fast_Fourier_transform#C.
// This function tests the return values of the API in detail.
void example_clFFT(void) {
  printf("====================================\n");
  printf("===  Testing clFFT library       ===\n");
  printf("====================================\n\n");
  // Look at each device of each platform and report about the test.
  unsigned int pi, di;
  for (pi=0; pi<ocl_globals.platforms; pi++) {
    for (di=0; di<ocl_globals.devices[pi]; di++) {
      float complex buffer [] = {1, 2, 3, 4, 0, 0, 0, 0};
      float complex result [] = {0, 0, 0, 0, 0, 0, 0, 0};
      size_t        datalen   = sizeof buffer / sizeof *buffer;
      printf("example_clFFT on platform %d device %d ", pi, di);
      oclHandleType handle    = create_clFFT_plan(datalen, 1, 1.0f, pi, di);
      if (handle) {
        // Apply the forward FFT out-of-place.
        execute_clFFT_plan(handle, CLFFT_FORWARD , (void *) buffer, (void *) result);
        // Apply the backward FFT in-place.
        execute_clFFT_plan(handle, CLFFT_BACKWARD, (void *) result, (void *) result);
        // This sequence should reproduce the original data.
        int failed = 0;
        unsigned int i;
        for (i = 0; i < datalen; i++)
          failed |= cabs(result[i] - buffer[i]) > 0.000001;
        if (failed)
          printf("produced numerical error\n");
        else
          printf("succeeded\n");
        destroy_clFFT_plan(&handle);
      } else {
        printf("  create_clFFT_plan failed\n");
      }
    } // di
  } // pi
  printf("-----------------------------------------------\n\n");
} // example_clFFT

int ocl_test() 
{
info_ocl();
example_clFFT();
clfftTeardown();
return EXIT_SUCCESS;
}

#ifdef OCLPROGS_NEEDS_A_MAIN_FUNCTION
int main() {
  return ocl_test();
}
#endif
#endif


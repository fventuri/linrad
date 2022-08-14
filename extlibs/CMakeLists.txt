# Build the external libraries.

# Is OpenCL available in this build environment ?
# Then we can build and use the clFFT library for signal processing.
if (OPENCL_FOUND)
  message(STATUS "Preparing build of clFFT")
  set (clFFT.Source
    clFFT/src/library/action.cpp
    clFFT/src/library/transform.cpp
    clFFT/src/library/accessors.cpp
    clFFT/src/library/plan.cpp
    clFFT/src/library/repo.cpp
    clFFT/src/library/fft_binary_lookup.cpp
    clFFT/src/library/generator.stockham.cpp
    clFFT/src/library/generator.transpose.gcn.cpp
    clFFT/src/library/generator.transpose.nonsquare.cpp
    clFFT/src/library/generator.transpose.square.cpp
    clFFT/src/library/generator.copy.cpp
    clFFT/src/library/lifetime.cpp
    clFFT/src/library/md5sum.c
    clFFT/src/library/stdafx.cpp
  )
  set (clFFT.Headers
    clFFT/src/library/action.h
    clFFT/src/library/private.h
    clFFT/src/library/repo.h
    clFFT/src/library/plan.h
    clFFT/src/library/lock.h
    clFFT/src/library/mainpage.h
    clFFT/src/library/fft_binary_lookup.h
    clFFT/src/library/generator.h
    clFFT/src/library/generator.stockham.h
    clFFT/src/library/generator.transpose.gcn.h
    clFFT/src/library/generator.transpose.nonsquare.h
    clFFT/src/library/generator.transpose.square.h
    clFFT/src/library/md5sum.h
    clFFT/src/include/stdafx.h
    clFFT/src/include/unicode.compatibility.h
    clFFT/src/include/targetver.h
    clFFT/src/include/clAmdFft.h
    clFFT/src/include/clFFT.h
  )
  set (clFFT.Files ${clFFT.Source} ${clFFT.Headers})
  set (CLFFT_VERSION_MAJOR 2)
  set (CLFFT_VERSION_MINOR 10)
  set (CLFFT_VERSION_PATCH 0)
  configure_file( "clFFT/src/include/clFFT.version.h.in" "${PROJECT_SOURCE_DIR}/extlibs/clFFT/src/include/clFFT.version.h" )
  add_library( clFFT ${clFFT.Files} )
  target_link_libraries( clFFT ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
  #add_subdirectory(clFFT/src)

  # Build the clpeak executable.
  if (NOT ${OPENCL_CL_HPP_FOUND})
    message(STATUS "CL/cl.hpp: Using the copy that comes with clpeak")
    include_directories("clpeak/src/include/override/")
  endif()
  include_directories(${OPENCL_INCLUDES} "clpeak/src/include" "clpeak/src/kernels")
  set(CLPEAK_SOURCE_FILES
    clpeak/src/common.cpp
    clpeak/src/clpeak.cpp
    clpeak/src/options.cpp
    clpeak/src/logger.cpp
    clpeak/src/global_bandwidth.cpp
    clpeak/src/compute_sp.cpp
    clpeak/src/compute_dp.cpp
    clpeak/src/compute_integer.cpp
    clpeak/src/transfer_bandwidth.cpp
    clpeak/src/kernel_latency.cpp
    clpeak/src/entry.cpp
  )
  if (${CPU} STREQUAL "CPU_ARM")
    # Ugly hack to allow compilation inspite of a bug in CL/cl.hpp.
    add_definitions(-D_mm_mfence=abort)
  endif()
  add_executable(clpeak ${CLPEAK_SOURCE_FILES})
  # Allow modern C++ syntax. Allow it only for local source files.
  set_target_properties(clpeak PROPERTIES COMPILE_FLAGS "-std=c++11 -std=c++11")
  target_link_libraries(clpeak m ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${OPENCL_LIBRARIES})
endif()


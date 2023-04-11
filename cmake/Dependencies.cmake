# Modifications (c) 2019-2021 Advanced Micro Devices, Inc.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Dependencies

# Git
find_package(Git REQUIRED)

# Add some paths
list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH} ${ROCM_PATH}/hip)

# Find OpenMP package
find_package(OpenMP)
if (NOT OPENMP_FOUND)
  message("-- OpenMP not found. Compiling WITHOUT OpenMP support.")
else()
  option(HPCG_OPENMP "Compile WITH OpenMP support." ON)
  if(NOT DEFINED OpenMP_CXX_LIBRARIES OR "${OpenMP_CXX_LIBRARIES}" STREQUAL "")
    message(WARNING "Found OpenMP but not the library")
    message(STATUS "Using the Cray OpenMP library.")
    set(OpenMP_CXX_LIBRARIES "craymp")
  endif()
  message(STATUS "OpenMP flags: ${OpenMP_CXX_FLAGS}")
  message(STATUS "OpenMP libs: ${OpenMP_CXX_LIBRARIES}")
  message(STATUS "OpenMP lib names: ${OpenMP_CXX_LIB_NAMES}")
endif()

# MPI
if (DEFINED HPCG_MPI_DIR)
  set(MPI_HOME ${HPCG_MPI_DIR})
  set(MPI_DIR ${HPCG_MPI_DIR})
  message(STATUS "Looking for MPI in ${HPCG_MPI_DIR}")
endif()
find_package(MPI)
if (NOT MPI_FOUND)
  message("-- MPI not found. Compiling WITHOUT MPI support.")
  if (HPCG_MPI)
    message(FATAL_ERROR "Cannot build with MPI support.")
  endif()
else()
  option(HPCG_MPI "Compile WITH MPI support." ON)
  if (NOT DEFINED MPI_C_INCLUDE_DIRS)
    set(MPI_C_INCLUDE_DIRS "${HPCG_MPI_DIR}/include")
    set(MPI_CXX_INCLUDE_DIRS "${HPCG_MPI_DIR}/include")
  endif()
  message(STATUS "Found MPI C headers at ${MPI_C_INCLUDE_DIRS}.")
  message(STATUS "Found MPI CXX headers at ${MPI_CXX_INCLUDE_DIRS}.")
endif()

# gtest
if(BUILD_TEST)
  find_package(GTest REQUIRED)
endif()

# libnuma if MPI is enabled
if(HPCG_MPI)
  find_package(LIBNUMA REQUIRED)
  find_library(MPI_GTL_HSA_LIB mpi_gtl_hsa PATHS $ENV{CRAY_MPICH_ROOTDIR}/gtl/lib)
  if("${MPI_GTL_HSA_LIB}" STREQUAL "MPI_GTL_HSA_LIB-NOTFOUND")
    message(SEND_ERROR "MPI GTL HSA library not found!")
  else()
    message(STATUS "GTL found at ${MPI_GTL_HSA_LIB}.")
  endif()
endif()

# rocm-cmake
#find_package(ROCM 0.7.3 CONFIG PATHS ${CMAKE_PREFIX_PATH} $ENV{ROCM_PATH})
find_package(ROCM CONFIG PATHS ${CMAKE_PREFIX_PATH} $ENV{ROCM_PATH})
if(NOT ROCM_FOUND)
  set(PROJECT_EXTERN_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps")
  file( TO_NATIVE_PATH "${PROJECT_EXTERN_DIR}" PROJECT_EXTERN_DIR_NATIVE)
  set(rocm_cmake_tag "master" CACHE STRING "rocm-cmake tag to download")
  file(
      DOWNLOAD https://github.com/RadeonOpenCompute/rocm-cmake/archive/${rocm_cmake_tag}.tar.gz
      ${PROJECT_EXTERN_DIR}/rocm-cmake-${rocm_cmake_tag}.tar.gz
      STATUS rocm_cmake_download_status LOG rocm_cmake_download_log
  )
  list(GET rocm_cmake_download_status 0 rocm_cmake_download_error_code)
  if(rocm_cmake_download_error_code)
      message(FATAL_ERROR "Error: downloading "
          "https://github.com/RadeonOpenCompute/rocm-cmake/archive/${rocm_cmake_tag}.zip failed "
          "error_code: ${rocm_cmake_download_error_code} "
          "log: ${rocm_cmake_download_log} "
      )
  endif()

  execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzvf ${PROJECT_EXTERN_DIR}/rocm-cmake-${rocm_cmake_tag}.tar.gz
      WORKING_DIRECTORY ${PROJECT_EXTERN_DIR}
  )
  execute_process(
      COMMAND ${CMAKE_COMMAND} -S ${PROJECT_EXTERN_DIR}/rocm-cmake-${rocm_cmake_tag} -B ${PROJECT_EXTERN_DIR}/rocm-cmake-${rocm_cmake_tag}/build
      WORKING_DIRECTORY ${PROJECT_EXTERN_DIR}
  )
  execute_process(
      COMMAND ${CMAKE_COMMAND} --install ${PROJECT_EXTERN_DIR}/rocm-cmake-${rocm_cmake_tag}/build --prefix ${PROJECT_EXTERN_DIR}/rocm
      WORKING_DIRECTORY ${PROJECT_EXTERN_DIR} )
  if(rocm_cmake_unpack_error_code)
      message(FATAL_ERROR "Error: unpacking ${CMAKE_CURRENT_BINARY_DIR}/rocm-cmake-${rocm_cmake_tag}.zip failed")
  endif()
  find_package(ROCM 0.7.3 REQUIRED CONFIG PATHS ${PROJECT_EXTERN_DIR})
endif()

include(ROCMSetupVersion)
include(ROCMCreatePackage)
include(ROCMInstallTargets)
include(ROCMPackageConfigHelpers)
include(ROCMInstallSymlinks)
include(ROCMCheckTargetIds)
include(ROCMClients)


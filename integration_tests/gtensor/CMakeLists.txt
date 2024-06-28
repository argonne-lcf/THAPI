cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(thapi-integration-test)

if(GTENSOR_DEVICE STREQUAL "cuda")
  enable_language(CUDA)
  find_package(CUDAToolkit REQUIRED)
endif()

set(THAPI_PATH "/opt/thapi" CACHE STRING "Path to thapi installation")

message(STATUS "${PROJECT_NAME}: THAPI_PATH=${THAPI_PATH}")

# add dependencies
include(cmake/CPM.cmake)
CPMFindPackage(NAME gtensor
               GITHUB_REPOSITORY wdmapp/gtensor
               GIT_TAG "main"
               OPTIONS "GTENSOR_ENABLE_BLAS ON"
                       "GTENSOR_ENABLE_SOLVER ON")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LTTNG_CTL REQUIRED lttng-ctl)

add_library(thapi_ctl INTERFACE IMPORTED)
target_include_directories(thapi_ctl INTERFACE "${THAPI_PATH}/include")
target_link_libraries(thapi_ctl INTERFACE "${THAPI_PATH}/lib/libthapi-ctl.so")
target_link_libraries(thapi_ctl INTERFACE ${LTTNG_CTL_LDFLAGS})

add_executable(axpy_start_stop)
target_gtensor_sources(axpy_start_stop PRIVATE axpy_start_stop.cxx)
target_link_libraries(axpy_start_stop gtensor::gtensor)
target_link_libraries(axpy_start_stop thapi_ctl)

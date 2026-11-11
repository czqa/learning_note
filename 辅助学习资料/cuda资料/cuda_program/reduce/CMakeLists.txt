cmake_minimum_required(VERSION 3.20.0)

project(reduce_test LANGUAGES CUDA CXX)

find_package(CUDAToolkit)

add_executable(reduce_v0_global_memory 
            reduce_v0_global_memory.cu)

target_link_libraries(reduce_v0_global_memory PRIVATE CUDA::cudart ${CUDA_cublas_LIBRARY})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(reduce_v0_global_memory PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-G>)
endif()

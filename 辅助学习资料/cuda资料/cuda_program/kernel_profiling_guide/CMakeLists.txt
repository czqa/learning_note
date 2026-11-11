cmake_minimum_required(VERSION 3.20.0)

project(reduce_test LANGUAGES CUDA CXX)

find_package(CUDAToolkit)

add_executable(my_transpose_v5_shared_memory 
            my_transpose_v5_shared_memory.cu)

target_link_libraries(my_transpose_v5_shared_memory PRIVATE CUDA::cudart ${CUDA_cublas_LIBRARY})

# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_options(my_transpose_v2_float4 PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-G>)
# endif()

target_compile_options(my_transpose_v5_shared_memory PRIVATE -lineinfo)
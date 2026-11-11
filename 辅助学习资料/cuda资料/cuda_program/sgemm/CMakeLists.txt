cmake_minimum_required(VERSION 3.20.0)

project(reduce_test LANGUAGES CUDA CXX)

find_package(CUDAToolkit)

add_executable(sgemm_v8_double_buffer 
            sgemm_v8_double_buffer.cu)

target_link_libraries(sgemm_v8_double_buffer PRIVATE CUDA::cudart ${CUDA_cublas_LIBRARY})
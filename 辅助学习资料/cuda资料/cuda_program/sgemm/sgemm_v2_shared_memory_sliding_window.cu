#include <cuda_runtime.h>

#include <cstdio>
#include <stdlib.h>

#define THREAD_PER_BLOCK 256 // 每个线程块线程个数

// 初始化矩阵
void random_matrix(int m, int n, float *a)
{
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
            a[i * n + j] = 2.0 * drand48() - 1.0;
    }
}

// CPU 矩阵乘法
void cpu_sgemm(float *A_ptr, float *B_ptr, float *C_ptr, int M, int N, int K)
{
    for(int m = 0; m < M; m++)
    {
        for(int n = 0; n < N; n++)
        {
            for(int k = 0; k < K; k++)
            {
                C_ptr[m * N + n] += A_ptr[m * K + k] * B_ptr[k * N + n];
            }
        }
    }
} 

// 检查结果
void compare_matrix(int m, int n, float *a, float *b)
{
    for(int i = 0; i < m; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(abs(a[i * n + j] - b[i * n + j]) > 1e-3)
            {
                printf("结果错误！\n");
                return;
            }
        }
    }

    printf("结果正确！\n");
}

// GPU 矩阵乘法
template <unsigned int BLOCK_SIZE>
__global__ void cuda_sgemm(float *A_ptr, float *B_ptr, float *C_ptr, int M, int N, int K)
{
    // 定义全局行索引、列索引
    int idx = threadIdx.x + blockDim.x * blockIdx.x;
    int idy = threadIdx.y + blockDim.y * blockIdx.y;

    float *A_ptr_start = A_ptr + blockDim.y * blockIdx.y * K;
    float *B_ptr_start = B_ptr + blockDim.x *blockIdx.x;

    // 定义共享内存变量
    __shared__ float a_shared[BLOCK_SIZE][BLOCK_SIZE];
    __shared__ float b_shared[BLOCK_SIZE][BLOCK_SIZE];

    float temp = 0.0f;
    for(int s = 0; s < K; s += BLOCK_SIZE)
    {
        a_shared[threadIdx.y][threadIdx.x] = A_ptr_start[threadIdx.y * K + threadIdx.x + s];
        b_shared[threadIdx.y][threadIdx.x] = B_ptr_start[(threadIdx.y + s) * N + threadIdx.x];

        __syncthreads();

        for(int k = 0; k < BLOCK_SIZE; k++)
        {
            temp += a_shared[threadIdx.y][k] * b_shared[k][threadIdx.x];
        }

        __syncthreads();
    }

    C_ptr[idy * N + idx] = temp;
}


int main()
{
    // 设置矩阵的维度
    int m = 512;
    int n = 512;
    int k = 512;
    const size_t mem_size_A = m * k * sizeof(float);
    const size_t mem_size_B = k * n * sizeof(float);
    const size_t mem_size_C = m * n * sizeof(float);

    // ************************** CPU 端计算 ***************************
    float *matrix_A_host = (float*)malloc(mem_size_A);
    float *matrix_B_host = (float*)malloc(mem_size_B);
    float *matrix_C_host_cpu_calc = (float*)malloc(mem_size_C);
    memset(matrix_C_host_cpu_calc, 0 , mem_size_C);

    random_matrix(m, k, matrix_A_host);
    random_matrix(k, n, matrix_B_host);

    cpu_sgemm(matrix_A_host, matrix_B_host, matrix_C_host_cpu_calc, m, n, k);

    // ************************** GPU 端计算 ***************************
    float *matrix_A_device;
    float *matrix_B_device;
    float *matrix_C_device;
    cudaMalloc(&matrix_A_device, mem_size_A);
    cudaMalloc(&matrix_B_device, mem_size_B);
    cudaMalloc(&matrix_C_device, mem_size_C);

    cudaMemcpy(matrix_A_device, matrix_A_host, mem_size_A, cudaMemcpyHostToDevice);
    cudaMemcpy(matrix_B_device, matrix_B_host, mem_size_B, cudaMemcpyHostToDevice);

    const int BLOCK = 16;
    dim3 block(BLOCK, BLOCK);
    dim3 grid((n + BLOCK - 1) / BLOCK, (m + BLOCK - 1) / BLOCK);
    cuda_sgemm<BLOCK><<<grid, block>>>(matrix_A_device, matrix_B_device, matrix_C_device, m, n, k);

    // ************************** 检查结果 ***************************
    float *matrix_C_host_gpu_calc = (float*)malloc(mem_size_C);
    cudaMemcpy(matrix_C_host_gpu_calc, matrix_C_device, mem_size_C, cudaMemcpyDeviceToHost);

    compare_matrix(m, n, matrix_C_host_cpu_calc, matrix_C_host_gpu_calc);

    free(matrix_A_host);
    free(matrix_B_host);
    free(matrix_C_host_cpu_calc);
    free(matrix_C_host_gpu_calc);
    cudaFree(matrix_A_device);
    cudaFree(matrix_B_device);
    cudaFree(matrix_C_device);

    return 0;
}
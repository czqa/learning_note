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


#define FETCH_FLOAT4(pointer) (reinterpret_cast<float4 *>(&pointer)[0])
// GPU 矩阵乘法
template <unsigned int M_MUM_PER_BLOCK, 
        unsigned int N_MUM_PER_BLOCK,
        unsigned int K_MUM_PER_BLOCK,
        unsigned int M_NUM_PER_THREAD,
        unsigned int N_NUM_PER_THREAD,
        unsigned int K_NUM_PER_THREAD>
__global__ void cuda_sgemm(float *A_ptr, float *B_ptr, float *C_ptr, int M, int N, int K)
{
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    float *A_ptr_start = A_ptr + blockIdx.y * M_MUM_PER_BLOCK * K; 
    float *B_ptr_start = B_ptr + blockIdx.x * N_MUM_PER_BLOCK;

    // 定义共享内存变量
    __shared__ float a_shared[M_MUM_PER_BLOCK][K_MUM_PER_BLOCK];
    __shared__ float b_shared[K_MUM_PER_BLOCK][N_MUM_PER_BLOCK];

    float a_reg[M_NUM_PER_THREAD] = {0.0f};
    float b_reg[N_NUM_PER_THREAD] = {0.0f};
    float temp[M_NUM_PER_THREAD][N_NUM_PER_THREAD] = {0.0f};

    for(int s = 0; s < K; s += K_MUM_PER_BLOCK)
    {
        for(int i = 0; i < M_NUM_PER_THREAD; i++)
        {
            FETCH_FLOAT4(a_shared[ty * M_NUM_PER_THREAD + i][tx * K_NUM_PER_THREAD]) = 
                FETCH_FLOAT4(A_ptr_start[(ty * M_NUM_PER_THREAD + i) * K + s + tx * K_NUM_PER_THREAD]);
        }
        for (int i = 0; i < K_NUM_PER_THREAD; i++)
        {
            FETCH_FLOAT4(b_shared[ty * K_NUM_PER_THREAD + i][tx * N_NUM_PER_THREAD]) = 
                FETCH_FLOAT4(B_ptr_start[(ty * K_NUM_PER_THREAD + i + s) * N + tx * N_NUM_PER_THREAD]);
        }
        
        __syncthreads();

        for (int k = 0; k < K_MUM_PER_BLOCK; k++)
        {
            a_reg[0] = a_shared[ty * M_NUM_PER_THREAD][k];
            a_reg[1] = a_shared[ty * M_NUM_PER_THREAD + 1][k];
            a_reg[2] = a_shared[ty * M_NUM_PER_THREAD + 2][k];
            a_reg[3] = a_shared[ty * M_NUM_PER_THREAD + 3][k];

            FETCH_FLOAT4(b_reg[0]) = FETCH_FLOAT4(b_shared[k][tx * N_NUM_PER_THREAD]);

            for(int i = 0; i < M_NUM_PER_THREAD; i++)
            {
                for(int j = 0; j < N_NUM_PER_THREAD; j++)
                {
                    temp[i][j] += a_reg[i] * b_reg[j];
                }
            }
        }

        __syncthreads();
    }

    float *C_ptr_start = C_ptr + N * blockIdx.y * M_MUM_PER_BLOCK + blockIdx.x * N_MUM_PER_BLOCK;

    for (int i = 0; i < M_NUM_PER_THREAD; i++)
    {
        FETCH_FLOAT4(C_ptr_start[(ty * M_NUM_PER_THREAD + i) * N + tx * N_NUM_PER_THREAD]) = FETCH_FLOAT4(temp[i]);
    }  
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


    const int M_MUM_PER_BLOCK = 64;
    const int N_MUM_PER_BLOCK = 64;
    const int K_MUM_PER_BLOCK = 64;
    const int M_NUM_PER_THREAD = 4;
    const int N_NUM_PER_THREAD = 4;
    const int K_NUM_PER_THREAD = 4;

    dim3 block(16, 16);
    dim3 grid(n / N_MUM_PER_BLOCK, m / M_MUM_PER_BLOCK);
    cuda_sgemm<M_MUM_PER_BLOCK, N_MUM_PER_BLOCK, K_MUM_PER_BLOCK, M_NUM_PER_THREAD, N_NUM_PER_THREAD, K_NUM_PER_THREAD>
            <<<grid, block>>>(matrix_A_device, matrix_B_device, matrix_C_device, m, n, k);

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
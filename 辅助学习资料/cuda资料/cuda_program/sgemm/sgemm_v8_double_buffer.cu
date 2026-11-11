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
        unsigned int N_NUM_PER_THREAD>
__global__ void cuda_sgemm(float *A_ptr, float *B_ptr, float *C_ptr, int M, int N, int K)
{
    // 线程索引
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    // 线程块中的线程总索引
    int tid = tx + ty * blockDim.x;

    // A_tile、B_tile 每一行需要的线程个数
    int A_tile_thread_per_row = K_MUM_PER_BLOCK / 4;    // 2
    int B_tile_thread_per_row = N_MUM_PER_BLOCK / 4;    // 32

    // 线程块中线程索引重排
    int A_tile_tid_x = tid % A_tile_thread_per_row;
    int A_tile_tid_y = tid / A_tile_thread_per_row;

    int B_tile_tid_x = tid % B_tile_thread_per_row; 
    int B_tile_tid_y = tid / B_tile_thread_per_row;

    // 定义共享内存变量
    __shared__ float a_shared[2][K_MUM_PER_BLOCK][M_MUM_PER_BLOCK];
    __shared__ float b_shared[2][K_MUM_PER_BLOCK][N_MUM_PER_BLOCK];

    // 定义寄存器变量
    float a_reg[M_NUM_PER_THREAD] = {0.0f};
    float b_reg[N_NUM_PER_THREAD] = {0.0f};
    float a_load_reg[4] = {0.0f};
    float temp[M_NUM_PER_THREAD][N_NUM_PER_THREAD] = {0.0f};

    // 重新定义数据起点
    float *A_ptr_start = A_ptr + blockIdx.y * M_MUM_PER_BLOCK * K; 
    float *B_ptr_start = B_ptr + blockIdx.x * N_MUM_PER_BLOCK;

    // 先搬运一次数据
    FETCH_FLOAT4(a_load_reg[0]) = FETCH_FLOAT4(A_ptr_start[A_tile_tid_y * K + A_tile_tid_x * 4]);
    a_shared[0][A_tile_tid_x * 4][A_tile_tid_y] = a_load_reg[0];
    a_shared[0][A_tile_tid_x * 4 + 1][A_tile_tid_y] = a_load_reg[1];
    a_shared[0][A_tile_tid_x * 4 + 2][A_tile_tid_y] = a_load_reg[2];
    a_shared[0][A_tile_tid_x * 4 + 3][A_tile_tid_y] = a_load_reg[3];
    FETCH_FLOAT4(b_shared[0][B_tile_tid_y][B_tile_tid_x * 4]) = FETCH_FLOAT4(B_ptr_start[B_tile_tid_y * N + B_tile_tid_x * 4]);

    __syncthreads();

    int write_stage_idx = 1;
    for(int s = K_MUM_PER_BLOCK; s < K; s += K_MUM_PER_BLOCK)
    {
        FETCH_FLOAT4(a_load_reg[0]) = FETCH_FLOAT4(A_ptr_start[A_tile_tid_y * K + A_tile_tid_x * 4 + s]);
        a_shared[write_stage_idx][A_tile_tid_x * 4][A_tile_tid_y] = a_load_reg[0];
        a_shared[write_stage_idx][A_tile_tid_x * 4 + 1][A_tile_tid_y] = a_load_reg[1];
        a_shared[write_stage_idx][A_tile_tid_x * 4 + 2][A_tile_tid_y] = a_load_reg[2];
        a_shared[write_stage_idx][A_tile_tid_x * 4 + 3][A_tile_tid_y] = a_load_reg[3];
        FETCH_FLOAT4(b_shared[write_stage_idx][B_tile_tid_y][B_tile_tid_x * 4]) = FETCH_FLOAT4(B_ptr_start[(B_tile_tid_y + s) * N + B_tile_tid_x * 4]);

        write_stage_idx = (write_stage_idx + 1) % 2;

        for (int k = 0; k < K_MUM_PER_BLOCK; k++)
        {
            FETCH_FLOAT4(a_reg[0]) = FETCH_FLOAT4(a_shared[write_stage_idx][k][ty * M_NUM_PER_THREAD]);
            FETCH_FLOAT4(a_reg[4]) = FETCH_FLOAT4(a_shared[write_stage_idx][k][ty * M_NUM_PER_THREAD + 4]);
            FETCH_FLOAT4(b_reg[0]) = FETCH_FLOAT4(b_shared[write_stage_idx][k][tx * N_NUM_PER_THREAD]);
            FETCH_FLOAT4(b_reg[4]) = FETCH_FLOAT4(b_shared[write_stage_idx][k][tx * N_NUM_PER_THREAD + 4]);

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

    // 计算最后一次数据
    write_stage_idx = (write_stage_idx + 1) % 2;
    for (int k = 0; k < K_MUM_PER_BLOCK; k++)
    {
        FETCH_FLOAT4(a_reg[0]) = FETCH_FLOAT4(a_shared[write_stage_idx][k][ty * M_NUM_PER_THREAD]);
        FETCH_FLOAT4(a_reg[4]) = FETCH_FLOAT4(a_shared[write_stage_idx][k][ty * M_NUM_PER_THREAD + 4]);
        FETCH_FLOAT4(b_reg[0]) = FETCH_FLOAT4(b_shared[write_stage_idx][k][tx * N_NUM_PER_THREAD]);
        FETCH_FLOAT4(b_reg[4]) = FETCH_FLOAT4(b_shared[write_stage_idx][k][tx * N_NUM_PER_THREAD + 4]);

        for (int i = 0; i < M_NUM_PER_THREAD; i++)
        {
            for (int j = 0; j < N_NUM_PER_THREAD; j++)
            {
                temp[i][j] += a_reg[i] * b_reg[j];
            }
        }
    }

    __syncthreads();

    float *C_ptr_start = C_ptr + N * blockIdx.y * M_MUM_PER_BLOCK + blockIdx.x * N_MUM_PER_BLOCK;

    for (int i = 0; i < M_NUM_PER_THREAD; i++)
    {
        FETCH_FLOAT4(C_ptr_start[(ty * M_NUM_PER_THREAD + i) * N + tx * N_NUM_PER_THREAD]) = FETCH_FLOAT4(temp[i][0]);
        FETCH_FLOAT4(C_ptr_start[(ty * M_NUM_PER_THREAD + i) * N + tx * N_NUM_PER_THREAD + 4]) = FETCH_FLOAT4(temp[i][4]);
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


    const int M_MUM_PER_BLOCK = 128;
    const int N_MUM_PER_BLOCK = 128;
    const int K_MUM_PER_BLOCK = 8;
    const int M_NUM_PER_THREAD = 8;
    const int N_NUM_PER_THREAD = 8;

    dim3 block(N_MUM_PER_BLOCK / N_NUM_PER_THREAD, M_MUM_PER_BLOCK / M_NUM_PER_THREAD);
    dim3 grid(n / N_MUM_PER_BLOCK, m / M_MUM_PER_BLOCK);
    cuda_sgemm<M_MUM_PER_BLOCK, N_MUM_PER_BLOCK, K_MUM_PER_BLOCK, M_NUM_PER_THREAD, N_NUM_PER_THREAD>
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
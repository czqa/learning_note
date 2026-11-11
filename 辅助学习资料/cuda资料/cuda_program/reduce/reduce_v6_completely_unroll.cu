#include <cuda_runtime.h>

#include <cstdio>
#include <stdlib.h>

#define THREAD_PER_BLOCK 256 // 每个线程块线程个数

// reduce 计算
__global__ void reduce(float *d_input, float *d_output)
{
    // 定义共享内存变量
    volatile __shared__ float shared[THREAD_PER_BLOCK];

    // 重新获取每个 block 的起始位置
    float *input_begin = d_input + blockDim.x * blockIdx.x * 2;
    
    // 将全局内存数据搬运到共享内存
    shared[threadIdx.x] = input_begin[threadIdx.x] + input_begin[threadIdx.x + blockDim.x];
    __syncthreads(); 

    if (threadIdx.x < 128)
    {
        shared[threadIdx.x] += shared[threadIdx.x + 128];
    }
    __syncthreads();

    if (threadIdx.x < 64)
    {
        shared[threadIdx.x] += shared[threadIdx.x + 64];
    }
    __syncthreads();

    if(threadIdx.x < 32)
    {
        shared[threadIdx.x] += shared[threadIdx.x + 32];
        shared[threadIdx.x] += shared[threadIdx.x + 16];
        shared[threadIdx.x] += shared[threadIdx.x + 8];
        shared[threadIdx.x] += shared[threadIdx.x + 4];
        shared[threadIdx.x] += shared[threadIdx.x + 2];
        shared[threadIdx.x] += shared[threadIdx.x + 1];
    }

    if(threadIdx.x == 0)
        d_output[blockIdx.x] = shared[0];

}

// 检查结果
bool check(float *out, float *res, int n)
{
    for(int i = 0; i < n; i++)
    {
        if(out[i] - res[i] > 1e-6)
            return false;
    }

    return true;
}

int main()
{
    // 元素总个数
    const int N = 32 * 1024 * 1024;

    // 线程块个数
    int block_num = N / THREAD_PER_BLOCK / 2;   

    // ************************* CPU 端计算 ***************************
    float *h_input = (float *)malloc(N * sizeof(float));
    float *h_output = (float *)malloc(block_num * sizeof(float));
    for(int i = 0; i < N; i++)
    {
        h_input[i] = 1;
    }

    for(int i = 0; i < block_num; i++)
    {
        for(int j = 0; j < 2 * THREAD_PER_BLOCK; j++)
        {
            h_output[i] += h_input[i * 2 * THREAD_PER_BLOCK + j];
        }
    }

    // ************************* GPU 端计算 ***************************
    float *d_input;
    cudaMalloc(&d_input, N * sizeof(float));
    float *d_output;
    cudaMalloc(&d_output, block_num * sizeof(float));

    cudaMemcpy(d_input, h_input, N * sizeof(float), cudaMemcpyHostToDevice);

    dim3 Grid(block_num);
    dim3 Block(THREAD_PER_BLOCK);

    reduce<<<Grid, Block>>>(d_input, d_output);

    // ************************* 对比结果 ***************************
    float *h_result = (float *)malloc(block_num * sizeof(float));
    cudaMemcpy(h_result, d_output, block_num * sizeof(float), cudaMemcpyDeviceToHost);

    if(check(h_output, h_result, block_num) == true)
        std::printf("结果正确！\n");
    else
    {
        std::printf("结果错误！\n");
    }
        
    // 释放内存
    free(h_input);
    free(h_output);
    free(h_result);
    cudaFree(d_input);
    cudaFree(d_output);

    return 0;
}

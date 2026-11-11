#include <cuda_runtime.h>

#include <cstdio>
#include <stdlib.h>

#define THREAD_PER_BLOCK 256 // 每个线程块线程个数

template <unsigned int NUM_PER_BLOCK>
// reduce 计算
__global__ void reduce(float *d_input, float *d_output)
{
    // 定义寄存器变量
    float sum = 0.0f;

    // 重新获取每个 block 的起始位置
    float *input_begin = d_input + NUM_PER_BLOCK * blockIdx.x;
    
    // 将全局内存数据搬运到寄存器变量
    for(int i = 0; i < NUM_PER_BLOCK / THREAD_PER_BLOCK; i++)
    {
        sum += input_begin[threadIdx.x + i * THREAD_PER_BLOCK];
    }

    // 一个warp的寄存器可以通信
    sum += __shfl_down_sync(0xffffffff, sum, 16);
    sum += __shfl_down_sync(0xffffffff, sum, 8);
    sum += __shfl_down_sync(0xffffffff, sum, 4);
    sum += __shfl_down_sync(0xffffffff, sum, 2);
    sum += __shfl_down_sync(0xffffffff, sum, 1);

    // 定义共享内存变量
    volatile __shared__ float warpLevelSums[32];
    const int laneId = threadIdx.x % 32;
    const int warpId = threadIdx.x / 32;

    // 将数据从寄存器搬到共享内存
    if(laneId == 0)
        warpLevelSums[warpId] = sum;

    __syncthreads();

    // 将数据从共享内存搬到寄存器
    if(warpId == 0)
    {
        sum = (laneId < blockDim.x / 32) ? warpLevelSums[laneId] : 0.0f;

        sum += __shfl_down_sync(0xffffffff, sum, 16);
        sum += __shfl_down_sync(0xffffffff, sum, 8);
        sum += __shfl_down_sync(0xffffffff, sum, 4);
        sum += __shfl_down_sync(0xffffffff, sum, 2);
        sum += __shfl_down_sync(0xffffffff, sum, 1);
    }

    if(threadIdx.x == 0)
        d_output[blockIdx.x] = sum;

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
    const int block_num = 1024;
    const int num_per_block = N / block_num;   

    // ************************* CPU 端计算 ***************************
    float *h_input = (float *)malloc(N * sizeof(float));
    float *h_output = (float *)malloc(block_num * sizeof(float));
    for(int i = 0; i < N; i++)
    {
        h_input[i] = 1;
    }

    for(int i = 0; i < block_num; i++)
    {
        for(int j = 0; j < num_per_block; j++)
        {
            h_output[i] += h_input[i * num_per_block + j];
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

    reduce<num_per_block><<<Grid, Block>>>(d_input, d_output);

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
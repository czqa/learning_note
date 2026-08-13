#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100
#define RADIX 10 // 十进制：桶的数量为10（0~9）

// 获取某个数在某一位上的数字
int getDigit(int num, int pos)
{
    int radix = 1;
    for (int i = 1; i < pos; i++)
    {
        radix *= 10;
    }
    return (num / radix) % 10;
}

// 获取数组中最大数的位数
int getMaxDigit(int *data, int len)
{
    int max = data[0];
    for (int i = 1; i < len; i++)
    {
        if (data[i] > max)
        {
            max = data[i];
        }
    }

    int digit = 0;
    while (max != 0)
    {
        digit++;
        max /= 10;
    }

    return digit;
}

// 基数排序
void radixSort(int *data, int len)
{
    int maxDigit = getMaxDigit(data, len); // 获取最大位数，决定排序轮数
    int bucket[RADIX][MAX];                // 定义10个桶，每个桶最多装 MAX 个元素
    int count[RADIX];                      // 每个桶中的元素数量

    for (int pos = 1; pos <= maxDigit; pos++) // 从个位开始，依次对每一位进行排序
    {
        memset(count, 0, sizeof(count)); // 清空计数器，准备新一轮

        // 分配阶段：将每个元素放入对应的桶中
        for (int i = 0; i < len; i++)
        {
            int digit = getDigit(data[i], pos);           // 获取当前位置的数字
            bucket[digit][count[digit]++] = data[i];      // 放入对应桶中
        }

        // 收集阶段：按桶的顺序重新收集数据
        int index = 0;
        for (int i = 0; i < RADIX; i++) // 依次从0到9号桶取出数据
        {
            for (int j = 0; j < count[i]; j++)
            {
                data[index++] = bucket[i][j];
            }
        }
    }
}

// 主函数测试
int main(int argc, char const *argv[])
{
    int data[] = {170, 201, 321, 95, 843, 996, 57, 8, 241, 30};
    int len = sizeof(data) / sizeof(data[0]);

    radixSort(data, len);

    for (int i = 0; i < len; i++)
    {
        printf("%d ", data[i]);
    }

    return 0;
}
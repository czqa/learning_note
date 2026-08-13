#include <stdio.h>

// 希尔排序函数
void shellSort(int* data, int len)
{
	int temp;
	// 初始化步长为数组长度的一半
	int step = len / 2;
	while(step >= 1)  // 当步长不小于1时继续排序
	{
		// 对每个分组执行插入排序
		for (int i = step; i < len; i++)
		{
			// 当前元素小于其步长距离前的元素 → 需要插入排序
			if (data[i] < data[i-step])
			{
				temp = data[i];  // 暂存当前值
				int j = i - step;

				// 向前寻找合适插入位置，并逐步移动大于 temp 的元素
				while(j >= 0 && temp < data[j])
				{
					data[j + step] = data[j];  // 将较大值后移
					j = j - step;
				}
				// 插入 temp 到正确位置
				data[j + step] = temp;
			}
		}
		// 缩小步长，通常除以2
		step = step / 2;
	}
}

int main(int argc, char const *argv[])
{
	// 待排序数组
	int data[] = {47, 35, 60, 95, 77, 15, 28, 52};
	int len = sizeof(data) / sizeof(data[0]);  // 计算数组长度

	shellSort(data, len);  // 调用希尔排序

	// 输出排序结果
	for(int i = 0; i < len; i++)
	{
		printf("%d ", data[i]);
	}
	return 0;
}
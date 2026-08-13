#include <stdio.h>
// 交换数组中下标为 m 和 n 的两个元素的值
void swap(int* data, int m, int n)
{
	int temp = data[m];
	data[m] = data[n];
	data[n] = temp;
}
// 实现冒泡排序：将数组 data 中的元素按升序排列
void bubbleSort(int* data, int len)
{
	// 外层循环控制“趟数”，共需 len 次比较（严格来说是 len - 1 次）
	for (int i = 0; i < len; i++)
	{
		// 内层循环控制每趟中相邻元素的比较和交换
		// 每次比较到 len - i - 1，因为右侧已经排好序
		for(int j = 0; j < len - i - 1; j++)
		{
			// 如果前一个元素比后一个大，则交换它们
			if (data[j] > data[j + 1])
			{
				swap(data, j, j+1); // 调用 swap 函数进行交换
			}
		}
	}
}
int main(int argc, char const *argv[])
{
	// 定义待排序的整数数组
	int data[] = {47, 35, 60, 95, 77, 15, 28};

	// 计算数组中元素的个数
	int len = sizeof(data) / sizeof(data[0]);

	// 调用冒泡排序函数进行排序
	bubbleSort(data, len);

	// 输出排序后的数组
	for(int i = 0; i < len; i++)
	{
		printf("%d ", data[i]);
	}

	return 0;
}
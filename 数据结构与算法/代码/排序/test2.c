#include <stdio.h>

// 插入排序函数
void insertSort(int* data, int len)
{
	int key; // 用于暂存当前待插入的元素
	for (int i = 1; i < len; i++) // 从第二个元素开始逐个插入
	{
		key = data[i];        // 将当前元素保存为 key
		int j = i - 1;         // 从已排序序列的最后一个元素开始向前比较

		// 向前查找插入位置，移动比 key 大的元素
		while(j >= 0 && data[j] > key)
		{
			data[j+1] = data[j]; // 将比 key 大的元素后移
			j--;                 // 向前继续比较
		}

		// 找到插入位置后，将 key 插入
		data[j+1] = key;
	}
}

int main(int argc, char const *argv[])
{
	//47，35，60， 95， 77
	int data[] = {47, 35, 60, 95, 77, 15, 28}; // 初始化数组
	int len = sizeof(data) / sizeof(data[0]); // 计算数组长度

	insertSort(data, len); // 对数组进行插入排序

	// 输出排序后的结果
	for(int i = 0; i < len; i++)
	{
		printf("%d ", data[i]);
	}
	return 0;
}
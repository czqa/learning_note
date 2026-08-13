#include <stdio.h>

// 折半插入排序
void binaryInsertSort(int* data, int len)
{
	int low, high, mid;
	int key;

	// 从第二个元素开始遍历，将其插入到前面已经排好序的子序列中
	for (int i = 1; i < len; i++)
	{
		key = data[i];         // 记录当前待插入的值
		low = 0;               // 设置当前有序序列的最小下标
		high = i - 1;          // 设置当前有序序列的最大下标

		// 使用折半查找法找出 key 应该插入的位置（即 low 最终的位置）
		while(low <= high)
		{
			mid = (low + high) / 2;
			if (data[mid] > key)
			{
				high = mid - 1; // 插入点在左半边
			}
			else
			{
				low = mid + 1;  // 插入点在右半边
			}
		}

		// 从后往前依次将大于 key 的元素后移，为 key 腾出位置
		int j = i - 1;
		while(j >= low)
		{
			data[j+1] = data[j];
			j--;
		}

		// 插入 key 到正确的位置
		data[low] = key;
	}
}

int main(int argc, char const *argv[])
{
	int data[] = {47, 35, 60, 95, 77, 15, 28};
	int len = sizeof(data) / sizeof(data[0]);

	// 调用折半插入排序
	binaryInsertSort(data, len);

	// 输出排好序的结果
	for(int i = 0; i < len; i++)
	{
		printf("%d ", data[i]);
	}
	return 0;
}
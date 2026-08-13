#include <stdio.h>

// 交换数组中两个元素的值
void swap(int* data, int m, int n)
{
	int temp = data[m];
	data[m] = data[n];
	data[n] = temp;
}

// 向下调整堆（从 root 开始，len 为当前堆的大小）
void adjust(int* data, int root, int len)
{
	int child;
	int temp = data[root];  // 保存当前根节点的值
	child = 2 * root;        // 左孩子下标

	// 只要存在孩子节点
	while(child <= len)
	{
		// 如果有右孩子，且右孩子比左孩子大，则选右孩子
		if (child < len && data[child] < data[child+1])
		{
			child++;  // 右孩子下标
		}
		// 如果当前节点比较大，不需要调整
		if (temp > data[child])
		{
			break;
		}
		// 将子节点的值上移
		data[child/2] = data[child];
		// 继续向下调整
		child = child * 2;
	}
	// 把原 root 的值放到最终位置
	data[child/2] = temp;
}

// 堆排序主函数
void heapSort(int* data, int len)
{
	// 1. 建堆过程：从最后一个非叶子节点开始向上调整
	for (int i = len / 2; i > 0; i--)
	{
		adjust(data, i, len);
	}

	// 2. 排序过程：将堆顶最大值依次交换到数组末尾，并调整剩余部分为大根堆
	for(int i = len; i > 1; i--)
	{
		swap(data, 1, i);         // 将当前最大值（堆顶）移到末尾
		adjust(data, 1, i-1);     // 对剩余部分重新建堆
	}
}

int main(int argc, char const *argv[])
{
	// 注意：data[0]未使用，占位是为了从 data[1] 开始建堆
	int data[] = {-1, 47, 35, 60, 95, 77, 15, 28};
	int len = sizeof(data) / sizeof(data[0]);

	// 调用堆排序函数（不包含占位的 data[0]）
	heapSort(data, len-1);

	// 输出排序结果
	for(int i = 1; i < len; i++)
	{
		printf("%d ", data[i]);
	}
	return 0;
}
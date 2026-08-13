#include <stdio.h>
//折半查找函数
int binary_search(int* data, int len, int value)
{
	//data:查找表 n:表中元素个数 value:要查找的内容
	int low = 0; //查找表的起始下标为low
	int high = len - 1; //假设查找表的结束下标为high
	int mid;
	while(low <= high)
	{
		mid = (low + high) / 2; //计算中间下标：mid = (low + high) / 2
		if (value > data[mid])
		{
			low = mid + 1; //如果关键字大于a[mid]，则在右半部分继续查找，令low = mid + 1
		}
		else if(value < data[mid])
		{
			high = mid - 1; //如果关键字小于data[mid]，则在左半部分继续查找，令high = mid - 1
		}
		else
		{
			return mid; //如果关键字等于data[mid]，查找成功，返回下标
		}
	}
	return -1;
}

int main(int argc, char const *argv[])
{
	int arr[] = {2, 13, 22, 38, 46, 58, 66, 77, 85, 98}; //查找表
	int len = sizeof(arr) / sizeof(arr[0]); //数组长度
	int pos = binary_search(arr, len, 66); //调用函数查找66
	printf("%d\n", pos);
	return 0;
}
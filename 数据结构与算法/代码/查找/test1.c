#include <stdio.h>
//顺序查找函数
int search(int* data, int len, int value)
{
	//data:查找表 n:表中元素个数 value:要查找的内容
	for (int i = 0; i < len; i++)
	{
		//逐个比对是表中元素与要查找数据是否一致
		if (data[i] == value)
		{
			return i;//返回找到的元素下标
		}
	}
	return -1;//没有查到返回-1
}

int main(int argc, char const *argv[])
{
	int arr[] = {15, 54, 76, 6, 9, 45, 12}; //查找表
	int len = sizeof(arr) / sizeof(arr[0]); //数组长度
	int pos = search(arr, len, 45); //调用函数查找45
	printf("%d\n", pos);
	return 0;
}
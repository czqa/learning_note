#include <stdio.h>
#include <stdlib.h>
#define Max 0x7fffffff
int find(int data[], int n)
{
	int i;
	int *record;

	record = (int*)malloc(sizeof(int) * Max);

	for (i = 0; i < Max; i++)
	{
		record[i] = 0;
	}
	

	for (i = 0; i < n; i++)
	{
		if (data[i] > 0)
		{
			record[data[i]] = 1;
		}
	}

	for (i = 1; i < Max; i++)
	{
		if (record[i] == 0)
		{
			break;
		}
	}
	free(record);
	return i;
}

int main(int argc, char const *argv[])
{
	int data[] = {-5, 3, 2, 3};
	//int data[] = {1, 2, 3};
	int len = sizeof(data) / sizeof(data[0]);
	int result = find(data, len);
	printf("%d\n", result);
	return 0;
}
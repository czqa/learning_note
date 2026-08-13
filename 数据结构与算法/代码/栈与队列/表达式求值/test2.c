#include <stdio.h>

typedef enum
{
	mon=1, tue, wed, thu, fri, sat, sun
} weekday;

typedef enum
{
	false, true
}bool;


//bool boolean

int main(int argc, char const *argv[])
{
	weekday a;
	a = mon;
	printf("%d\n", a);

	bool flag;
	flag = true;
	if (flag)
	{
		printf("真的");
	}
	return 0;
}
#include <stdio.h>
#include <stdlib.h>
//fun函数中的参数temp 为二级指针，调用时应传入一个指针变量的地址（或者叫指针变量的指针）
void fun(int** temp)
{
	*temp = (int*)malloc(sizeof(int)); //解引用一层指针，并让其指向堆内存中新开辟的4个字节空间
	**temp = 100; //解引用两层指针，对上一行代码中4个字节的空间赋值100
}
 
int main(int argc, char const *argv[])
{
	int *p = NULL; //声明指针变量p，p是一个空指针，它没有指向任何内存地址
	fun(&p); //调用函数fun，传入指针变量p的地址 
	printf("%d\n", *p);
	free(p);
	return 0;
}

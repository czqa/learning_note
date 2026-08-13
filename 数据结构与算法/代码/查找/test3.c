#include <stdio.h>
#include <stdlib.h>
typedef int ElemType; // 定义二叉树节点数据类型
// 定义二叉树节点结构体
typedef struct TreeNode
{
	ElemType data;               // 节点存储的数据（关键字）
	struct TreeNode *lchild;    // 左子节点
	struct TreeNode *rchild;    // 右子节点
} TreeNode;

typedef TreeNode* BiTree;      // 定义二叉树指针类型

// 用数组模拟先序创建一棵二叉树（-1 表示空节点）
int treeArr[] = {70, 55, 49, 30, -1, 39, -1, -1, 53, -1, -1, -1, 80, 75, -1, -1, 98, 95, -1, -1, -1};
int idx = 0;

// 递归先序方式构建二叉树（含空节点）
void createTree(BiTree *T)
{
	ElemType num;
	num = treeArr[idx++];  // 取当前索引位置的值

	if (num == -1)
	{
		// -1 代表空指针
		*T = NULL;
	}
	else
	{
		// 创建新节点并递归构建左右子树
		*T = (BiTree)malloc(sizeof(TreeNode));
		(*T)->data = num;
		createTree(&(*T)->lchild);  // 构建左子树
		createTree(&(*T)->rchild);  // 构建右子树
	}
}

// 先序遍历打印二叉树结点值（调试用）
void preOrder(BiTree T)
{
	if (T == NULL)
	{
		return;
	}

	printf("%d ", T->data);     // 访问当前节点
	preOrder(T->lchild);        // 遍历左子树
	preOrder(T->rchild);        // 遍历右子树
}

/*
 * 查找二叉排序树中是否包含指定值 value
 * 参数 T: 当前子树根节点
 * 参数 value: 要查找的值
 * 参数 parent: 当前节点的父节点
 * 参数 pos: 指针变量，用于返回找到的节点
 * 返回值：1 表示找到；0 表示未找到
 */
int search_bst(BiTree T, int value, BiTree parent, BiTree *pos)
{
	if (T == NULL)
	{
		*pos = parent; // 递归到空节点，查找失败，将父节点返回（后面内容可用于插入）
		return 0;
	}

	if (T->data == value)
	{
		*pos = T; // 找到目标值
		return 1;
	}

	if (T->data > value)
	{
		return search_bst(T->lchild, value, T, pos); // 要查找的值比当前节点小，往左子树找
	}
	else
	{
		return search_bst(T->rchild, value, T, pos); // 要查找的值比当前节点大，往右子树找
	}
}

int main(int argc, char const *argv[])
{
	BiTree T;
	createTree(&T);   // 创建二叉排序树

	BiTree searchT;
	search_bst(T, 53, NULL, &searchT);  // 查找值为 53 的节点

	preOrder(T);      // 输出树结构（调试用）
	printf("\n");

	printf("%d\n", searchT->data);  // 输出查找到的节点值
	return 0;
}
#include <stdio.h>
#include <stdlib.h>

typedef int ElemType;

// 二叉树结点结构体定义
typedef struct TreeNode
{
	ElemType data;               // 数据域
	struct TreeNode *lchild;     // 左子树指针
	struct TreeNode *rchild;     // 右子树指针
}TreeNode;

typedef TreeNode* BiTree;

// 先序遍历，用于查看当前树结构
void preOrder(BiTree T)
{
	if (T == NULL)
	{
		return;
	}

	printf("%d ", T->data);       // 先访问根结点
	preOrder(T->lchild);          // 遍历左子树
	preOrder(T->rchild);          // 遍历右子树
}

// 查找元素是否存在于二叉排序树中
// 如果找到，pos 指向该结点，返回 1
// 如果未找到，pos 指向应插入位置的父节点，返回 0
int search_bst(BiTree T, int value, BiTree parent, BiTree *pos)
{
	if (T == NULL)
	{
		*pos = parent;             // 最终未找到，pos 指向待插入位置的父结点
		return 0;
	}

	if (T->data == value)
	{
		*pos = T;                 // 找到了该值
		return 1;
	}

	if (T->data > value)
	{
		// 当前值大于目标值，向左子树查找
		return search_bst(T->lchild, value, T, pos);
	}
	else
	{
		// 当前值小于目标值，向右子树查找
		return search_bst(T->rchild, value, T, pos);
	}
}

// 向二叉排序树中插入一个值
// 如果值已存在，插入失败，返回 0；否则插入成功，返回 1
int insert_bst(BiTree *T, int value)
{
	BiTree parent, pos;          // parent 用于辅助 search_bst，pos 表示插入位置或已存在的位置
	BiTree curr;

	// 查找插入位置
	int status = search_bst(*T, value, NULL, &pos);
	if(status == 0)
	{
		// 如果未找到，创建新结点
		curr = (BiTree)malloc(sizeof(TreeNode));
		curr->data = value;
		curr->lchild = NULL;
		curr->rchild = NULL;
		// 如果树为空，新结点作为根结点
		if (pos == NULL)
		{
			*T = curr;
		}
		// 插入为左子结点
		else if (value < pos->data)
		{
			pos->lchild = curr;
		}
		// 插入为右子结点
		else
		{
			pos->rchild = curr;
		}
		return 1;
	}
	else
	{
		// 如果已存在，不插入
		return 0;
	}
}

int main(int argc, char const *argv[])
{
	int i = 0;
	BiTree T = NULL;

	// 准备插入的初始序列
	int treeArr[] = {70, 55, 49, 30, 39, 53, 80, 75, 98, 95};

	// 批量插入
	for (int i = 0; i < 10; i++)
	{
		insert_bst(&T, treeArr[i]);
	}

	preOrder(T); // 查看当前树结构
	printf("\n");

	insert_bst(&T, 99); // 插入一个新值 99

	preOrder(T); // 再次查看树结构
	printf("\n");

	return 0;
}
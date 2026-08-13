#include <stdio.h>
#include <stdlib.h>

// 定义数据类型
typedef int ElemType;

// 定义树的节点结构
typedef struct TreeNode
{
	ElemType data;
	struct TreeNode *lchild;  // 左子树
	struct TreeNode *rchild;  // 右子树
}TreeNode;

typedef TreeNode* BiTree;

// 前序遍历：递归地访问根、左、右
void preOrder(BiTree T)
{
	if (T == NULL)
	{
		return;
	}

	printf("%d ", T->data);
	preOrder(T->lchild);
	preOrder(T->rchild);
}

// 查找指定值，若找到将pos指向该节点，若未找到则pos指向待插入位置的双亲节点
int search_bst(BiTree T, int value, BiTree parent, BiTree *pos)
{
	if (T == NULL)
	{
		*pos = parent;  // 没找到，返回最后访问的节点
		return 0;
	}

	if (T->data == value)
	{
		*pos = T;  // 找到，pos指向该节点
		return 1;
	}

	if (T->data > value)
	{
		return search_bst(T->lchild, value, T, pos);
	}
	else
	{
		return search_bst(T->rchild, value, T, pos);
	}
}

// 插入操作，若值不存在，则插入到合适位置
int insert_bst(BiTree *T, int value)
{
	BiTree parent, pos;
	BiTree curr;

	int status = search_bst(*T, value, NULL, &pos);
	if(status == 0)
	{
		// 创建新节点
		curr = (BiTree)malloc(sizeof(TreeNode));
		curr->data = value;
		curr->lchild = NULL;
		curr->rchild = NULL;

		// 如果是根节点
		if (pos == NULL)
		{
			*T = curr;
		}
		else if (value < pos->data)
		{
			pos->lchild = curr;
		}
		else
		{
			pos->rchild = curr;
		}
		return 1;
	}
	else
	{
		// 已存在该值
		return 0;
	}
}

// 真正执行删除的操作（分三种情况）
int delete(BiTree *d)
{
	BiTree temp, record;

	if((*d)->rchild == NULL)
	{
		// 情况1：只有左子树或无子节点
		temp = *d;
		*d = (*d)->lchild;
		free(temp);
	}
	else if ((*d)->lchild == NULL)
	{
		// 情况2：只有右子树
		temp = *d;
		*d = (*d)->rchild;
		free(temp);
	}
	else
	{
		// 情况3：左右子树都有，找左子树中最大的节点替换
		temp = *d;
		record = (*d)->lchild;
		while(record->rchild != NULL)
		{
			temp = record;
			record = record->rchild;
		}

		// 替换当前值
		(*d)->data = record->data;

		// 删除替代节点
		if (temp != *d)
		{
			temp->rchild = record->lchild;
		}
		else
		{
			temp->lchild = record->lchild;
		}
		free(record);
	}
	return 1;
}

// 递归查找并删除目标节点
int delete_bst(BiTree *T, int value)
{
	if (*T == NULL)
	{
		printf("not found!\n");
		return 0;
	}
	else
	{
		if ((*T)->data == value)
		{
			return delete(T);
		}
		else if ((*T)->data > value)
		{
			return delete_bst(&(*T)->lchild, value);
		}
		else
		{
			return delete_bst(&(*T)->rchild, value);
		}
	}
}

int main(int argc, char const *argv[])
{
	int i = 0;
	BiTree T = NULL;
	// 构建初始二叉排序树
	int treeArr[] = {70, 55, 49, 30, 39, 53, 80, 75, 98, 95};
	for (int i = 0; i < 10; i++)
	{
		insert_bst(&T, treeArr[i]);
	}

	preOrder(T);  // 输出原始树结构
	printf("\n");

	delete_bst(&T, 49);  // 删除指定节点

	preOrder(T);  // 再次输出
	printf("\n");
	return 0;
}
//非递归后序遍历
#include <stdio.h>
#include <stdlib.h>
#define MAXSIZE 100
typedef char ElemType;

// 定义二叉树结构体
typedef struct TreeNode {
    ElemType data;  // 当前节点存储的数据（字符型）
    struct TreeNode *lchild;  // 指向左孩子
    struct TreeNode *rchild;  // 指向右孩子
} TreeNode;
typedef TreeNode* BiTree;
// 表示二叉树中的内容，#表示空
char str[] = "ABDH#K###E##CFI###G#J##"; 
int idx = 0;

// 以前序遍历的顺序构建二叉树：每次读取一个字符，递归构建当前节点的左子树和右子树
void createTree(BiTree *T) 
{
    ElemType ch = str[idx++];  // 读取下一个字符
    if (ch == '#') 
    {
        *T = NULL; // 如果是#，则为空节点
    } 
    else 
    {
        *T = (BiTree)malloc(sizeof(TreeNode));  // 创建新节点
        (*T)->data = ch;  // 设置数据域
        createTree(&(*T)->lchild); // 递归构建左子树
        createTree(&(*T)->rchild); // 递归构建右子树
    }
}

// 栈结构体定义（用于遍历时手动维护）
typedef struct {
    BiTree data[MAXSIZE];
    int top;
} Stack;

// 初始化栈
void initStack(Stack *s) {
    s->top = -1;
}

// 判断栈是否为空
int isEmpty(Stack *s) {
    return s->top == -1;
}

// 入栈
int push(Stack *s, BiTree p) {
    if (s->top >= MAXSIZE - 1) return 0;
    s->data[++(s->top)] = p;
    return 1;
}

// 出栈
int pop(Stack *s, BiTree *p) {
    if (isEmpty(s)) return 0;
    *p = s->data[(s->top)--];
    return 1;
}

// 后序遍历使用两个栈：s1记录正序，s2反转输出
void postOrderNonRecursive(BiTree T) {
    if (T == NULL) return;

    Stack s1, s2;
    initStack(&s1);
    initStack(&s2);
    push(&s1, T);

    BiTree p;

    // 类似“根 → 右 → 左”的顺序压入s2
    while (!isEmpty(&s1)) {
        pop(&s1, &p);
        push(&s2, p);  // 放入辅助栈

        // 注意：先压左，再压右，s2中得到“左 → 右 → 根”的反序
        if (p->lchild) {
            push(&s1, p->lchild);
        }
        if (p->rchild) {
            push(&s1, p->rchild);
        }
    }
    // 依次弹出s2中的元素，得到后序遍历
    while (!isEmpty(&s2)) {
        pop(&s2, &p);
        printf("%c ", p->data);
    }
}

int main(int argc, char const *argv[])
{
    //创建二叉树
	BiTree T;
	createTree(&T);
    //后序遍历
    postOrderNonRecursive(T);
	return 0;
}
//非递归前序遍历
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

// 非递归实现前序遍历
void preOrderNonRecursive(BiTree T) {
    if (T == NULL) return;

    Stack s;
    initStack(&s);
    push(&s, T);  // 先将根节点压入栈

    while (!isEmpty(&s)) {
        BiTree p;
        pop(&s, &p);               // 弹出当前节点
        printf("%c ", p->data);    // 访问该节点

        // 注意：先压右子，后压左子，确保左子先出
        if (p->rchild) {
            push(&s, p->rchild);
        }
        if (p->lchild) {
            push(&s, p->lchild);
        }
    }
}

int main(int argc, char const *argv[])
{
    //创建二叉树
    BiTree T;
    createTree(&T);
    //前序遍历
    preOrderNonRecursive(T);
    return 0;
}
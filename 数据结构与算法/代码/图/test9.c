#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100  // 最大顶点数

// 边结点结构（邻接多重表）
typedef struct EBox {
    int ivex, jvex;          // 该边所关联的两个顶点的编号
    struct EBox *ilink;      // 指向在 ivex 的边链表中的下一个边结点
    struct EBox *jlink;      // 指向在 jvex 的边链表中的下一个边结点
} EBox;

// 顶点结构
typedef struct VexBox {
    char data;               // 顶点信息，例如 '0', '1', ...
    EBox *firstedge;         // 指向第一个边结点的指针
} VexBox;

// 图结构体
typedef struct {
    VexBox adjmulist[MAXVEX];  // 顶点数组
    int numVertexes;           // 顶点数
    int numEdges;              // 边数
} AMLGraph;

// 构建邻接多重表图（图6-21）
void createGraph(AMLGraph *G) {
    G->numVertexes = 4;
    G->numEdges = 5;

    // 初始化所有顶点
    for (int i = 0; i < G->numVertexes; i++) {
        G->adjmulist[i].data = '0' + i;
        G->adjmulist[i].firstedge = NULL;
    }

    // 图中边的定义（无向边，格式为 顶点对）
    int edges[5][2] = {{0, 1},{1, 2},{2, 3},{3, 0},{0, 2}};

    // 插入每条边
    for (int k = 0; k < G->numEdges; k++) {
        int i = edges[k][0];
        int j = edges[k][1];

        EBox *p = (EBox *)malloc(sizeof(EBox));
        p->ivex = i;
        p->jvex = j;

        // 插入到 ivex 顶点的边链表
        p->ilink = G->adjmulist[i].firstedge;
        p->jlink = G->adjmulist[j].firstedge;

        // 更新 ivex 和 jvex 的边链表头指针
        G->adjmulist[i].firstedge = p;
        G->adjmulist[j].firstedge = p;
    }
}

// 打印邻接多重表结构
void printGraph(AMLGraph G) {
    printf("邻接多重表结构如下：\n");

    for (int i = 0; i < G.numVertexes; i++) {
        printf("顶点 V%c 的边: ", G.adjmulist[i].data);
        EBox *p = G.adjmulist[i].firstedge;

        while (p) {
            int neighbor = (p->ivex == i) ? p->jvex : p->ivex;
            printf("%d ", neighbor);
            p = (p->ivex == i) ? p->ilink : p->jlink;
        }
        printf("\n");
    }
}

// 主函数
int main() {
    AMLGraph G;
    createGraph(&G);   // 构建图
    printGraph(G);     // 输出图结构
    return 0;
}

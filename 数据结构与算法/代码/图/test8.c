#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100  // 最大顶点数

// 边结点结构（十字链表专用）
typedef struct ArcBox {
    int tailvex;                // 边的起点
    int headvex;                // 边的终点
    struct ArcBox *nextout;    // 指向同一起点的下一条出边
    struct ArcBox *nextin;     // 指向同一终点的下一条入边
} ArcBox;

// 顶点结构
typedef struct VexNode {
    char data;                 // 顶点信息（此处为字符 '0'、'1' 等）
    ArcBox *firstin;           // 指向入边链表的头结点
    ArcBox *firstout;          // 指向出边链表的头结点
} VexNode, OLGraph[MAXVEX];

// 图结构体（十字链表）
typedef struct {
    OLGraph xlist;             // 顶点数组
    int numVertexes;           // 顶点数量
    int numEdges;              // 边数量
} OrthogonalGraph;

// 创建图（构造图 6-19 的结构）
void createGraph(OrthogonalGraph *G) {
    G->numVertexes = 4;  // 顶点数 V0 ~ V3
    G->numEdges = 5;     // 边数

    // 初始化所有顶点
    for (int i = 0; i < G->numVertexes; i++) {
        G->xlist[i].data = '0' + i;   // 顶点编号设为字符 '0', '1', ...
        G->xlist[i].firstin = NULL;   // 入边链表初始化为空
        G->xlist[i].firstout = NULL;  // 出边链表初始化为空
    }

    // 定义图中所有边（格式为 起点 → 终点）
    int edges[5][2] = {
        {0, 3},
        {1, 0},
        {1, 2},
        {2, 0},
        {2, 1}
    };

    // 插入每条边
    for (int i = 0; i < G->numEdges; i++) {
        int tail = edges[i][0];
        int head = edges[i][1];

        // 创建边结点
        ArcBox *p = (ArcBox *)malloc(sizeof(ArcBox));
        p->tailvex = tail;
        p->headvex = head;

        // 插入到起点的出边链表（头插法）
        p->nextout = G->xlist[tail].firstout;
        G->xlist[tail].firstout = p;

        // 插入到终点的入边链表（头插法）
        p->nextin = G->xlist[head].firstin;
        G->xlist[head].firstin = p;
    }
}

// 打印图结构（展示每个顶点的出边与入边）
void printGraph(OrthogonalGraph G) {
    printf("十字链表图结构如下：\n");

    for (int i = 0; i < G.numVertexes; i++) {
        printf("顶点 V%c 的出边: ", G.xlist[i].data);
        ArcBox *p = G.xlist[i].firstout;
        while (p) {
            printf("(%d→%d) ", p->tailvex, p->headvex);
            p = p->nextout;
        }
        printf("\n");

        printf("顶点 V%c 的入边: ", G.xlist[i].data);
        p = G.xlist[i].firstin;
        while (p) {
            printf("(%d→%d) ", p->tailvex, p->headvex);
            p = p->nextin;
        }
        printf("\n\n");
    }
}

// 主函数
int main() {
    OrthogonalGraph G;
    createGraph(&G);     // 构造图
    printGraph(G);       // 打印图结构
    return 0;
}

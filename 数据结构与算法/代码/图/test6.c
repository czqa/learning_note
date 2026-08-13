#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100

// 边表结点（用于逆邻接表）
typedef struct EdgeNode {
    int from;                   // 谁指向我（前驱）
    struct EdgeNode *next;
} EdgeNode;

// 顶点表结构体
typedef struct VertexNode {
    char data;                 // 顶点信息
    EdgeNode *firstin;        // 指向前驱链表头
} VertexNode, InAdjList[MAXVEX];

// 逆邻接表结构体
typedef struct {
    InAdjList inList;          // 逆邻接表数组
    int numVertexes;
    int numEdges;
} InverseGraph;

// 构建逆邻接表（基于图 6-16）
void createInverseGraph(InverseGraph *G) {
    G->numVertexes = 4;
    G->numEdges = 5;

    // 初始化顶点名
    for (int i = 0; i < G->numVertexes; i++) {
        G->inList[i].data = '0' + i;
        G->inList[i].firstin = NULL;
    }

    // 原图的边集：{起点, 终点}
    int edges[5][2] = {
        {0, 3}, {1, 0}, {1, 2},
        {2, 0}, {2, 1}
    };

    for (int i = 0; i < G->numEdges; i++) {
        int from = edges[i][0];
        int to = edges[i][1];

        // 插入逆邻接链表：to ← from
        EdgeNode *e = (EdgeNode *)malloc(sizeof(EdgeNode));
        e->from = from;
        e->next = G->inList[to].firstin;
        G->inList[to].firstin = e;
    }
}

// 打印逆邻接表
void printInverseGraph(InverseGraph G) {
    printf("图的逆邻接表如下：\n");
    for (int i = 0; i < G.numVertexes; i++) {
        printf("V%c <- ", G.inList[i].data);
        EdgeNode *p = G.inList[i].firstin;
        while (p) {
            printf("%d <- ", p->from);
            p = p->next;
        }
        printf("^\n");
    }
}

int main() {
    InverseGraph G;
    createInverseGraph(&G);
    printInverseGraph(G);
    return 0;
}

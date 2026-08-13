#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100  // 最大顶点数

// 边表结点
typedef struct EdgeNode {
    int adjvex;                    // 邻接点下标
    struct EdgeNode *next;        // 指向下一条边
} EdgeNode;

// 顶点表结点
typedef struct VertexNode {
    char data;                    // 顶点信息
    EdgeNode *firstedge;         // 指向边表的头指针
} VertexNode, AdjList[MAXVEX];

// 图结构体（邻接表）
typedef struct {
    AdjList adjList;              // 邻接表
    int numVertexes;              // 顶点个数
    int numEdges;                 // 边的条数
} GraphAdjList;

// 创建图 6-16 的邻接表（有向图）
void createGraph(GraphAdjList *G) {
    G->numVertexes = 4;
    G->numEdges = 5;

    // 初始化顶点名称（V0 ~ V3）
    for (int i = 0; i < G->numVertexes; i++) {
        G->adjList[i].data = '0' + i;
        G->adjList[i].firstedge = NULL;
    }

    // 有向边集合（只添加一个方向）
    int edges[5][2] = {
        {0, 3}, {1, 0}, {1, 2},
        {2, 0}, {2, 1}
    };

    // 添加每条边
    for (int i = 0; i < G->numEdges; i++) {
        int from = edges[i][0];
        int to = edges[i][1];

        EdgeNode *e = (EdgeNode *)malloc(sizeof(EdgeNode));
        e->adjvex = to;
        e->next = G->adjList[from].firstedge;
        G->adjList[from].firstedge = e;
    }
}

// 打印邻接表
void printGraph(GraphAdjList G) {
    printf("图的邻接表如下：\n");
    for (int i = 0; i < G.numVertexes; i++) {
        printf("V%c -> ", G.adjList[i].data);
        EdgeNode *p = G.adjList[i].firstedge;
        while (p) {
            printf("%d -> ", p->adjvex);
            p = p->next;
        }
        printf("^\n");
    }
}

int main() {
    GraphAdjList G;
    createGraph(&G);
    printGraph(G);
    return 0;
}

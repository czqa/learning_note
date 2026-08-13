#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100  // 最大顶点数

// 定义边表结点结构体
typedef struct EdgeNode {
    int adjvex;                    // 邻接点在顶点数组中的下标
    struct EdgeNode *next;        // 指向下一个邻接点
} EdgeNode;

// 定义顶点表结点结构体
typedef struct VertexNode {
    char data;                    // 顶点信息（这里用字符代替）
    EdgeNode *firstedge;         // 指向边表的头指针
} VertexNode, AdjList[MAXVEX];

// 图的结构体
typedef struct {
    AdjList adjList;              // 邻接表
    int numVertexes;              // 顶点数
    int numEdges;                 // 边数
} GraphAdjList;

// 创建图 邻接表（无向图）
void createGraph(GraphAdjList *G) {
    G->numVertexes = 4;
    G->numEdges = 5;

    // 顶点命名（V0 ~ V3）
    for (int i = 0; i < G->numVertexes; i++) {
        G->adjList[i].data = '0' + i;
        G->adjList[i].firstedge = NULL;
    }

    // 边集合（无向边，需添加两次）
    int edges[5][2] = {
        {0, 1}, {0, 2}, {0, 3},
        {1, 2}, {2, 3}
    };

    for (int i = 0; i < G->numEdges; i++) {
        int from = edges[i][0];
        int to = edges[i][1];

        // from → to
        EdgeNode *e1 = (EdgeNode *)malloc(sizeof(EdgeNode));
        e1->adjvex = to;
        e1->next = G->adjList[from].firstedge;
        G->adjList[from].firstedge = e1;

        // to → from（因为是无向图）
        EdgeNode *e2 = (EdgeNode *)malloc(sizeof(EdgeNode));
        e2->adjvex = from;
        e2->next = G->adjList[to].firstedge;
        G->adjList[to].firstedge = e2;
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

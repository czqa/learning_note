#include <stdio.h>
#include <stdlib.h>
#define MAXVEX 100
// 边表结构：包含邻接点编号和边的权值
typedef struct EdgeNode {
    int adjvex;                 // 邻接点下标
    int weight;                 // 边的权值
    struct EdgeNode *next;     // 指向下一个边结点
} EdgeNode;

// 顶点结构
typedef struct VertexNode {
    char data;                 // 顶点信息
    EdgeNode *firstedge;      // 指向边表的头指针
} VertexNode, AdjList[MAXVEX];

// 图的结构体
typedef struct {
    AdjList adjList;           // 邻接表数组
    int numVertexes;           // 顶点数
    int numEdges;              // 边数
} GraphAdjList;

// 构建图 6-18 的邻接表（有向 + 带权）
void createGraph(GraphAdjList *G) {
    G->numVertexes = 4;
    G->numEdges = 5;

    // 初始化顶点
    for (int i = 0; i < G->numVertexes; i++) {
        G->adjList[i].data = '0' + i;       // 顶点命名 V0~V3
        G->adjList[i].firstedge = NULL;
    }

    // 边集合（起点，终点，权值）
    int edges[5][3] = {
        {0, 3, 30},
        {1, 0, 50},
        {1, 2, 20},
        {2, 0, 60},
        {2, 1, 40}
    };

    // 插入所有边（有向边，只插入一方向）
    for (int i = 0; i < G->numEdges; i++) {
        int from = edges[i][0];
        int to = edges[i][1];
        int weight = edges[i][2];

        EdgeNode *e = (EdgeNode *)malloc(sizeof(EdgeNode));
        e->adjvex = to;
        e->weight = weight;
        e->next = G->adjList[from].firstedge;
        G->adjList[from].firstedge = e;
    }
}

// 打印邻接表（包含权值）
void printGraph(GraphAdjList G) {
    printf("图的邻接表如下（格式：目标点[权值]）：\n");
    for (int i = 0; i < G.numVertexes; i++) {
        printf("V%c -> ", G.adjList[i].data);
        EdgeNode *p = G.adjList[i].firstedge;
        while (p) {
            printf("%d[%d] -> ", p->adjvex, p->weight);
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

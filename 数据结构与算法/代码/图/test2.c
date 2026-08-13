#include <stdio.h>

typedef char VertexType;   // 顶点的数据类型，这里用字符表示顶点
typedef int EdgeType;      // 边的权值类型（此处为简单图，仅用0和1表示是否有边）

#define MAXSIZE 100        // 最大顶点数，可根据需要修改

// 定义图的邻接矩阵结构体
typedef struct
{
    VertexType vertex[MAXSIZE];         // 顶点数组
    EdgeType arc[MAXSIZE][MAXSIZE];     // 邻接矩阵（二维数组）
    int vertex_num;                     // 顶点数
    int edge_num;                       // 边的数目
} Mat_Grph;

// 创建图的邻接矩阵（以图6-13为例，有向图）
void create_graph(Mat_Grph* G)
{
    G->vertex_num = 4;   // 顶点数：V0~V3
    G->edge_num = 4;     // 有向边数为4

    // 顶点命名
    G->vertex[0] = '0';
    G->vertex[1] = '1';
    G->vertex[2] = '2';
    G->vertex[3] = '3';

    // 初始化邻接矩阵，全部赋值为0
    for (int i = 0; i < G->vertex_num; i++)
    {
        for (int j = 0; j < G->vertex_num; j++)
        {
            G->arc[i][j] = 0;
        }
    }

     // 添加有向边
    G->arc[0][3] = 1;  // 0 → 3
    G->arc[1][0] = 1;  // 1 → 0
    G->arc[1][2] = 1;  // 1 → 2
    G->arc[2][0] = 1;  // 2 → 0
    G->arc[2][1] = 1;  // 2 → 1
}

// 输出邻接矩阵
void print_matrix(Mat_Grph G)
{
    printf("邻接矩阵表示如下：\n");
    for (int i = 0; i < G.vertex_num; i++)
    {
        for (int j = 0; j < G.vertex_num; j++)
        {
            printf("%d ", G.arc[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char const *argv[])
{
    Mat_Grph G;               // 定义一个图结构体变量
    create_graph(&G);        // 构建邻接矩阵
    print_matrix(G);         // 输出邻接矩阵
    return 0;
}

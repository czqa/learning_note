#include <stdio.h>

typedef char VertexType;   // 顶点类型
typedef int EdgeType;      // 边的权值类型（0 或 1）
#define MAXSIZE 100        // 最大顶点数

// 图的邻接矩阵结构体定义
typedef struct
{
    VertexType vertex[MAXSIZE];         // 顶点数组
    EdgeType arc[MAXSIZE][MAXSIZE];     // 邻接矩阵（二维数组）
    int vertex_num;                     // 顶点个数
    int edge_num;                       // 边的条数
} Mat_Grph;

// 创建图的邻接矩阵
void create_graph(Mat_Grph* G)
{
    G->vertex_num = 4;   // 顶点数：V0~V3
    G->edge_num = 5;     // 边数

    // 顶点命名
    G->vertex[0] = '0';
    G->vertex[1] = '1';
    G->vertex[2] = '2';
    G->vertex[3] = '3';

    // 初始化邻接矩阵
    for (int i = 0; i < G->vertex_num; i++)
    {
        for (int j = 0; j < G->vertex_num; j++)
        {
            G->arc[i][j] = 0;
        }
    }

    // 添加边（无向图，因此要对称赋值）
    G->arc[0][1] = 1;  G->arc[1][0] = 1;  // V0-V1
    G->arc[0][2] = 1;  G->arc[2][0] = 1;  // V0-V2
    G->arc[0][3] = 1;  G->arc[3][0] = 1;  // V0-V3
    G->arc[1][2] = 1;  G->arc[2][1] = 1;  // V1-V2
    G->arc[2][3] = 1;  G->arc[3][2] = 1;  // V2-V3
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
    Mat_Grph G; //创建一个邻接矩阵的结构体变量
    create_graph(&G); //初始化邻接矩阵
    print_matrix(G); //输出邻接矩阵
    return 0;
}

#include <stdio.h>
#define MAXSIZE 100
#define Max 0x7fffffff   // 用一个极大值表示“无穷”，即无边
typedef char VertexType;   // 顶点类型
typedef int EdgeType;      // 边权类型

// 图的邻接矩阵结构定义
typedef struct {
    VertexType vertex[MAXSIZE];       // 顶点数组
    EdgeType arc[MAXSIZE][MAXSIZE];   // 邻接矩阵
    int vertex_num;                   // 顶点个数
    int edge_num;                     // 边的条数
} Mat_Grph;

// 创建图（带权有向图）
void create_graph(Mat_Grph* G) {
    G->vertex_num = 4;   // 顶点编号：0~3
    G->edge_num = 5;     // 5 条边

    // 初始化顶点标识（可选）
    for (int i = 0; i < G->vertex_num; i++) {
        G->vertex[i] = '0' + i;
    }

    // 初始化邻接矩阵，默认全部为 Max（表示无边）
    for (int i = 0; i < G->vertex_num; i++) {
        for (int j = 0; j < G->vertex_num; j++) {
            G->arc[i][j] = Max;
            if (i == j)
            {
                G->arc[i][j] = 0;
            }
        }
    }

    // 添加边及其权值（由图提供）
    G->arc[0][3] = 3;  // 0 → 3，权值为 3
    G->arc[1][0] = 5;  // 1 → 0，权值为 5
    G->arc[1][2] = 2;  // 1 → 2，权值为 2
    G->arc[2][0] = 6;  // 2 → 0，权值为 6
    G->arc[2][1] = 4;  // 2 → 1，权值为 4
}

// 打印邻接矩阵
void print_matrix(Mat_Grph G) {
    printf("图的邻接矩阵如下（Max 表示无边）：\n");
    for (int i = 0; i < G.vertex_num; i++) {
        for (int j = 0; j < G.vertex_num; j++) {
            if (G.arc[i][j] == Max)
                printf("Max ");
            else
                printf("%3d ", G.arc[i][j]);
        }
        printf("\n");
    }
}

int main() {
    Mat_Grph G; // 定义一个图结构体变量
    create_graph(&G); // 构建邻接矩阵
    print_matrix(G); //输出邻接矩阵
    return 0;
}

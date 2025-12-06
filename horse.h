#ifndef HORSE_H
#define HORSE_H

// ===== 棋盘尺寸定义（8x8 棋盘）=====
#define size_x 8
#define size_y 8
#define able_step 8
#define Total_step (size_x * size_y)

// qsort 排序用的全局指针（在 horse.cpp 中定义）
extern const int *ptr;

// 骑士巡游算法类（使用 Warnsdorff 规则）
class horse
{
public:
    // 起点坐标（0-based 网格坐标）
    static int begin_x;
    static int begin_y;

    // 8 个移动方向
    static int dir_x[size_x];
    static int dir_y[size_y];

    // Warnsdorff 排序用的下标数组
    static int j_c_Warnsdorff_sortarray[size_x];

    // 棋盘：0 = 未访问；>0 = 第几步访问
    static int grid[size_x][size_y];

    // 是否要求闭合巡游（true = 最后一步必须能一步回到起点）
    static bool closedTour;

    // 清空棋盘（全部置为 0）
    static void initBoard();

    // 设置是否要求闭合巡游
    static void setClosedTour(bool closed);

    // 使用 0-based 坐标求解骑士巡游
    // startX: 0..size_x-1, startY: 0..size_y-1
    // 返回 true 表示找到完整的巡游路径
    static bool solve(int startX, int startY);

    // 兼容旧接口：x, y 为 1-based 坐标
    // 等价于：initBoard() + 在 (x-1, y-1) 放置起点
    static void init(int x, int y);

    static void print_result();
    static int in_grid(int x, int y);
    static int compare(const void *p1, const void *p2);
    static void every_init(int deep);
    static void sort_index(const int array[], int index[], int num);
    static int hores_traversal(int x, int y, int deep);
    static void sort_j_c_Warnsdorff(int x, int y);
};

#endif // HORSE_H

#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QPoint>
#include <QVector>
#include <QTimer>
#include <QPainter>
#include <QEasingCurve>
#include <QTimeLine>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QElapsedTimer>

// 棋盘主题枚举
enum class BoardTheme {
    Light,      // 亮色主题
    Dark,       // 深色主题
    Wood        // 古典木纹
};

// 动画状态结构
struct AnimationState {
    QPoint fromPos;         // 起始位置
    QPoint toPos;           // 目标位置
    double progress;        // 动画进度 0.0-1.0
    bool isAnimating;       // 是否正在动画
};

class ChessBoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChessBoardWidget(QWidget *parent = nullptr);

    // 设置起始位置
    void setStartPosition(int row, int col);

    // 开始演示
    void startTour(bool useWarnsdorff = false);

    // 停止演示
    void stopTour();

    // 重置棋盘
    void reset();

    // 设置演示速度（毫秒）
    void setAnimationSpeed(int milliseconds);

    // 设置棋盘主题
    void setBoardTheme(BoardTheme theme);

    // 步进控制
    void goToStep(int step);
    void nextStep();
    void previousStep();

    // 检查指定位置是否可闭合巡游
    bool checkClosedTour(int row, int col, bool useWarnsdorff = true);

    // 自动选择可闭合起点
    QPoint findClosedTourStart(bool useWarnsdorff = true);

    // 获取当前步数
    int getCurrentStep() const { return currentStep; }

    // 获取总步数
    int getTotalSteps() const { return tourPath.size(); }

    // 获取路径
    QVector<QPoint> getTourPath() const { return tourPath; }

    // 获取算法统计信息
    struct AlgorithmStats {
        int attempts;           // 尝试次数
        int elapsedMs;          // 耗时（毫秒）
        bool success;           // 是否成功
        QString algorithmName;  // 算法名称
    };
    AlgorithmStats getLastStats() const { return lastStats; }

    // 导出功能
    bool exportStepImages(const QString &basePath);  // 导出每步PNG
    bool exportGifAnimation(const QString &filePath, int frameDelay = 100);  // 导出GIF动画

    // 摄像机控制
    void setCameraFollow(bool enabled);  // 启用/禁用摄像机跟随
    void setCameraZoom(double zoom);     // 设置缩放比例 (1.0 = 正常, >1.0 = 放大)
    bool getCameraFollow() const { return cameraFollowEnabled; }
    double getCameraZoom() const { return cameraZoom; }

    // Warnsdorff可视化控制
    void setShowWarnsdorffDegrees(bool show);  // 设置是否显示Warnsdorff度数

signals:
    void tourCompleted();
    void tourFailed();
    void stepChanged(int step);  // 步数改变信号
    void hoveredCellChanged(int row, int col, bool canClose);  // 悬停格子改变
    void exportProgress(int current, int total);  // 导出进度信号

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onAnimationTimer();
    void onAnimationFrameChanged(int frame);

private:
    // 棋盘相关
    static const int BOARD_SIZE = 8;
    int cellSize;
    int boardOffsetX;
    int boardOffsetY;
    BoardTheme boardTheme;  // 当前主题

    // 骑士位置和路径
    QPoint startPos;          // 起始位置 (row, col)，范围 0-7
    QPoint currentKnightPos;  // 当前骑士位置
    QPoint animatedKnightPos; // 动画中的骑士位置
    QVector<QPoint> tourPath; // 完整路径
    int currentStep;          // 当前演示步骤
    bool isSelectingStart;    // 是否正在选择起始位置
    bool isTourRunning;       // 是否正在演示
    bool isManualStepMode;   // 是否为手动步进模式

    // 鼠标悬停
    QPoint hoveredCell;       // 当前悬停的格子 (-1, -1) 表示无悬停
    QVector<QVector<bool>> closedTourCache;  // 可闭合巡游缓存

    // 动画
    QTimer *animationTimer;
    QTimeLine *jumpAnimation;  // 跳跃动画时间线
    int animationSpeed;
    AnimationState animState;   // 动画状态
    QEasingCurve easingCurve;  // 缓动曲线

    // 算法相关
    bool useWarnsdorffRule;
    AlgorithmStats lastStats;   // 最后一次算法统计

    // Warnsdorff可视化
    bool showWarnsdorffDegrees;  // 是否显示候选度数
    QVector<QVector<int>> warnsdorffDegrees;  // 当前步骤的候选度数

    // 摄像机跟随
    bool cameraFollowEnabled;   // 是否启用摄像机跟随
    double cameraZoom;           // 当前缩放比例
    QPointF cameraOffset;        // 摄像机偏移量（用于跟随）

    // 导出相关
    QPixmap renderToPixmap(int step = -1);  // 渲染当前状态到Pixmap（-1表示当前步骤）

    // 辅助函数
    QPoint screenToBoard(const QPoint &screenPos) const;
    QRect getCellRect(int row, int col) const;
    void drawChessBoard(QPainter &painter);
    void drawKnight(QPainter &painter, int row, int col);
    void drawKnightAnimated(QPainter &painter);  // 绘制动画中的骑士
    void drawPath(QPainter &painter);
    void drawNumbers(QPainter &painter);
    void drawHoverIndicator(QPainter &painter);  // 绘制悬停指示器
    void drawWarnsdorffDegrees(QPainter &painter);  // 绘制Warnsdorff度数
    void drawCoordinates(QPainter &painter);  // 绘制坐标轴

    // 主题相关
    QColor getLightCellColor() const;
    QColor getDarkCellColor() const;
    void drawWoodTexture(QPainter &painter, const QRect &rect, bool isLight);

    // 骑士移动相关
    static const int KNIGHT_MOVES[8][2];
    bool isValidMove(int row, int col) const;
    int countPossibleMoves(int row, int col, const QVector<QVector<bool>> &visited) const;

    // 预计算的闭合巡游路径（从(0,0)出发）
    static const int BASE_CLOSED_TOUR[64][2];
    QVector<QPoint> findClosedTourFromPrecomputed(int startRow, int startCol) const;

    // 算法实现
    bool solveKnightTour(int row, int col, int moveCount, QVector<QVector<bool>> &visited, QVector<QPoint> &path);
    bool solveKnightTourWarnsdorff(int row, int col, int moveCount, QVector<QVector<bool>> &visited, QVector<QPoint> &path);

    // 改进的闭合巡游算法（优先考虑闭合条件）
    bool solveKnightTourClosed(int row, int col, int moveCount,
                               QVector<QVector<bool>> &visited,
                               QVector<QPoint> &path,
                               const QPoint &startPos);
    bool solveKnightTourWarnsdorffClosed(int row, int col, int moveCount,
                                         QVector<QVector<bool>> &visited,
                                         QVector<QPoint> &path,
                                         const QPoint &startPos);

    QVector<QPoint> getValidMoves(int row, int col, const QVector<QVector<bool>> &visited) const;

    // 检查闭合巡游（快速检查，不完整求解）
    bool quickCheckClosedTour(int row, int col, bool useWarnsdorff);

    // 辅助函数：有限深度的快速检查
    bool quickCheckClosedTourHelper(int row, int col, int moveCount,
                                    QVector<QVector<bool>> &visited,
                                    QVector<QPoint> &path,
                                    bool useWarnsdorff, int maxDepth);

    // 更新Warnsdorff度数显示
    void updateWarnsdorffDegrees(int step);

    // 启动跳跃动画
    void startJumpAnimation(const QPoint &from, const QPoint &to);
};

#endif // CHESSBOARDWIDGET_H

#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include "horse.h"
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

// 棋盘主题
enum class BoardTheme {
    Light,
    Dark,
    Wood
};

struct AnimationState {
    QPoint fromPos;
    QPoint toPos;
    double progress;
    bool isAnimating;
};

class ChessBoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChessBoardWidget(QWidget *parent = nullptr);

    void setStartPosition(int row, int col);
    void startTour(bool useWarnsdorff = false);
    void stopTour();
    void reset();
    void setAnimationSpeed(int milliseconds);
    void setBoardTheme(BoardTheme theme);

    void goToStep(int step);
    void nextStep();
    void previousStep();

    bool checkClosedTour(int row, int col, bool useWarnsdorff = true);
    QPoint findClosedTourStart(bool useWarnsdorff = true);

    int getCurrentStep() const { return currentStep; }
    int getTotalSteps() const { return tourPath.size(); }
    QVector<QPoint> getTourPath() const { return tourPath; }

    struct AlgorithmStats {
        int attempts;
        int elapsedMs;
        bool success;
        QString algorithmName;
    };
    AlgorithmStats getLastStats() const { return lastStats; }

    bool exportStepImages(const QString &basePath);
    bool exportGifAnimation(const QString &filePath, int frameDelay = 100);

    void setCameraFollow(bool enabled);
    void setCameraZoom(double zoom);
    bool getCameraFollow() const { return cameraFollowEnabled; }
    double getCameraZoom() const { return cameraZoom; }

    void setShowWarnsdorffDegrees(bool show);

signals:
    void tourCompleted();
    void tourFailed();
    void stepChanged(int step);
    void hoveredCellChanged(int row, int col, bool canClose);
    void exportProgress(int current, int total);

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
    // 棋盘
    static const int BOARD_SIZE = 8;
    int cellSize;
    int boardOffsetX;
    int boardOffsetY;
    BoardTheme boardTheme;

    // 状态
    QPoint startPos;
    QPoint currentKnightPos;
    QPoint animatedKnightPos;
    QVector<QPoint> tourPath;
    int currentStep;
    bool isSelectingStart;
    bool isTourRunning;
    bool isManualStepMode;

    QPoint hoveredCell;
    QVector<QVector<bool>> closedTourCache;

    QTimer *animationTimer;
    QTimeLine *jumpAnimation;
    int animationSpeed;
    AnimationState animState;
    QEasingCurve easingCurve;

    bool useWarnsdorffRule;
    AlgorithmStats lastStats;

    bool showWarnsdorffDegrees;
    QVector<QVector<int>> warnsdorffDegrees;

    bool cameraFollowEnabled;
    double cameraZoom;
    QPointF cameraOffset;

    // 绘图辅助
    QPixmap renderToPixmap(int step = -1);
    QPoint screenToBoard(const QPoint &screenPos) const;
    QRect getCellRect(int row, int col) const;
    void drawChessBoard(QPainter &painter);
    void drawKnight(QPainter &painter, int row, int col);
    void drawKnightAnimated(QPainter &painter);
    void drawPath(QPainter &painter);
    void drawNumbers(QPainter &painter);
    void drawHoverIndicator(QPainter &painter);
    void drawWarnsdorffDegrees(QPainter &painter);
    void drawCoordinates(QPainter &painter);

    QColor getLightCellColor() const;
    QColor getDarkCellColor() const;
    void drawWoodTexture(QPainter &painter, const QRect &rect, bool isLight);

    // 骑士移动辅助
    static const int KNIGHT_MOVES[8][2];
    bool isValidMove(int row, int col) const;
    int countPossibleMoves(int row, int col, const QVector<QVector<bool>> &visited) const;
    QVector<QPoint> getValidMoves(int row, int col, const QVector<QVector<bool>> &visited) const;

    // 预计算路径
    static const int BASE_CLOSED_TOUR[64][2];
    QVector<QPoint> findClosedTourFromPrecomputed(int startRow, int startCol) const;

    // 👉 这里是唯一保留的算法实现（调用 horse.cpp）
    bool solveKnightTourHorseWrapper(int row, int col, QVector<QPoint> &path);

    // 快速判断闭合（不计算）
    bool quickCheckClosedTour(int row, int col, bool useWarnsdorff);

    void updateWarnsdorffDegrees(int step);
    void startJumpAnimation(const QPoint &from, const QPoint &to);
};

#endif // CHESSBOARDWIDGET_H

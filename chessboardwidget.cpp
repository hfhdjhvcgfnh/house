#include "chessboardwidget.h"
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainterPath>
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>

// 骑士的8种可能移动方向
const int ChessBoardWidget::KNIGHT_MOVES[8][2] = {
    {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
    {1, -2},  {1, 2},  {2, -1},  {2, 1}
};

// 预计算的闭合骑士巡游路径（从(0,0)出发，访问所有64个格子，最后可以回到(0,0)）
// 这个路径是经过验证的闭合巡游，可以保证从任意起点都能找到闭合路径
const int ChessBoardWidget::BASE_CLOSED_TOUR[64][2] = {
    {0, 0}, {1, 2}, {2, 0}, {0, 1}, {1, 3}, {0, 5}, {1, 7}, {3, 6},
    {5, 7}, {7, 6}, {6, 4}, {7, 2}, {6, 0}, {4, 1}, {6, 2}, {7, 0},
    {5, 1}, {3, 0}, {1, 1}, {0, 3}, {1, 5}, {0, 7}, {2, 6}, {4, 7},
    {6, 6}, {7, 4}, {5, 5}, {6, 7}, {7, 5}, {6, 3}, {7, 1}, {5, 0},
    {3, 1}, {1, 0}, {2, 2}, {4, 3}, {2, 4}, {4, 5}, {3, 7}, {1, 6},
    {0, 4}, {2, 5}, {0, 6}, {2, 7}, {4, 6}, {3, 4}, {5, 3}, {3, 2},
    {4, 0}, {6, 1}, {7, 3}, {5, 2}, {3, 3}, {5, 4}, {4, 2}, {2, 3},
    {4, 4}, {6, 5}, {7, 7}, {5, 6}, {3, 5}, {1, 4}, {0, 2}, {2, 1}
};

ChessBoardWidget::ChessBoardWidget(QWidget *parent)
    : QWidget(parent)
    , cellSize(60)
    , boardOffsetX(20)
    , boardOffsetY(20)
    , boardTheme(BoardTheme::Light)
    , currentStep(0)
    , isSelectingStart(true)
    , isTourRunning(false)
    , isManualStepMode(false)
    , hoveredCell(-1, -1)
    , animationSpeed(300)
    , useWarnsdorffRule(false)
    , showWarnsdorffDegrees(false)
    , cameraFollowEnabled(false)
    , cameraZoom(1.0)
    , cameraOffset(0, 0)
{
    // 初始化动画定时器
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &ChessBoardWidget::onAnimationTimer);

    // 初始化跳跃动画时间线
    jumpAnimation = new QTimeLine(animationSpeed, this);
    jumpAnimation->setFrameRange(0, 100);
    jumpAnimation->setUpdateInterval(16); // ~60fps
    connect(jumpAnimation, &QTimeLine::frameChanged, this, &ChessBoardWidget::onAnimationFrameChanged);
    connect(jumpAnimation, &QTimeLine::finished, this, [this]() {
        animState.isAnimating = false;
        update();
    });

    // 设置缓动曲线（类似马的跳跃）
    easingCurve.setType(QEasingCurve::OutCubic);

    // 初始化动画状态
    animState.isAnimating = false;
    animState.progress = 0.0;

    // 初始化可闭合巡游缓存
    // 使用预计算路径，所有64个位置都可以找到闭合巡游
    closedTourCache.resize(BOARD_SIZE);
    for (int i = 0; i < BOARD_SIZE; i++) {
        closedTourCache[i].resize(BOARD_SIZE);
        // 预计算路径包含所有64个位置，所以初始化为true
        closedTourCache[i].fill(true);
    }

    // 初始化Warnsdorff度数
    warnsdorffDegrees.resize(BOARD_SIZE);
    for (int i = 0; i < BOARD_SIZE; i++) {
        warnsdorffDegrees[i].resize(BOARD_SIZE);
        warnsdorffDegrees[i].fill(-1);
    }

    setMinimumSize(520, 520);
    setMouseTracking(true);  // 启用鼠标跟踪以支持悬停效果
}

void ChessBoardWidget::setStartPosition(int row, int col)
{
    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        startPos = QPoint(row, col);
        currentKnightPos = startPos;
        isSelectingStart = false;
        update();
    }
}

void ChessBoardWidget::startTour(bool useWarnsdorff)
{
    if (isSelectingStart || startPos.x() < 0 || startPos.y() < 0) {
        return;
    }

    useWarnsdorffRule = useWarnsdorff;
    tourPath.clear();
    currentStep = 0;
    isTourRunning = false;

    // 记录求解时间
    QElapsedTimer timer;
    timer.start();

    QVector<QPoint> path;
    bool success = false;
    int attempts = 0;

    // 如果使用Warnsdorff规则，必须使用动态算法（严格遵循Warnsdorff规则）
    // 预计算路径可能不遵循Warnsdorff规则
    if (useWarnsdorff) {
        // 使用Warnsdorff规则的动态算法
        const int maxAttempts = 30;

        while (!success && attempts < maxAttempts) {
            QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
            path.clear();

            visited[startPos.x()][startPos.y()] = true;
            path.append(startPos);

            success = solveKnightTourWarnsdorffClosed(startPos.x(), startPos.y(), 1, visited, path, startPos);

            attempts++;

            // 如果找到路径，检查是否能闭合
            if (success && path.size() == BOARD_SIZE * BOARD_SIZE) {
                QPoint lastPos = path.last();
                bool canReturn = false;
                for (int i = 0; i < 8; i++) {
                    int newRow = lastPos.x() + KNIGHT_MOVES[i][0];
                    int newCol = lastPos.y() + KNIGHT_MOVES[i][1];
                    if (newRow == startPos.x() && newCol == startPos.y()) {
                        canReturn = true;
                        break;
                    }
                }
                if (!canReturn) {
                    success = false;  // 不能闭合，继续尝试
                } else {
                    // 添加回到起点的步骤
                    path.append(startPos);
                    break;
                }
            }

            // 处理事件，避免界面冻结
            if (attempts % 5 == 0) {
                QApplication::processEvents();
            }
        }
    } else {
        // 不使用Warnsdorff规则，优先使用预计算路径（快速且保证成功）
        path = findClosedTourFromPrecomputed(startPos.x(), startPos.y());
        success = !path.isEmpty();
        attempts = 1;

        // 如果预计算路径失败（理论上不会发生），尝试动态求解
        if (!success) {
            const int maxAttempts = 20;

            while (!success && attempts < maxAttempts) {
                QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));
                path.clear();

                visited[startPos.x()][startPos.y()] = true;
                path.append(startPos);

                success = solveKnightTourClosed(startPos.x(), startPos.y(), 1, visited, path, startPos);

                attempts++;

                // 如果找到路径，检查是否能闭合
                if (success && path.size() == BOARD_SIZE * BOARD_SIZE) {
                    QPoint lastPos = path.last();
                    bool canReturn = false;
                    for (int i = 0; i < 8; i++) {
                        int newRow = lastPos.x() + KNIGHT_MOVES[i][0];
                        int newCol = lastPos.y() + KNIGHT_MOVES[i][1];
                        if (newRow == startPos.x() && newCol == startPos.y()) {
                            canReturn = true;
                            break;
                        }
                    }
                    if (!canReturn) {
                        success = false;  // 不能闭合，继续尝试
                    } else {
                        // 添加回到起点的步骤
                        path.append(startPos);
                        break;
                    }
                }

                // 处理事件，避免界面冻结
                if (attempts % 5 == 0) {
                    QApplication::processEvents();
                }
            }
        }
    }

    int elapsedMs = timer.elapsed();

    if (success && path.size() >= BOARD_SIZE * BOARD_SIZE) {
        tourPath = path;
        currentStep = 0;
        currentKnightPos = startPos;
        animatedKnightPos = startPos;
        isTourRunning = true;

        // 更新缓存，标记该起点可以闭合巡游
        if (startPos.x() >= 0 && startPos.x() < BOARD_SIZE &&
            startPos.y() >= 0 && startPos.y() < BOARD_SIZE) {
            closedTourCache[startPos.x()][startPos.y()] = true;
        }

        // 记录算法统计信息
        lastStats.attempts = attempts;
        lastStats.elapsedMs = elapsedMs;
        lastStats.success = true;
        lastStats.algorithmName = useWarnsdorff ? tr("Warnsdorff规则（严格实现）") : tr("基础回溯法（预计算路径）");

        animationTimer->start(animationSpeed);
        update();
    } else {
        emit tourFailed();
    }
}

void ChessBoardWidget::stopTour()
{
    animationTimer->stop();
    isTourRunning = false;
    update();
}

void ChessBoardWidget::reset()
{
    stopTour();
    tourPath.clear();
    currentStep = 0;
    startPos = QPoint(-1, -1);
    currentKnightPos = QPoint(-1, -1);
    isSelectingStart = true;
    update();
}

void ChessBoardWidget::setAnimationSpeed(int milliseconds)
{
    animationSpeed = milliseconds;
    if (animationTimer->isActive()) {
        animationTimer->setInterval(animationSpeed);
    }
}

void ChessBoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 应用摄像机变换（缩放和偏移）
    if (cameraFollowEnabled && cameraZoom != 1.0) {
        painter.save();

        // 计算摄像机应该跟随的位置（当前骑士位置）
        if (currentKnightPos.x() >= 0 && currentKnightPos.y() >= 0) {
            QRect knightRect = getCellRect(currentKnightPos.x(), currentKnightPos.y());
            QPointF knightCenter = knightRect.center();
            QPointF widgetCenter(width() / 2.0, height() / 2.0);

            // 计算偏移量，使骑士居中
            cameraOffset = widgetCenter - knightCenter * cameraZoom;
        }

        // 应用缩放和平移
        painter.translate(cameraOffset);
        painter.scale(cameraZoom, cameraZoom);
    }

    // 计算棋盘大小和位置
    int boardPixelSize = BOARD_SIZE * cellSize;
    boardOffsetX = (width() - boardPixelSize) / 2;
    boardOffsetY = (height() - boardPixelSize) / 2;

    // 绘制棋盘
    drawChessBoard(painter);

    // 绘制悬停指示器（显示是否可闭合巡游）
    if (hoveredCell.x() >= 0 && hoveredCell.y() >= 0 && isSelectingStart) {
        drawHoverIndicator(painter);
    }

    // 绘制Warnsdorff度数（如果启用）
    if (showWarnsdorffDegrees && !tourPath.isEmpty() && currentStep > 0) {
        drawWarnsdorffDegrees(painter);
    }

    // 绘制路径
    if (!tourPath.isEmpty() && currentStep > 0) {
        drawPath(painter);
    }

    // 绘制数字
    if (!tourPath.isEmpty() && currentStep > 0) {
        drawNumbers(painter);
    }

    // 绘制起始位置标记
    if (startPos.x() >= 0 && startPos.y() >= 0) {
        QRect cellRect = getCellRect(startPos.x(), startPos.y());
        painter.setPen(QPen(QColor(255, 0, 0), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(cellRect.adjusted(5, 5, -5, -5));
    }

    // 绘制坐标轴
    drawCoordinates(painter);

    // 绘制骑士（动画或静态）
    if (animState.isAnimating) {
        drawKnightAnimated(painter);
    } else if (currentKnightPos.x() >= 0 && currentKnightPos.y() >= 0) {
        drawKnight(painter, currentKnightPos.x(), currentKnightPos.y());
    }

    // 恢复变换
    if (cameraFollowEnabled && cameraZoom != 1.0) {
        painter.restore();
    }
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isSelectingStart && !isTourRunning) {
        QPoint boardPos = screenToBoard(event->pos());
        if (boardPos.x() >= 0 && boardPos.x() < BOARD_SIZE &&
            boardPos.y() >= 0 && boardPos.y() < BOARD_SIZE) {
            setStartPosition(boardPos.x(), boardPos.y());
        }
    }
}

void ChessBoardWidget::mouseMoveEvent(QMouseEvent *event)
{
    // 检测鼠标悬停的格子
    QPoint boardPos = screenToBoard(event->pos());
    if (boardPos.x() >= 0 && boardPos.x() < BOARD_SIZE &&
        boardPos.y() >= 0 && boardPos.y() < BOARD_SIZE) {
        if (hoveredCell != boardPos) {
            hoveredCell = boardPos;

            // 检查该位置是否可闭合巡游
            // 使用预计算路径，所有位置都可以（快速且不阻塞）
            bool canClose = quickCheckClosedTour(boardPos.x(), boardPos.y(), useWarnsdorffRule);

            emit hoveredCellChanged(boardPos.x(), boardPos.y(), canClose);
            update();
        }
    } else {
        if (hoveredCell.x() >= 0) {
            hoveredCell = QPoint(-1, -1);
            emit hoveredCellChanged(-1, -1, false);
            update();
        }
    }
}

void ChessBoardWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    if (hoveredCell.x() >= 0) {
        hoveredCell = QPoint(-1, -1);
        emit hoveredCellChanged(-1, -1, false);
        update();
    }
}

void ChessBoardWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    // 根据窗口大小调整格子大小
    int minSize = qMin(width(), height());
    cellSize = (minSize - 40) / BOARD_SIZE;
    update();
}

void ChessBoardWidget::onAnimationTimer()
{
    if (currentStep < tourPath.size()) {
        // 启动跳跃动画
        if (currentStep > 0) {
            QPoint from = tourPath[currentStep - 1];
            QPoint to = tourPath[currentStep];
            startJumpAnimation(from, to);
        }

        currentKnightPos = tourPath[currentStep];
        animatedKnightPos = currentKnightPos;
        currentStep++;

        // 更新Warnsdorff度数显示
        if (showWarnsdorffDegrees) {
            updateWarnsdorffDegrees(currentStep - 1);
        }

        emit stepChanged(currentStep);
        update();
    } else {
        animationTimer->stop();
        isTourRunning = false;
        emit tourCompleted();
    }
}

QPoint ChessBoardWidget::screenToBoard(const QPoint &screenPos) const
{
    int col = (screenPos.x() - boardOffsetX) / cellSize;
    int row = (screenPos.y() - boardOffsetY) / cellSize;
    return QPoint(row, col);
}

QRect ChessBoardWidget::getCellRect(int row, int col) const
{
    int x = boardOffsetX + col * cellSize;
    int y = boardOffsetY + row * cellSize;
    return QRect(x, y, cellSize, cellSize);
}

void ChessBoardWidget::drawChessBoard(QPainter &painter)
{
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            QRect cellRect = getCellRect(row, col);

            bool isLight = (row + col) % 2 == 0;

            if (boardTheme == BoardTheme::Wood) {
                drawWoodTexture(painter, cellRect, isLight);
            } else {
                QColor cellColor = isLight ? getLightCellColor() : getDarkCellColor();
                painter.fillRect(cellRect, cellColor);
            }

            // 绘制边框
            painter.setPen(QPen(QColor(100, 100, 100), 1));
            painter.drawRect(cellRect);
        }
    }

    // 坐标标签在drawCoordinates中绘制
}

void ChessBoardWidget::drawKnight(QPainter &painter, int row, int col)
{
    QRect cellRect = getCellRect(row, col);
    QPoint center = cellRect.center();

    // 使用Unicode字符显示国际象棋马 ♞
    painter.save();

    // 设置字体大小（根据格子大小调整）
    QFont font = painter.font();
    int fontSize = cellSize * 0.7;  // 字体大小为格子大小的70%
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);

    // 根据背景色选择文字颜色
    bool isLight = (row + col) % 2 == 0;
    painter.setPen(isLight ? QColor(0, 0, 0) : QColor(255, 255, 255));

    // 绘制Unicode马字符 ♞
    QString knightChar = QString::fromUtf8("♞");
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(knightChar);
    textRect.moveCenter(center);

    painter.drawText(textRect, Qt::AlignCenter, knightChar);

    painter.restore();
}

// 绘制动画中的骑士（带跳跃轨迹）
void ChessBoardWidget::drawKnightAnimated(QPainter &painter)
{
    if (!animState.isAnimating) {
        // 如果不在动画中，使用普通绘制
        if (animatedKnightPos.x() >= 0 && animatedKnightPos.y() >= 0) {
            drawKnight(painter, animatedKnightPos.x(), animatedKnightPos.y());
        }
        return;
    }

    // 计算当前位置（使用缓动曲线）
    double easedProgress = easingCurve.valueForProgress(animState.progress);

    QRect fromRect = getCellRect(animState.fromPos.x(), animState.fromPos.y());
    QRect toRect = getCellRect(animState.toPos.x(), animState.toPos.y());

    QPointF fromCenter = fromRect.center();
    QPointF toCenter = toRect.center();

    // 计算弧线路径（马的跳跃是弧形的）
    QPointF currentPos = fromCenter + (toCenter - fromCenter) * easedProgress;

    // 添加垂直偏移以形成弧形（跳跃高度）
    double jumpHeight = 30.0 * sin(easedProgress * M_PI);  // 正弦曲线形成弧形
    currentPos.setY(currentPos.y() - jumpHeight);

    // 保存状态并平移
    painter.save();
    painter.translate(currentPos);

    // 根据跳跃进度调整大小（跳跃时稍微放大）
    double scale = 1.0 + 0.2 * sin(easedProgress * M_PI);
    painter.scale(scale, scale);

    // 使用Unicode字符绘制动画中的马
    QFont font = painter.font();
    int fontSize = static_cast<int>(cellSize * 0.7 * scale);
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);

    // 根据背景色选择文字颜色
    bool isLight = (animState.toPos.x() + animState.toPos.y()) % 2 == 0;
    painter.setPen(isLight ? QColor(0, 0, 0) : QColor(255, 255, 255));

    QString knightChar = QString::fromUtf8("♞");
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(knightChar);
    textRect.moveCenter(QPoint(0, 0));

    painter.drawText(textRect, Qt::AlignCenter, knightChar);

    painter.restore();
}

void ChessBoardWidget::drawPath(QPainter &painter)
{
    if (currentStep <= 1) return;

    // 使用弧线绘制路径，更符合马的跳跃轨迹
    painter.setPen(QPen(QColor(0, 150, 255), 3));
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i < currentStep - 1; i++) {
        QPoint p1 = tourPath[i];
        QPoint p2 = tourPath[i + 1];

        QRect rect1 = getCellRect(p1.x(), p1.y());
        QRect rect2 = getCellRect(p2.x(), p2.y());

        QPointF center1 = rect1.center();
        QPointF center2 = rect2.center();

        // 绘制弧线路径（使用二次贝塞尔曲线）
        QPointF controlPoint = (center1 + center2) / 2;
        controlPoint.setY(qMin(center1.y(), center2.y()) - 20);  // 向上弯曲形成弧形

        QPainterPath arcPath;
        arcPath.moveTo(center1);
        arcPath.quadTo(controlPoint, center2);

        painter.drawPath(arcPath);

        // 在路径上绘制箭头指示方向
        if (i == currentStep - 2) {  // 只在最后一段路径绘制箭头
            QPointF dir = center2 - controlPoint;
            double angle = atan2(dir.y(), dir.x());
            QPointF arrowHead = center2;
            QPointF arrow1 = arrowHead + QPointF(-10 * cos(angle - M_PI/6), -10 * sin(angle - M_PI/6));
            QPointF arrow2 = arrowHead + QPointF(-10 * cos(angle + M_PI/6), -10 * sin(angle + M_PI/6));

            painter.drawLine(arrowHead, arrow1);
            painter.drawLine(arrowHead, arrow2);
        }
    }
}

void ChessBoardWidget::drawNumbers(QPainter &painter)
{
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);

    for (int i = 0; i < currentStep && i < tourPath.size(); i++) {
        QPoint pos = tourPath[i];
        QRect cellRect = getCellRect(pos.x(), pos.y());

        // 根据背景色选择文字颜色
        bool isLight = (pos.x() + pos.y()) % 2 == 0;
        painter.setPen(isLight ? QColor(0, 0, 0) : QColor(255, 255, 255));

        QString number = QString::number(i + 1);
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(number);
        textRect.moveCenter(cellRect.center());

        // 绘制背景
        painter.setBrush(QBrush(QColor(255, 255, 0, 180)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(textRect.adjusted(-3, -3, 3, 3));

        // 绘制数字
        painter.setPen(QColor(0, 0, 0));
        painter.drawText(textRect, Qt::AlignCenter, number);
    }
}

bool ChessBoardWidget::isValidMove(int row, int col) const
{
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

int ChessBoardWidget::countPossibleMoves(int row, int col, const QVector<QVector<bool>> &visited) const
{
    int count = 0;
    for (int i = 0; i < 8; i++) {
        int newRow = row + KNIGHT_MOVES[i][0];
        int newCol = col + KNIGHT_MOVES[i][1];
        if (isValidMove(newRow, newCol) && !visited[newRow][newCol]) {
            count++;
        }
    }
    return count;
}

QVector<QPoint> ChessBoardWidget::getValidMoves(int row, int col, const QVector<QVector<bool>> &visited) const
{
    QVector<QPoint> moves;
    for (int i = 0; i < 8; i++) {
        int newRow = row + KNIGHT_MOVES[i][0];
        int newCol = col + KNIGHT_MOVES[i][1];
        if (isValidMove(newRow, newCol) && !visited[newRow][newCol]) {
            moves.append(QPoint(newRow, newCol));
        }
    }
    return moves;
}

bool ChessBoardWidget::solveKnightTour(int row, int col, int moveCount,
                                       QVector<QVector<bool>> &visited,
                                       QVector<QPoint> &path)
{
    if (moveCount == BOARD_SIZE * BOARD_SIZE) {
        return true;
    }

    for (int i = 0; i < 8; i++) {
        int newRow = row + KNIGHT_MOVES[i][0];
        int newCol = col + KNIGHT_MOVES[i][1];

        if (isValidMove(newRow, newCol) && !visited[newRow][newCol]) {
            visited[newRow][newCol] = true;
            path.append(QPoint(newRow, newCol));

            if (solveKnightTour(newRow, newCol, moveCount + 1, visited, path)) {
                return true;
            }

            // 回溯
            visited[newRow][newCol] = false;
            path.removeLast();
        }
    }

    return false;
}

// 改进的闭合巡游算法（优先考虑闭合条件）
bool ChessBoardWidget::solveKnightTourClosed(int row, int col, int moveCount,
                                             QVector<QVector<bool>> &visited,
                                             QVector<QPoint> &path,
                                             const QPoint &startPos)
{
    // 如果已经访问了所有64个格子
    if (moveCount == BOARD_SIZE * BOARD_SIZE) {
        // 检查最后位置是否能回到起点
        for (int i = 0; i < 8; i++) {
            int newRow = row + KNIGHT_MOVES[i][0];
            int newCol = col + KNIGHT_MOVES[i][1];
            if (newRow == startPos.x() && newCol == startPos.y()) {
                return true;  // 可以闭合
            }
        }
        return false;  // 不能闭合，需要回溯
    }

    // 获取所有可能的移动
    QVector<QPoint> moves = getValidMoves(row, col, visited);

    if (moves.isEmpty()) {
        return false;
    }

    // 如果接近完成，优先选择能回到起点的路径
    int remaining = BOARD_SIZE * BOARD_SIZE - moveCount;

    // 对移动进行排序：优先选择能回到起点的
    if (remaining <= 15) {
        // 计算每个移动到起点的距离，并排序
        QVector<QPair<QPoint, int>> movesWithDist;
        for (const QPoint &move : moves) {
            // 使用曼哈顿距离作为启发
            int dist = abs(move.x() - startPos.x()) + abs(move.y() - startPos.y());
            movesWithDist.append(qMakePair(move, dist));
        }

        // 按距离排序
        std::sort(movesWithDist.begin(), movesWithDist.end(),
                  [](const QPair<QPoint, int> &a, const QPair<QPoint, int> &b) {
                      return a.second < b.second;
                  });

        // 如果剩余1步，只选择能直接到达起点的
        if (remaining == 1) {
            QVector<QPoint> validMoves;
            for (const auto &pair : movesWithDist) {
                QPoint move = pair.first;
                for (int i = 0; i < 8; i++) {
                    int newRow = move.x() + KNIGHT_MOVES[i][0];
                    int newCol = move.y() + KNIGHT_MOVES[i][1];
                    if (newRow == startPos.x() && newCol == startPos.y()) {
                        validMoves.append(move);
                        break;
                    }
                }
            }
            if (!validMoves.isEmpty()) {
                moves.clear();
                for (const auto &pair : movesWithDist) {
                    if (validMoves.contains(pair.first)) {
                        moves.append(pair.first);
                    }
                }
            }
        } else {
            // 重新构建moves列表（按距离排序）
            moves.clear();
            for (const auto &pair : movesWithDist) {
                moves.append(pair.first);
            }
        }
    }

    // 尝试每个移动
    for (const QPoint &move : moves) {
        visited[move.x()][move.y()] = true;
        path.append(move);

        if (solveKnightTourClosed(move.x(), move.y(), moveCount + 1, visited, path, startPos)) {
            return true;
        }

        // 回溯
        visited[move.x()][move.y()] = false;
        path.removeLast();
    }

    return false;
}

bool ChessBoardWidget::solveKnightTourWarnsdorff(int row, int col, int moveCount,
                                                 QVector<QVector<bool>> &visited,
                                                 QVector<QPoint> &path)
{
    if (moveCount == BOARD_SIZE * BOARD_SIZE) {
        return true;
    }

    // 获取所有可能的移动
    QVector<QPoint> moves = getValidMoves(row, col, visited);

    if (moves.isEmpty()) {
        return false;
    }

    // 使用Warnsdorff规则：对每个可能的移动，计算从该位置出发的可能移动数
    QVector<QPair<QPoint, int>> movesWithCount;
    for (const QPoint &move : moves) {
        int count = countPossibleMoves(move.x(), move.y(), visited);
        movesWithCount.append(qMakePair(move, count));
    }

    // 排序：首先按可能移动数升序，然后按位置（行优先，列优先）
    // 这符合Warnsdorff规则：优先选择下一步可走位置最少的格子
    std::sort(movesWithCount.begin(), movesWithCount.end(),
              [](const QPair<QPoint, int> &a, const QPair<QPoint, int> &b) {
                  if (a.second != b.second) {
                      return a.second < b.second;  // 可走位置数少的优先
                  }
                  // 如果可走位置数相等，按位置（行优先，列优先）排序
                  if (a.first.x() != b.first.x()) {
                      return a.first.x() < b.first.x();
                  }
                  return a.first.y() < b.first.y();
              });

    // 尝试每个移动（按Warnsdorff规则排序后的顺序）
    for (const auto &pair : movesWithCount) {
        QPoint move = pair.first;
        visited[move.x()][move.y()] = true;
        path.append(move);

        if (solveKnightTourWarnsdorff(move.x(), move.y(), moveCount + 1, visited, path)) {
            return true;
        }

        // 回溯
        visited[move.x()][move.y()] = false;
        path.removeLast();
    }

    return false;
}

// 改进的Warnsdorff闭合巡游算法
// 改写后的 Warnsdorff 闭合巡游算法，模仿 horse::hores_traversal + sort_j_c_Warnsdorff 逻辑
bool ChessBoardWidget::solveKnightTourWarnsdorffClosed(int row, int col, int moveCount,
                                                       QVector<QVector<bool>> &visited,
                                                       QVector<QPoint> &path,
                                                       const QPoint &startPos)
{
    const int Total_step = BOARD_SIZE * BOARD_SIZE;   // 等价 horse 里的 Total_step

    // 仿 horse::in_grid
    auto in_grid = [&](int x, int y) -> bool {
        return (x >= 0 && x < BOARD_SIZE &&
                y >= 0 && y < BOARD_SIZE &&
                !visited[x][y]);
    };

    // 仿 horse::sort_index + compare，用 std::sort 实现
    auto sort_index = [](const int array[], int index[], int num) {
        // index 初始为 0..num-1
        for (int i = 0; i < num; ++i) {
            index[i] = i;
        }
        // 根据 array[] 的值升序排列 index[]
        std::sort(index, index + num, [&](int a, int b) {
            if (array[a] != array[b]) {
                return array[a] < array[b];
            }
            // 度数相同则按原始顺序（索引小的在前），模仿 qsort 稳定性
            return a < b;
        });
    };

    // 仿 horse::sort_j_c_Warnsdorff
    auto sort_j_c_Warnsdorff = [&](int x, int y, int sortArray[8]) {
        int sorted_by[8];

        for (int i = 0; i < 8; ++i) {
            int next_x1 = x + KNIGHT_MOVES[i][0];
            int next_y1 = y + KNIGHT_MOVES[i][1];

            int step_cnt = 0;
            if (in_grid(next_x1, next_y1)) {
                // 从 (next_x1, next_y1) 再看 8 个方向，可走的数量
                for (int j = 0; j < 8; ++j) {
                    int next_x1_next = next_x1 + KNIGHT_MOVES[j][0];
                    int next_y1_next = next_y1 + KNIGHT_MOVES[j][1];
                    if (in_grid(next_x1_next, next_y1_next)) {
                        ++step_cnt;
                    }
                }
            }
            // 不可走的会保持 step_cnt = 0，但稍后 in_grid 会过滤掉
            sorted_by[i] = step_cnt;
        }

        // 对 0..7 的方向索引按 sorted_by 升序（Warnsdorff 度数小的优先）
        sort_index(sorted_by, sortArray, 8);
    };

    // 仿 horse::hores_traversal 的递归
    std::function<bool(int,int,int)> dfs =
        [&](int x, int y, int deep) -> bool
    {
        // deep：当前已经走到的步数（含当前位置），等价 horse 里的 deep
        if (deep >= Total_step) {
            // 检查最后一步能不能回到起点（go_to_begin == 5）
            for (int i = 0; i < 8; ++i) {
                int next_x1 = x + KNIGHT_MOVES[i][0];
                int next_y1 = y + KNIGHT_MOVES[i][1];

                if (next_x1 == startPos.x() && next_y1 == startPos.y()) {
                    return true;    // 闭合巡游成功
                }
            }
            return false;           // 不能闭合，需要回溯
        }

        // 对 8 个方向做 Warnsdorff 排序
        int j_c_Warnsdorff_sortarray[8];
        sort_j_c_Warnsdorff(x, y, j_c_Warnsdorff_sortarray);

        // 仿 horse::for (i = 0; i < able_step; i++)
        for (int k = 0; k < 8; ++k) {
            int dirIndex = j_c_Warnsdorff_sortarray[k];

            int next_x1 = x + KNIGHT_MOVES[dirIndex][0];
            int next_y1 = y + KNIGHT_MOVES[dirIndex][1];

            if (!in_grid(next_x1, next_y1)) {
                continue;
            }

            // 访问该点
            visited[next_x1][next_y1] = true;
            path.append(QPoint(next_x1, next_y1));

            // deep + 1 继续
            if (dfs(next_x1, next_y1, deep + 1)) {
                return true;    // 找到一条完整闭合路径，向上返回
            }

            // 回溯
            visited[next_x1][next_y1] = false;
            path.removeLast();
        }

        return false;   // 所有方向都试完，失败
    };

    // 入口：startTour 里已经把 startPos 标记 visited 并放进 path，
    // 所以这里从 (row, col) 开始，deep = moveCount
    return dfs(row, col, moveCount);
}

// 从预计算路径中查找闭合巡游
QVector<QPoint> ChessBoardWidget::findClosedTourFromPrecomputed(int startRow, int startCol) const
{
    QVector<QPoint> path;

    // 在预计算路径中查找起点对应的下标
    int startIndex = -1;
    for (int i = 0; i < 64; i++) {
        if (BASE_CLOSED_TOUR[i][0] == startRow && BASE_CLOSED_TOUR[i][1] == startCol) {
            startIndex = i;
            break;
        }
    }

    // 理论上必须能找到；若未找到则返回空路径
    if (startIndex == -1) {
        return path;
    }

    // 从 startIndex 开始，环形遍历 64 个格子，构造路径
    for (int k = 0; k < 64; k++) {
        int idx = (startIndex + k) % 64;
        path.append(QPoint(BASE_CLOSED_TOUR[idx][0], BASE_CLOSED_TOUR[idx][1]));
    }

    // 追加起点一次，表示从最后一步跳回起点，构成闭合巡游
    path.append(QPoint(startRow, startCol));

    return path;
}

// ========== 新增功能实现 ==========

// 绘制悬停指示器（显示是否可闭合巡游）
void ChessBoardWidget::drawHoverIndicator(QPainter &painter)
{
    if (hoveredCell.x() < 0 || hoveredCell.y() < 0) return;

    QRect cellRect = getCellRect(hoveredCell.x(), hoveredCell.y());

    // 检查是否可闭合巡游
    bool canClose = closedTourCache[hoveredCell.x()][hoveredCell.y()];

    // 根据结果设置颜色：绿色=可闭合，红色=不可闭合
    QColor overlayColor = canClose ? QColor(0, 255, 0, 100) : QColor(255, 0, 0, 100);

    painter.setPen(QPen(canClose ? QColor(0, 200, 0) : QColor(200, 0, 0), 3));
    painter.setBrush(QBrush(overlayColor));
    painter.drawRect(cellRect);

    // 绘制提示文字
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(canClose ? QColor(0, 150, 0) : QColor(150, 0, 0));
    QString text = canClose ? tr("可闭合") : tr("不可闭合");
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(text);
    textRect.moveCenter(cellRect.center());
    painter.drawText(textRect, Qt::AlignCenter, text);
}

// 绘制坐标轴
void ChessBoardWidget::drawCoordinates(QPainter &painter)
{
    painter.setPen(QPen(QColor(50, 50, 50), 1));
    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    // 行号（1-8，左侧）
    for (int row = 0; row < BOARD_SIZE; row++) {
        QRect cellRect = getCellRect(row, 0);
        QString num = QString::number(BOARD_SIZE - row);
        painter.drawText(cellRect.left() - 20, cellRect.center().y() + 5, num);
    }

    // 列号（a-h，底部）
    QString letters = "abcdefgh";
    for (int col = 0; col < BOARD_SIZE; col++) {
        QRect cellRect = getCellRect(BOARD_SIZE - 1, col);
        painter.drawText(cellRect.center().x() - 5, cellRect.bottom() + 20, letters[col]);
    }
}

// 绘制Warnsdorff度数
void ChessBoardWidget::drawWarnsdorffDegrees(QPainter &painter)
{
    if (warnsdorffDegrees.isEmpty()) return;

    QFont font = painter.font();
    font.setPointSize(7);
    painter.setFont(font);

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            int degree = warnsdorffDegrees[row][col];
            if (degree >= 0) {
                QRect cellRect = getCellRect(row, col);
                QPointF pos = cellRect.topLeft() + QPointF(5, 15);

                painter.setPen(QColor(100, 100, 255));
                painter.setBrush(QBrush(QColor(200, 200, 255, 150)));
                painter.drawEllipse(pos, 8, 8);

                painter.setPen(QColor(0, 0, 150));
                painter.drawText(QRectF(pos.x() - 5, pos.y() - 5, 10, 10),
                                 Qt::AlignCenter, QString::number(degree));
            }
        }
    }
}

// 更新Warnsdorff度数显示
void ChessBoardWidget::updateWarnsdorffDegrees(int step)
{
    if (step < 0 || step >= tourPath.size()) return;

    // 清空当前度数
    for (int i = 0; i < BOARD_SIZE; i++) {
        warnsdorffDegrees[i].fill(-1);
    }

    // 计算当前步骤的可达位置及其度数
    if (step < tourPath.size()) {
        QPoint current = tourPath[step];
        QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));

        // 标记已访问的位置
        for (int i = 0; i <= step && i < tourPath.size(); i++) {
            visited[tourPath[i].x()][tourPath[i].y()] = true;
        }

        // 计算当前可移动位置的度数
        QVector<QPoint> moves = getValidMoves(current.x(), current.y(), visited);
        for (const QPoint &move : moves) {
            int degree = countPossibleMoves(move.x(), move.y(), visited);
            warnsdorffDegrees[move.x()][move.y()] = degree;
        }
    }
}

// 快速检查闭合巡游（不完整求解，仅做快速判断）
bool ChessBoardWidget::quickCheckClosedTour(int row, int col, bool useWarnsdorff)
{
    Q_UNUSED(useWarnsdorff)

    // 对于8x8棋盘，使用预计算路径，所有位置都可以找到闭合巡游
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }

    // 如果已经在缓存中，直接返回
    if (closedTourCache[row][col]) {
        return true;
    }

    // 使用预计算路径快速检查：查找起点是否在预计算路径中
    // 预计算路径包含了所有64个位置，所以所有位置都可以
    for (int i = 0; i < 64; i++) {
        if (BASE_CLOSED_TOUR[i][0] == row && BASE_CLOSED_TOUR[i][1] == col) {
            // 找到起点，可以构造闭合路径
            closedTourCache[row][col] = true;
            return true;
        }
    }

    // 理论上所有位置都应该在预计算路径中
    // 如果没找到，也返回true（8x8棋盘上所有位置都可以）
    // 并更新缓存
    closedTourCache[row][col] = true;
    return true;
}

// 辅助函数：有限深度的快速检查
bool ChessBoardWidget::quickCheckClosedTourHelper(int row, int col, int moveCount,
                                                  QVector<QVector<bool>> &visited,
                                                  QVector<QPoint> &path,
                                                  bool useWarnsdorff, int maxDepth)
{
    // 如果达到最大深度，认为可行
    if (moveCount >= maxDepth) {
        return true;
    }

    // 获取所有可能的移动
    QVector<QPoint> moves = getValidMoves(row, col, visited);

    if (moves.isEmpty()) {
        return false;
    }

    // 如果使用Warnsdorff规则，对移动进行排序
    if (useWarnsdorff) {
        QVector<QPair<QPoint, int>> movesWithCount;
        for (const QPoint &move : moves) {
            int count = countPossibleMoves(move.x(), move.y(), visited);
            movesWithCount.append(qMakePair(move, count));
        }

        std::sort(movesWithCount.begin(), movesWithCount.end(),
                  [](const QPair<QPoint, int> &a, const QPair<QPoint, int> &b) {
                      if (a.second != b.second) {
                          return a.second < b.second;
                      }
                      if (a.first.x() != b.first.x()) {
                          return a.first.x() < b.first.x();
                      }
                      return a.first.y() < b.first.y();
                  });

        for (const auto &pair : movesWithCount) {
            QPoint move = pair.first;
            visited[move.x()][move.y()] = true;
            path.append(move);

            if (quickCheckClosedTourHelper(move.x(), move.y(), moveCount + 1,
                                           visited, path, useWarnsdorff, maxDepth)) {
                return true;
            }

            visited[move.x()][move.y()] = false;
            path.removeLast();
        }
    } else {
        // 基础回溯法
        for (const QPoint &move : moves) {
            visited[move.x()][move.y()] = true;
            path.append(move);

            if (quickCheckClosedTourHelper(move.x(), move.y(), moveCount + 1,
                                           visited, path, useWarnsdorff, maxDepth)) {
                return true;
            }

            visited[move.x()][move.y()] = false;
            path.removeLast();
        }
    }

    return false;
}

// 检查指定位置是否可闭合巡游
bool ChessBoardWidget::checkClosedTour(int row, int col, bool useWarnsdorff)
{
    return quickCheckClosedTour(row, col, useWarnsdorff);
}

// 自动选择可闭合起点
QPoint ChessBoardWidget::findClosedTourStart(bool useWarnsdorff)
{
    Q_UNUSED(useWarnsdorff)

    // 使用预计算路径，所有位置都可以找到闭合巡游
    // 优先选择中心位置（更美观）
    QVector<QPoint> candidates = {
        QPoint(3, 3), QPoint(3, 4), QPoint(4, 3), QPoint(4, 4),  // 中心4个
        QPoint(2, 2), QPoint(2, 5), QPoint(5, 2), QPoint(5, 5),  // 次中心
        QPoint(1, 1), QPoint(1, 6), QPoint(6, 1), QPoint(6, 6),  // 更外围
        QPoint(0, 0), QPoint(0, 7), QPoint(7, 0), QPoint(7, 7)  // 角落
    };

    // 优先尝试中心位置
    for (const QPoint &candidate : candidates) {
        if (candidate.x() >= 0 && candidate.x() < BOARD_SIZE &&
            candidate.y() >= 0 && candidate.y() < BOARD_SIZE) {
            // 使用预计算路径，所有位置都可以
            return candidate;
        }
    }

    // 如果候选列表有问题，返回中心位置
    return QPoint(3, 3);
}

// 步进控制
void ChessBoardWidget::goToStep(int step)
{
    if (step < 0) step = 0;
    if (step > tourPath.size()) step = tourPath.size();

    currentStep = step;
    if (step > 0 && step <= tourPath.size()) {
        currentKnightPos = tourPath[step - 1];
        animatedKnightPos = currentKnightPos;
    }

    if (showWarnsdorffDegrees) {
        updateWarnsdorffDegrees(step - 1);
    }

    emit stepChanged(currentStep);
    update();
}

void ChessBoardWidget::nextStep()
{
    if (currentStep < tourPath.size()) {
        goToStep(currentStep + 1);
    }
}

void ChessBoardWidget::previousStep()
{
    if (currentStep > 0) {
        goToStep(currentStep - 1);
    }
}

// 设置棋盘主题
void ChessBoardWidget::setBoardTheme(BoardTheme theme)
{
    boardTheme = theme;
    update();
}

// 获取主题颜色
QColor ChessBoardWidget::getLightCellColor() const
{
    switch (boardTheme) {
    case BoardTheme::Light:
        return QColor(240, 217, 181);
    case BoardTheme::Dark:
        return QColor(200, 200, 200);
    case BoardTheme::Wood:
        return QColor(222, 184, 135);
    }
    return QColor(240, 217, 181);
}

QColor ChessBoardWidget::getDarkCellColor() const
{
    switch (boardTheme) {
    case BoardTheme::Light:
        return QColor(181, 136, 99);
    case BoardTheme::Dark:
        return QColor(100, 100, 100);
    case BoardTheme::Wood:
        return QColor(139, 90, 43);
    }
    return QColor(181, 136, 99);
}

// 绘制木纹纹理
void ChessBoardWidget::drawWoodTexture(QPainter &painter, const QRect &rect, bool isLight)
{
    QColor baseColor = isLight ? getLightCellColor() : getDarkCellColor();
    painter.fillRect(rect, baseColor);

    if (boardTheme == BoardTheme::Wood) {
        // 绘制木纹线条
        painter.setPen(QPen(baseColor.darker(110), 1));
        for (int i = 0; i < rect.height(); i += 3) {
            painter.drawLine(rect.left(), rect.top() + i,
                             rect.right(), rect.top() + i);
        }
    }
}

// 动画帧更新
void ChessBoardWidget::onAnimationFrameChanged(int frame)
{
    animState.progress = frame / 100.0;
    update();
}

// 启动跳跃动画
void ChessBoardWidget::startJumpAnimation(const QPoint &from, const QPoint &to)
{
    animState.fromPos = from;
    animState.toPos = to;
    animState.progress = 0.0;
    animState.isAnimating = true;

    jumpAnimation->setDuration(animationSpeed);
    jumpAnimation->start();
}

// 摄像机控制
void ChessBoardWidget::setCameraFollow(bool enabled)
{
    cameraFollowEnabled = enabled;
    update();
}

void ChessBoardWidget::setCameraZoom(double zoom)
{
    cameraZoom = qMax(1.0, qMin(3.0, zoom));  // 限制在1.0-3.0之间
    update();
}

// 设置是否显示Warnsdorff度数
void ChessBoardWidget::setShowWarnsdorffDegrees(bool show)
{
    showWarnsdorffDegrees = show;
    if (show && !tourPath.isEmpty() && currentStep > 0) {
        updateWarnsdorffDegrees(currentStep - 1);
    }
    update();
}

// 渲染到Pixmap（用于导出）
QPixmap ChessBoardWidget::renderToPixmap(int step)
{
    int savedStep = currentStep;
    QPoint savedKnightPos = currentKnightPos;

    // 临时设置步骤
    if (step >= 0 && step <= tourPath.size()) {
        currentStep = step;
        if (step > 0) {
            currentKnightPos = tourPath[step - 1];
        }
    }

    // 创建Pixmap并渲染
    QPixmap pixmap(size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制到Pixmap（复制paintEvent的逻辑，但不应用摄像机变换）
    int boardPixelSize = BOARD_SIZE * cellSize;
    boardOffsetX = (width() - boardPixelSize) / 2;
    boardOffsetY = (height() - boardPixelSize) / 2;

    drawChessBoard(painter);

    if (!tourPath.isEmpty() && currentStep > 0) {
        drawPath(painter);
        drawNumbers(painter);
    }

    if (startPos.x() >= 0 && startPos.y() >= 0) {
        QRect cellRect = getCellRect(startPos.x(), startPos.y());
        painter.setPen(QPen(QColor(255, 0, 0), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(cellRect.adjusted(5, 5, -5, -5));
    }

    drawCoordinates(painter);

    if (currentKnightPos.x() >= 0 && currentKnightPos.y() >= 0) {
        drawKnight(painter, currentKnightPos.x(), currentKnightPos.y());
    }

    // 恢复状态
    currentStep = savedStep;
    currentKnightPos = savedKnightPos;

    return pixmap;
}

// 导出每步PNG
bool ChessBoardWidget::exportStepImages(const QString &basePath)
{
    if (tourPath.isEmpty()) return false;

    QDir dir;
    QString dirPath = QFileInfo(basePath).absolutePath();
    if (!dir.exists(dirPath)) {
        dir.mkpath(dirPath);
    }

    for (int i = 0; i <= tourPath.size(); i++) {
        QPixmap pixmap = renderToPixmap(i);
        QString fileName = QString("%1/step_%2.png").arg(dirPath).arg(i + 1, 3, 10, QChar('0'));

        if (!pixmap.save(fileName, "PNG")) {
            return false;
        }

        emit exportProgress(i + 1, tourPath.size() + 1);
        QApplication::processEvents();  // 处理事件，更新UI
    }

    return true;
}

// 导出GIF动画（需要QMovie或第三方库，这里使用简化版本）
bool ChessBoardWidget::exportGifAnimation(const QString &filePath, int frameDelay)
{
    // 注意：Qt本身不直接支持GIF写入，需要使用第三方库如giflib
    // 这里提供一个框架，实际实现需要链接gif库

    // 简化实现：导出为多帧PNG，用户可以使用外部工具合成GIF
    QFileInfo fileInfo(filePath);
    QString basePath = fileInfo.absolutePath() + "/" + fileInfo.baseName();

    // 导出所有帧
    if (!exportStepImages(basePath)) {
        return false;
    }

    // 提示用户使用外部工具
    QMessageBox::information(nullptr, tr("导出完成"),
                             tr("所有帧已导出为PNG文件。\n"
                                "请使用外部工具（如ImageMagick、FFmpeg）将这些PNG合成为GIF动画。\n"
                                "命令示例：\n"
                                "ffmpeg -framerate %1 -i %2/step_%%03d.png %3")
                                 .arg(1000 / frameDelay).arg(basePath).arg(filePath));

    return true;
}

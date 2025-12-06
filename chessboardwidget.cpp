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

    easingCurve.setType(QEasingCurve::OutCubic);

    animState.isAnimating = false;
    animState.progress = 0.0;

    // 初始缓存：全部 true
    closedTourCache.resize(BOARD_SIZE);
    for (int i = 0; i < BOARD_SIZE; i++) {
        closedTourCache[i].resize(BOARD_SIZE);
        closedTourCache[i].fill(true);
    }

    warnsdorffDegrees.resize(BOARD_SIZE);
    for (int i = 0; i < BOARD_SIZE; i++) {
        warnsdorffDegrees[i].resize(BOARD_SIZE);
        warnsdorffDegrees[i].fill(-1);
    }

    setMinimumSize(520, 520);
    setMouseTracking(true);
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

    QElapsedTimer timer;
    timer.start();

    QVector<QPoint> path;
    bool success = false;
    int attempts = 0;

    if (useWarnsdorff) {
        // ⭐ 使用 horse 原算法
        attempts = 1;
        success = solveKnightTourHorseWrapper(startPos.x(), startPos.y(), path);

        // horse 算法输出 1..64 步，这里补回起点形成闭环动画
        if (success && path.size() == BOARD_SIZE * BOARD_SIZE) {
            path.append(startPos);
        }
    } else {
        // 使用预计算闭环路径
        path = findClosedTourFromPrecomputed(startPos.x(), startPos.y());
        success = !path.isEmpty();
        attempts = 1;
    }

    int elapsedMs = timer.elapsed();

    if (success && path.size() >= BOARD_SIZE * BOARD_SIZE) {

        tourPath = path;
        currentStep = 0;
        currentKnightPos = startPos;
        animatedKnightPos = startPos;
        isTourRunning = true;

        closedTourCache[startPos.x()][startPos.y()] = true;

        lastStats.attempts = attempts;
        lastStats.elapsedMs = elapsedMs;
        lastStats.success = true;
        lastStats.algorithmName = useWarnsdorff
                                      ? tr("Warnsdorff规则（horse算法）")
                                      : tr("基础回溯法（预计算路径）");

        animationTimer->start(animationSpeed);
        update();
    } else {
        lastStats.attempts = attempts;
        lastStats.elapsedMs = elapsedMs;
        lastStats.success = false;
        lastStats.algorithmName = useWarnsdorff
                                      ? tr("Warnsdorff规则（horse算法）")
                                      : tr("基础回溯法（预计算路径）");

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

    if (cameraFollowEnabled && cameraZoom != 1.0) {
        painter.save();

        if (currentKnightPos.x() >= 0 && currentKnightPos.y() >= 0) {
            QRect knightRect = getCellRect(currentKnightPos.x(), currentKnightPos.y());
            QPointF knightCenter = knightRect.center();
            QPointF widgetCenter(width() / 2.0, height() / 2.0);
            cameraOffset = widgetCenter - knightCenter * cameraZoom;
        }

        painter.translate(cameraOffset);
        painter.scale(cameraZoom, cameraZoom);
    }

    int boardPixelSize = BOARD_SIZE * cellSize;
    boardOffsetX = (width() - boardPixelSize) / 2;
    boardOffsetY = (height() - boardPixelSize) / 2;

    drawChessBoard(painter);

    if (hoveredCell.x() >= 0 && hoveredCell.y() >= 0 && isSelectingStart) {
        drawHoverIndicator(painter);
    }

    if (showWarnsdorffDegrees && !tourPath.isEmpty() && currentStep > 0) {
        drawWarnsdorffDegrees(painter);
    }

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

    if (animState.isAnimating) {
        drawKnightAnimated(painter);
    } else if (currentKnightPos.x() >= 0 && currentKnightPos.y() >= 0) {
        drawKnight(painter, currentKnightPos.x(), currentKnightPos.y());
    }

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
    QPoint boardPos = screenToBoard(event->pos());
    if (boardPos.x() >= 0 && boardPos.x() < BOARD_SIZE &&
        boardPos.y() >= 0 && boardPos.y() < BOARD_SIZE) {

        if (hoveredCell != boardPos) {
            hoveredCell = boardPos;
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
    cellSize = (qMin(width(), height()) - 40) / BOARD_SIZE;
    update();
}

void ChessBoardWidget::onAnimationTimer()
{
    if (currentStep < tourPath.size()) {

        if (currentStep > 0) {
            QPoint from = tourPath[currentStep - 1];
            QPoint to = tourPath[currentStep];
            startJumpAnimation(from, to);
        }

        currentKnightPos = tourPath[currentStep];
        animatedKnightPos = currentKnightPos;
        currentStep++;

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

            painter.setPen(QPen(QColor(100, 100, 100), 1));
            painter.drawRect(cellRect);
        }
    }
}

void ChessBoardWidget::drawKnight(QPainter &painter, int row, int col)
{
    QRect cellRect = getCellRect(row, col);
    QPoint center = cellRect.center();

    painter.save();

    QFont font = painter.font();
    int fontSize = cellSize * 0.7;
    font.setPointSize(fontSize);
    font.setBold(true);
    painter.setFont(font);

    bool isLight = (row + col) % 2 == 0;
    painter.setPen(isLight ? QColor(0, 0, 0) : QColor(255, 255, 255));

    QString knightChar = QString::fromUtf8("♞");
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(knightChar);
    textRect.moveCenter(center);

    painter.drawText(textRect, Qt::AlignCenter, knightChar);

    painter.restore();
}

void ChessBoardWidget::drawKnightAnimated(QPainter &painter)
{
    if (!animState.isAnimating) {
        if (animatedKnightPos.x() >= 0 && animatedKnightPos.y() >= 0) {
            drawKnight(painter, animatedKnightPos.x(), animatedKnightPos.y());
        }
        return;
    }

    double easedProgress = easingCurve.valueForProgress(animState.progress);

    QRect fromRect = getCellRect(animState.fromPos.x(), animState.fromPos.y());
    QRect toRect = getCellRect(animState.toPos.x(), animState.toPos.y());

    QPointF fromCenter = fromRect.center();
    QPointF toCenter   = toRect.center();

    QPointF currentPos = fromCenter + (toCenter - fromCenter) * easedProgress;
    double jumpHeight = 30.0 * sin(easedProgress * M_PI);
    currentPos.setY(currentPos.y() - jumpHeight);

    painter.save();
    painter.translate(currentPos);

    double scale = 1.0 + 0.2 * sin(easedProgress * M_PI);
    painter.scale(scale, scale);

    QFont font = painter.font();
    font.setPointSize(cellSize * 0.7 * scale);
    font.setBold(true);
    painter.setFont(font);

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

    painter.setPen(QPen(QColor(0, 150, 255), 3));
    painter.setBrush(Qt::NoBrush);

    for (int i = 0; i < currentStep - 1; i++) {
        QPoint p1 = tourPath[i];
        QPoint p2 = tourPath[i + 1];

        QRect rect1 = getCellRect(p1.x(), p1.y());
        QRect rect2 = getCellRect(p2.x(), p2.y());

        QPointF center1 = rect1.center();
        QPointF center2 = rect2.center();

        QPointF controlPoint = (center1 + center2) / 2;
        controlPoint.setY(qMin(center1.y(), center2.y()) - 20);

        QPainterPath arcPath;
        arcPath.moveTo(center1);
        arcPath.quadTo(controlPoint, center2);

        painter.drawPath(arcPath);

        if (i == currentStep - 2) {
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

        bool isLight = (pos.x() + pos.y()) % 2 == 0;
        painter.setPen(isLight ? QColor(0, 0, 0) : QColor(255, 255, 255));

        QString number = QString::number(i + 1);
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(number);
        textRect.moveCenter(cellRect.center());

        painter.setBrush(QBrush(QColor(255, 255, 0, 180)));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(textRect.adjusted(-3, -3, 3, 3));

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

QVector<QPoint> ChessBoardWidget::getValidMoves(int row, int col,
                                                const QVector<QVector<bool>> &visited) const
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

QVector<QPoint> ChessBoardWidget::findClosedTourFromPrecomputed(int startRow, int startCol) const
{
    QVector<QPoint> path;
    int startIndex = -1;

    for (int i = 0; i < 64; i++) {
        if (BASE_CLOSED_TOUR[i][0] == startRow &&
            BASE_CLOSED_TOUR[i][1] == startCol) {
            startIndex = i;
            break;
        }
    }

    if (startIndex == -1)
        return path;

    for (int k = 0; k < 64; k++) {
        int idx = (startIndex + k) % 64;
        path.append(QPoint(BASE_CLOSED_TOUR[idx][0], BASE_CLOSED_TOUR[idx][1]));
    }

    path.append(QPoint(startRow, startCol));
    return path;
}

void ChessBoardWidget::drawHoverIndicator(QPainter &painter)
{
    if (hoveredCell.x() < 0 || hoveredCell.y() < 0)
        return;

    QRect cellRect = getCellRect(hoveredCell.x(), hoveredCell.y());
    bool canClose = closedTourCache[hoveredCell.x()][hoveredCell.y()];

    QColor overlayColor = canClose ? QColor(0, 255, 0, 100)
                                   : QColor(255, 0, 0, 100);

    painter.setPen(QPen(canClose ? QColor(0, 200, 0)
                                 : QColor(200, 0, 0), 3));
    painter.setBrush(QBrush(overlayColor));
    painter.drawRect(cellRect);

    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    QString text = canClose ? tr("可闭合") : tr("不可闭合");
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(text);
    textRect.moveCenter(cellRect.center());

    painter.drawText(textRect, Qt::AlignCenter, text);
}

void ChessBoardWidget::drawCoordinates(QPainter &painter)
{
    painter.setPen(QPen(QColor(50, 50, 50), 1));

    QFont font = painter.font();
    font.setPointSize(10);
    font.setBold(true);
    painter.setFont(font);

    for (int row = 0; row < BOARD_SIZE; row++) {
        QRect cellRect = getCellRect(row, 0);
        QString num = QString::number(BOARD_SIZE - row);
        painter.drawText(cellRect.left() - 20, cellRect.center().y() + 5, num);
    }

    QString letters = "abcdefgh";
    for (int col = 0; col < BOARD_SIZE; col++) {
        QRect cellRect = getCellRect(BOARD_SIZE - 1, col);
        painter.drawText(cellRect.center().x() - 5,
                         cellRect.bottom() + 20,
                         letters[col]);
    }
}

void ChessBoardWidget::drawWarnsdorffDegrees(QPainter &painter)
{
    if (warnsdorffDegrees.isEmpty())
        return;

    QFont font = painter.font();
    font.setPointSize(7);
    painter.setFont(font);

    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {

            int degree = warnsdorffDegrees[row][col];
            if (degree < 0)
                continue;

            QRect cellRect = getCellRect(row, col);
            QPointF pos = cellRect.topLeft() + QPointF(5, 15);

            painter.setPen(QColor(100, 100, 255));
            painter.setBrush(QBrush(QColor(200, 200, 255, 150)));
            painter.drawEllipse(pos, 8, 8);

            painter.setPen(QColor(0, 0, 150));
            painter.drawText(QRectF(pos.x() - 5, pos.y() - 5, 10, 10),
                             Qt::AlignCenter,
                             QString::number(degree));
        }
    }
}

void ChessBoardWidget::updateWarnsdorffDegrees(int step)
{
    if (step < 0 || step >= tourPath.size())
        return;

    for (int i = 0; i < BOARD_SIZE; i++)
        warnsdorffDegrees[i].fill(-1);

    if (step < tourPath.size()) {
        QPoint current = tourPath[step];
        QVector<QVector<bool>> visited(BOARD_SIZE, QVector<bool>(BOARD_SIZE, false));

        for (int i = 0; i <= step && i < tourPath.size(); i++)
            visited[tourPath[i].x()][tourPath[i].y()] = true;

        QVector<QPoint> moves = getValidMoves(current.x(), current.y(), visited);

        for (const QPoint &move : moves) {
            int degree = countPossibleMoves(move.x(), move.y(), visited);
            warnsdorffDegrees[move.x()][move.y()] = degree;
        }
    }
}

// ⭐ 快速判断闭合：不做任何搜索（永远 true）
bool ChessBoardWidget::quickCheckClosedTour(int row, int col, bool useWarnsdorff)
{
    Q_UNUSED(useWarnsdorff);

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return false;

    return true;
}

bool ChessBoardWidget::checkClosedTour(int row, int col, bool useWarnsdorff)
{
    return quickCheckClosedTour(row, col, useWarnsdorff);
}

QPoint ChessBoardWidget::findClosedTourStart(bool useWarnsdorff)
{
    Q_UNUSED(useWarnsdorff)

    QVector<QPoint> candidates = {
        QPoint(3, 3), QPoint(3, 4), QPoint(4, 3), QPoint(4, 4),
        QPoint(2, 2), QPoint(2, 5), QPoint(5, 2), QPoint(5, 5),
        QPoint(1, 1), QPoint(1, 6), QPoint(6, 1), QPoint(6, 6),
        QPoint(0, 0), QPoint(0, 7), QPoint(7, 0), QPoint(7, 7)
    };

    for (const QPoint &c : candidates)
        return c;

    return QPoint(3, 3);
}
void ChessBoardWidget::goToStep(int step)
{
    if (step < 0) step = 0;
    if (step > tourPath.size()) step = tourPath.size();

    currentStep = step;

    if (step > 0 && step <= tourPath.size()) {
        currentKnightPos = tourPath[step - 1];
        animatedKnightPos = currentKnightPos;
    }

    if (showWarnsdorffDegrees)
        updateWarnsdorffDegrees(step - 1);

    emit stepChanged(currentStep);
    update();
}

void ChessBoardWidget::nextStep()
{
    if (currentStep < tourPath.size())
        goToStep(currentStep + 1);
}

void ChessBoardWidget::previousStep()
{
    if (currentStep > 0)
        goToStep(currentStep - 1);
}

void ChessBoardWidget::setBoardTheme(BoardTheme theme)
{
    boardTheme = theme;
    update();
}

QColor ChessBoardWidget::getLightCellColor() const
{
    switch (boardTheme) {
    case BoardTheme::Light:
        return QColor(240,217,181);
    case BoardTheme::Dark:
        return QColor(200,200,200);
    case BoardTheme::Wood:
        return QColor(222,184,135);
    }
    return QColor(255,255,255);
}

QColor ChessBoardWidget::getDarkCellColor() const
{
    switch (boardTheme) {
    case BoardTheme::Light:
        return QColor(181,136,99);
    case BoardTheme::Dark:
        return QColor(100,100,100);
    case BoardTheme::Wood:
        return QColor(139,90,43);
    }
    return QColor(0,0,0);
}

void ChessBoardWidget::drawWoodTexture(QPainter &painter, const QRect &rect, bool isLight)
{
    QColor base = isLight ? getLightCellColor() : getDarkCellColor();
    painter.fillRect(rect, base);

    painter.setPen(QPen(base.darker(110), 1));
    for (int i = 0; i < rect.height(); i += 3)
        painter.drawLine(rect.left(), rect.top() + i, rect.right(), rect.top() + i);
}

void ChessBoardWidget::onAnimationFrameChanged(int frame)
{
    animState.progress = frame / 100.0;
    update();
}

void ChessBoardWidget::startJumpAnimation(const QPoint &from, const QPoint &to)
{
    animState.fromPos = from;
    animState.toPos = to;
    animState.progress = 0.0;
    animState.isAnimating = true;

    jumpAnimation->setDuration(animationSpeed);
    jumpAnimation->start();
}

//////////////////////////////////////////////////////////////////////////
// 导出 PNG
//////////////////////////////////////////////////////////////////////////

QPixmap ChessBoardWidget::renderToPixmap(int step)
{
    int savedStep = currentStep;
    QPoint savedPos = currentKnightPos;

    if (step >= 0 && step <= tourPath.size()) {
        currentStep = step;
        if (step > 0)
            currentKnightPos = tourPath[step - 1];
    }

    QPixmap pix(size());
    pix.fill(Qt::white);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    int boardPixelSize = BOARD_SIZE * cellSize;
    boardOffsetX = (width() - boardPixelSize) / 2;
    boardOffsetY = (height() - boardPixelSize) / 2;

    drawChessBoard(painter);

    if (!tourPath.isEmpty() && currentStep > 0) {
        drawPath(painter);
        drawNumbers(painter);
    }

    if (startPos.x() >= 0) {
        QRect rect = getCellRect(startPos.x(), startPos.y());
        painter.setPen(QPen(QColor(255,0,0), 3));
        painter.drawEllipse(rect.adjusted(5,5,-5,-5));
    }

    drawCoordinates(painter);

    if (currentKnightPos.x() >= 0)
        drawKnight(painter, currentKnightPos.x(), currentKnightPos.y());

    currentStep = savedStep;
    currentKnightPos = savedPos;
    return pix;
}

bool ChessBoardWidget::exportStepImages(const QString &basePath)
{
    if (tourPath.isEmpty())
        return false;

    QDir dir;
    QString dirPath = QFileInfo(basePath).absolutePath();
    if (!dir.exists(dirPath))
        dir.mkpath(dirPath);

    for (int i = 0; i <= tourPath.size(); i++) {
        QPixmap pixmap = renderToPixmap(i);
        QString fileName = QString("%1/step_%2.png")
                               .arg(dirPath)
                               .arg(i + 1, 3, 10, QChar('0'));

        pixmap.save(fileName, "PNG");
        emit exportProgress(i + 1, tourPath.size() + 1);
        QApplication::processEvents();
    }

    return true;
}

bool ChessBoardWidget::exportGifAnimation(const QString &filePath, int frameDelay)
{
    QFileInfo info(filePath);
    QString base = info.absolutePath() + "/" + info.baseName();

    if (!exportStepImages(base))
        return false;

    QMessageBox::information(
        nullptr,
        tr("导出完成"),
        tr("PNG 已导出。\n请使用 ffmpeg/ImageMagick 合成 GIF：\n\n"
           "ffmpeg -framerate %1 -i %2/step_%%03d.png %3")
            .arg(1000 / frameDelay)
            .arg(base)
            .arg(filePath)
        );

    return true;
}

//////////////////////////////////////////////////////////////////////////
// ⭐ 关键：唯一的求解算法 —— 使用 horse.cpp
//////////////////////////////////////////////////////////////////////////

bool ChessBoardWidget::solveKnightTourHorseWrapper(int row, int col, QVector<QPoint> &path)
{
    // ----------------------------------------------------
    //  1)  Qt(row,col) → horse(x,y)  (全部使用 0-based，且不再做上下翻转)
    // ----------------------------------------------------
    // Qt: row 0 在上方，7 在下方；col 0 在左，7 在右
    // horse: y 作为行号（0..7，从上到下），x 作为列号（0..7，从左到右）
    int startX = col;  // 列 → x
    int startY = row;  // 行 → y

    // 这里我们要求闭合巡游：最后一步可以一步跳回起点
    horse::setClosedTour(true);

    bool ok = horse::solve(startX, startY);
    if (!ok)
        return false;

    // ----------------------------------------------------
    //  2) horse.grid(x,y) → Qt(row,col)
    // ----------------------------------------------------
    path.clear();
    // Pre-size the vector to avoid multiple reallocations.
    // The path should contain exactly Total_step points.
    path.resize(Total_step); 

    // Iterate through the solved grid once to build the path.
    // This is much more efficient than the previous O(n^2) loop.
    for (int x = 0; x < size_x; ++x) {
        for (int y = 0; y < size_y; ++y) {
            int step = horse::grid[x][y];
            // Ensure the step number is valid (1 to 64)
            if (step >= 1 && step <= Total_step) {
                // Place the coordinate at the correct position in the path vector.
                // (step-1 because vector is 0-indexed, steps are 1-indexed)
                path[step - 1] = QPoint(y, x); // y is row, x is col
            }
        }
    }

    return true;
}

void ChessBoardWidget::setCameraFollow(bool enabled)
{
    cameraFollowEnabled = enabled;
    update();
}

void ChessBoardWidget::setCameraZoom(double zoom)
{
    cameraZoom = zoom;
    update();
}

void ChessBoardWidget::setShowWarnsdorffDegrees(bool show)
{
    showWarnsdorffDegrees = show;
    update();
}

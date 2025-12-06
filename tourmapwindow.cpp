#include "tourmapwindow.h"
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

TourMapWindow::TourMapWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("巡游路径地图"));
    setMinimumSize(300, 500);
    resize(350, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 标题
    titleLabel = new QLabel(tr("巡游路径步骤列表"), this);
    titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    // 路径列表
    pathList = new QListWidget(this);
    pathList->setAlternatingRowColors(true);
    mainLayout->addWidget(pathList, 1);

    // 关闭按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    closeButton = new QPushButton(tr("关闭"), this);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(pathList, &QListWidget::itemClicked, this, &TourMapWindow::onItemClicked);
    connect(closeButton, &QPushButton::clicked, this, &TourMapWindow::onCloseClicked);
}

void TourMapWindow::setTourPath(const QVector<QPoint> &path)
{
    tourPath = path;
    pathList->clear();

    QString letters = "abcdefgh";

    for (int i = 0; i < path.size(); i++) {
        const QPoint &pos = path[i];
        QString stepText = formatStep(i + 1, pos);
        QListWidgetItem *item = new QListWidgetItem(stepText, pathList);
        item->setData(Qt::UserRole, i + 1);  // 存储步骤编号（从1开始）
        pathList->addItem(item);
    }

    titleLabel->setText(tr("巡游路径步骤列表 (共 %1 步)").arg(path.size()));
}

void TourMapWindow::highlightStep(int step)
{
    // 清除之前的高亮
    for (int i = 0; i < pathList->count(); i++) {
        QListWidgetItem *item = pathList->item(i);
        if (item) {
            item->setBackground(QBrush());
            item->setForeground(QBrush());
        }
    }

    // 高亮当前步骤
    if (step > 0 && step <= pathList->count()) {
        QListWidgetItem *item = pathList->item(step - 1);
        if (item) {
            item->setBackground(QBrush(QColor(255, 255, 0, 150)));  // 黄色背景
            item->setForeground(QBrush(QColor(0, 0, 0)));  // 黑色文字
            pathList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        }
    }
}

QString TourMapWindow::formatStep(int step, const QPoint &pos) const
{
    QString letters = "abcdefgh";
    QString colLetter = letters[pos.y()];
    int rowNumber = 8 - pos.x();

    return QString("%1: %2%3 (%4, %5)")
        .arg(step, 3, 10, QChar('0'))
        .arg(colLetter)
        .arg(rowNumber)
        .arg(pos.x() + 1)
        .arg(pos.y() + 1);
}

void TourMapWindow::onItemClicked(QListWidgetItem *item)
{
    if (item) {
        int step = item->data(Qt::UserRole).toInt();
        emit stepSelected(step);
        highlightStep(step);
    }
}

void TourMapWindow::onCloseClicked()
{
    hide();
}

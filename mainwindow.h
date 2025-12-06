#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QSlider>
#include <QTextEdit>
#include <QSplitter>
#include <QListWidget>
#include "chessboardwidget.h"

// 前向声明
class TourMapWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onResetClicked();
    void onTourCompleted();
    void onTourFailed();
    void onSpeedChanged(int value);
    void onAlgorithmChanged(int index);
    void onNextStepClicked();
    void onPreviousStepClicked();
    void onStepSliderChanged(int value);
    void onAutoSelectStartClicked();
    void onOpenMapClicked();
    void onThemeChanged(QAction *action);
    void onShowWarnsdorffToggled(bool checked);
    void onStepChanged(int step);
    void onHoveredCellChanged(int row, int col, bool canClose);

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupInfoPanel();  // 设置信息面板
    void updateInfoPanel();  // 更新信息面板

    ChessBoardWidget *chessBoard;

    // 控制按钮
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *resetButton;
    QPushButton *nextStepButton;
    QPushButton *previousStepButton;
    QPushButton *autoSelectButton;
    QPushButton *openMapButton;

    // 控制组件
    QComboBox *algorithmCombo;
    QSpinBox *speedSpinBox;
    QSlider *stepSlider;
    QLabel *stepLabel;

    // 信息面板
    QTextEdit *infoPanel;
    QLabel *startPosLabel;
    QLabel *algorithmLabel;
    QLabel *statsLabel;

    // 状态
    QLabel *statusLabel;

    // 路径地图窗口
    TourMapWindow *tourMapWindow;

    // 布局
    QSplitter *mainSplitter;
};

#endif // MAINWINDOW_H

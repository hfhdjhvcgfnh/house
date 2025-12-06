#include "mainwindow.h"
#include "tourmapwindow.h"
#include <QMessageBox>
#include <QKeySequence>
#include <QAction>
#include <QFileDialog>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , tourMapWindow(nullptr)
{
    setupUI();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupInfoPanel();

    setWindowTitle(tr("骑士巡游 - Knight's Tour"));
    setMinimumSize(900, 750);
    resize(1000, 800);
}

MainWindow::~MainWindow()
{
    if (tourMapWindow) {
        delete tourMapWindow;
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 使用分割器布局：左侧棋盘，右侧信息面板
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(mainSplitter);

    // 左侧：棋盘和控制面板
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    // 创建棋盘
    chessBoard = new ChessBoardWidget(this);
    leftLayout->addWidget(chessBoard, 1);

    // 控制面板
    QGroupBox *controlGroup = new QGroupBox(tr("控制面板"), this);
    QVBoxLayout *controlMainLayout = new QVBoxLayout(controlGroup);

    // 第一行：算法和速度
    QHBoxLayout *controlRow1 = new QHBoxLayout();
    QLabel *algorithmLabel = new QLabel(tr("算法:"), this);
    algorithmCombo = new QComboBox(this);
    algorithmCombo->addItem(tr("基础回溯法"));
    algorithmCombo->addItem(tr("Warnsdorff规则优化"));
    algorithmCombo->setCurrentIndex(1);

    QLabel *speedLabel = new QLabel(tr("演示速度(ms):"), this);
    speedSpinBox = new QSpinBox(this);
    speedSpinBox->setRange(50, 2000);
    speedSpinBox->setValue(300);
    speedSpinBox->setSingleStep(50);
    speedSpinBox->setSuffix(" ms");

    controlRow1->addWidget(algorithmLabel);
    controlRow1->addWidget(algorithmCombo);
    controlRow1->addStretch();
    controlRow1->addWidget(speedLabel);
    controlRow1->addWidget(speedSpinBox);

    // 第二行：步进控制
    QHBoxLayout *controlRow2 = new QHBoxLayout();
    previousStepButton = new QPushButton(tr("上一步"), this);
    nextStepButton = new QPushButton(tr("下一步"), this);
    stepLabel = new QLabel(tr("步骤: 0/0"), this);
    stepSlider = new QSlider(Qt::Horizontal, this);
    stepSlider->setMinimum(0);
    stepSlider->setMaximum(0);
    stepSlider->setValue(0);
    stepSlider->setEnabled(false);

    controlRow2->addWidget(previousStepButton);
    controlRow2->addWidget(nextStepButton);
    controlRow2->addWidget(stepLabel);
    controlRow2->addWidget(stepSlider, 1);

    // 第三行：主要按钮
    QHBoxLayout *controlRow3 = new QHBoxLayout();
    startButton = new QPushButton(tr("开始"), this);
    startButton->setMinimumWidth(80);
    stopButton = new QPushButton(tr("停止"), this);
    stopButton->setMinimumWidth(80);
    stopButton->setEnabled(false);
    resetButton = new QPushButton(tr("重置"), this);
    resetButton->setMinimumWidth(80);
    autoSelectButton = new QPushButton(tr("自动选取起点"), this);
    openMapButton = new QPushButton(tr("打开路径地图"), this);
    openMapButton->setEnabled(false);

    controlRow3->addWidget(startButton);
    controlRow3->addWidget(stopButton);
    controlRow3->addWidget(resetButton);
    controlRow3->addStretch();
    controlRow3->addWidget(autoSelectButton);
    controlRow3->addWidget(openMapButton);

    controlMainLayout->addLayout(controlRow1);
    controlMainLayout->addLayout(controlRow2);
    controlMainLayout->addLayout(controlRow3);

    leftLayout->addWidget(controlGroup);
    leftLayout->setStretchFactor(chessBoard, 1);

    // 右侧：信息面板
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(10, 10, 10, 10);

    QGroupBox *infoGroup = new QGroupBox(tr("算法解说"), this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    startPosLabel = new QLabel(tr("起点坐标: 未选择"), this);
    algorithmLabel = new QLabel(tr("当前算法: Warnsdorff规则优化"), this);
    statsLabel = new QLabel(tr("尝试次数: -\n生成耗时: - ms"), this);

    infoLayout->addWidget(startPosLabel);
    infoLayout->addWidget(algorithmLabel);
    infoLayout->addWidget(statsLabel);
    infoLayout->addStretch();
    infoGroup->setLayout(infoLayout);

    rightLayout->addWidget(infoGroup);

    // 添加到分割器
    mainSplitter->addWidget(leftWidget);
    mainSplitter->addWidget(rightWidget);
    mainSplitter->setStretchFactor(0, 3);  // 左侧占3份
    mainSplitter->setStretchFactor(1, 1);  // 右侧占1份

    // 连接信号和槽
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(nextStepButton, &QPushButton::clicked, this, &MainWindow::onNextStepClicked);
    connect(previousStepButton, &QPushButton::clicked, this, &MainWindow::onPreviousStepClicked);
    connect(stepSlider, &QSlider::valueChanged, this, &MainWindow::onStepSliderChanged);
    connect(autoSelectButton, &QPushButton::clicked, this, &MainWindow::onAutoSelectStartClicked);
    connect(openMapButton, &QPushButton::clicked, this, &MainWindow::onOpenMapClicked);

    connect(chessBoard, &ChessBoardWidget::tourCompleted, this, &MainWindow::onTourCompleted);
    connect(chessBoard, &ChessBoardWidget::tourFailed, this, &MainWindow::onTourFailed);
    connect(chessBoard, &ChessBoardWidget::stepChanged, this, &MainWindow::onStepChanged);
    connect(chessBoard, &ChessBoardWidget::hoveredCellChanged, this, &MainWindow::onHoveredCellChanged);

    connect(speedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onSpeedChanged);
    connect(algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAlgorithmChanged);
}

void MainWindow::setupMenuBar()
{
    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu(tr("文件(&F)"));

    QAction *exportPngAction = fileMenu->addAction(tr("导出PNG序列(&P)"));
    connect(exportPngAction, &QAction::triggered, this, [this]() {
        if (chessBoard->getTourPath().isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("请先完成一次巡游！"));
            return;
        }

        QString dir = QFileDialog::getExistingDirectory(this, tr("选择导出目录"));
        if (!dir.isEmpty()) {
            QString basePath = dir + "/knight_tour";
            if (chessBoard->exportStepImages(basePath)) {
                QMessageBox::information(this, tr("成功"),
                                         tr("PNG序列已导出到：\n%1").arg(dir));
            } else {
                QMessageBox::warning(this, tr("失败"), tr("导出失败！"));
            }
        }
    });

    QAction *exportGifAction = fileMenu->addAction(tr("导出GIF动画(&G)"));
    connect(exportGifAction, &QAction::triggered, this, [this]() {
        if (chessBoard->getTourPath().isEmpty()) {
            QMessageBox::warning(this, tr("警告"), tr("请先完成一次巡游！"));
            return;
        }

        QString filePath = QFileDialog::getSaveFileName(this, tr("保存GIF"),
                                                        "", tr("GIF Files (*.gif)"));
        if (!filePath.isEmpty()) {
            chessBoard->exportGifAnimation(filePath, 100);
        }
    });

    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction(tr("退出(&X)"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // 视图菜单
    QMenu *viewMenu = menuBar()->addMenu(tr("视图(&V)"));

    QMenu *themeMenu = viewMenu->addMenu(tr("棋盘主题"));
    QAction *lightThemeAction = themeMenu->addAction(tr("亮色主题"));
    QAction *darkThemeAction = themeMenu->addAction(tr("深色主题"));
    QAction *woodThemeAction = themeMenu->addAction(tr("古典木纹"));

    connect(lightThemeAction, &QAction::triggered, this, [this]() {
        chessBoard->setBoardTheme(BoardTheme::Light);
    });
    connect(darkThemeAction, &QAction::triggered, this, [this]() {
        chessBoard->setBoardTheme(BoardTheme::Dark);
    });
    connect(woodThemeAction, &QAction::triggered, this, [this]() {
        chessBoard->setBoardTheme(BoardTheme::Wood);
    });

    viewMenu->addSeparator();
    QAction *showWarnsdorffAction = viewMenu->addAction(tr("显示Warnsdorff度数"));
    showWarnsdorffAction->setCheckable(true);
    connect(showWarnsdorffAction, &QAction::toggled, this, &MainWindow::onShowWarnsdorffToggled);

    viewMenu->addSeparator();
    QAction *cameraFollowAction = viewMenu->addAction(tr("摄像机跟随"));
    cameraFollowAction->setCheckable(true);
    connect(cameraFollowAction, &QAction::toggled, this, [this](bool checked) {
        chessBoard->setCameraFollow(checked);
    });

    QMenu *zoomMenu = viewMenu->addMenu(tr("缩放"));
    QAction *zoom1xAction = zoomMenu->addAction(tr("1x (正常)"));
    QAction *zoom2xAction = zoomMenu->addAction(tr("2x (放大)"));
    QAction *zoom3xAction = zoomMenu->addAction(tr("3x (最大)"));

    connect(zoom1xAction, &QAction::triggered, this, [this]() {
        chessBoard->setCameraZoom(1.0);
    });
    connect(zoom2xAction, &QAction::triggered, this, [this]() {
        chessBoard->setCameraZoom(2.0);
    });
    connect(zoom3xAction, &QAction::triggered, this, [this]() {
        chessBoard->setCameraZoom(3.0);
    });

    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    QAction *aboutAction = helpMenu->addAction(tr("关于(&A)"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("关于"),
                           tr("<h2>骑士巡游 - Knight's Tour</h2>"
                              "<p>这是一个国际象棋骑士巡游演示程序。</p>"
                              "<p><b>使用方法：</b></p>"
                              "<ul>"
                              "<li>在棋盘上点击选择骑士的起始位置（或使用自动选取）</li>"
                              "<li>选择算法（基础回溯法或Warnsdorff规则优化）</li>"
                              "<li>调整演示速度</li>"
                              "<li>点击\"开始\"按钮开始演示</li>"
                              "<li>使用步进控制可以手动查看每一步</li>"
                              "</ul>"
                              "<p><b>功能特性：</b></p>"
                              "<ul>"
                              "<li>鼠标悬停显示格子是否可闭合巡游</li>"
                              "<li>Warnsdorff规则可视化</li>"
                              "<li>弧形跳跃动画</li>"
                              "<li>多种棋盘主题</li>"
                              "<li>PNG/GIF导出</li>"
                              "<li>摄像机跟随效果</li>"
                              "</ul>"));
    });
}

void MainWindow::setupToolBar()
{
    // 工具栏暂时为空，可以后续添加
}

void MainWindow::setupStatusBar()
{
    statusLabel = new QLabel(tr("请点击棋盘选择起始位置（或使用自动选取）"), this);
    statusBar()->addWidget(statusLabel);
}

void MainWindow::setupInfoPanel()
{
    // 信息面板已在setupUI中创建
    // 延迟更新，确保所有控件都已初始化
    QTimer::singleShot(0, this, [this]() {
        updateInfoPanel();
    });
}

void MainWindow::updateInfoPanel()
{
    // 安全检查
    if (!chessBoard || !startPosLabel || !statsLabel || !algorithmLabel || !algorithmCombo) {
        return;
    }

    QVector<QPoint> tourPath = chessBoard->getTourPath();
    if (tourPath.isEmpty()) {
        startPosLabel->setText(tr("起点坐标: 未选择"));
        statsLabel->setText(tr("尝试次数: -\n生成耗时: - ms"));
    } else {
        QPoint startPos = tourPath.first();
        QString letters = "abcdefgh";
        if (startPos.y() >= 0 && startPos.y() < letters.length() &&
            startPos.x() >= 0 && startPos.x() < 8) {
            QString posStr = QString("%1%2").arg(letters[startPos.y()]).arg(8 - startPos.x());
            startPosLabel->setText(tr("起点坐标: %1 (%2, %3)")
                                       .arg(posStr, QString::number(startPos.x() + 1), QString::number(startPos.y() + 1)));
        }

        auto stats = chessBoard->getLastStats();
        statsLabel->setText(tr("尝试次数: %1\n生成耗时: %2 ms")
                                .arg(stats.attempts, stats.elapsedMs));
    }

    bool useWarnsdorff = (algorithmCombo->currentIndex() == 1);
    algorithmLabel->setText(tr("当前算法: %1")
                                .arg(useWarnsdorff ? tr("Warnsdorff规则优化") : tr("基础回溯法")));
}

void MainWindow::onStartClicked()
{
    bool useWarnsdorff = (algorithmCombo->currentIndex() == 1);
    chessBoard->startTour(useWarnsdorff);

    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    resetButton->setEnabled(false);
    algorithmCombo->setEnabled(false);
    nextStepButton->setEnabled(false);
    previousStepButton->setEnabled(false);
    stepSlider->setEnabled(false);

    statusLabel->setText(tr("正在计算路径..."));
    updateInfoPanel();
}

void MainWindow::onStopClicked()
{
    chessBoard->stopTour();

    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    resetButton->setEnabled(true);
    algorithmCombo->setEnabled(true);

    // 如果有路径，启用步进控制
    if (!chessBoard->getTourPath().isEmpty()) {
        nextStepButton->setEnabled(true);
        previousStepButton->setEnabled(true);
        stepSlider->setEnabled(true);
        openMapButton->setEnabled(true);
    }

    statusLabel->setText(tr("演示已停止"));
}

void MainWindow::onResetClicked()
{
    chessBoard->reset();

    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    resetButton->setEnabled(true);
    algorithmCombo->setEnabled(true);
    nextStepButton->setEnabled(false);
    previousStepButton->setEnabled(false);
    stepSlider->setEnabled(false);
    stepSlider->setMaximum(0);
    stepLabel->setText(tr("步骤: 0/0"));
    openMapButton->setEnabled(false);

    statusLabel->setText(tr("请点击棋盘选择起始位置（或使用自动选取）"));
    updateInfoPanel();
}

void MainWindow::onTourCompleted()
{
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    resetButton->setEnabled(true);
    algorithmCombo->setEnabled(true);
    nextStepButton->setEnabled(true);
    previousStepButton->setEnabled(true);
    stepSlider->setEnabled(true);
    openMapButton->setEnabled(true);

    // 更新步进控制
    int totalSteps = chessBoard->getTotalSteps();
    stepSlider->setMaximum(totalSteps);
    stepSlider->setValue(totalSteps);
    stepLabel->setText(tr("步骤: %1/%2").arg(totalSteps, totalSteps));

    statusLabel->setText(tr("巡游完成！骑士已访问所有64个格子并回到起点。"));
    updateInfoPanel();

    // 询问是否导出
    int ret = QMessageBox::question(this, tr("完成"),
                                    tr("骑士巡游成功完成！\n"
                                       "骑士已访问所有64个格子并回到起点。\n\n"
                                       "是否导出PNG序列？"),
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        QString dir = QFileDialog::getExistingDirectory(this, tr("选择导出目录"));
        if (!dir.isEmpty()) {
            QString basePath = dir + "/knight_tour";
            chessBoard->exportStepImages(basePath);
        }
    }
}

void MainWindow::onTourFailed()
{
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    resetButton->setEnabled(true);
    algorithmCombo->setEnabled(true);

    statusLabel->setText(tr("求解失败，请尝试其他起始位置或算法。"));
    updateInfoPanel();

    QMessageBox::warning(this, tr("失败"),
                         tr("无法找到完整的巡游路径。\n"
                            "请尝试：\n"
                            "1. 选择其他起始位置\n"
                            "2. 使用Warnsdorff规则优化算法\n"
                            "3. 使用自动选取起点功能"));
}

void MainWindow::onSpeedChanged(int value)
{
    chessBoard->setAnimationSpeed(value);
}

void MainWindow::onAlgorithmChanged(int index)
{
    Q_UNUSED(index)
    updateInfoPanel();
    // 算法选择改变时，如果正在运行，需要停止
    if (stopButton->isEnabled()) {
        onStopClicked();
    }
}

void MainWindow::onNextStepClicked()
{
    chessBoard->nextStep();
}

void MainWindow::onPreviousStepClicked()
{
    chessBoard->previousStep();
}

void MainWindow::onStepSliderChanged(int value)
{
    chessBoard->goToStep(value);
    stepLabel->setText(tr("步骤: %1/%2").arg(value, chessBoard->getTotalSteps()));
}

void MainWindow::onAutoSelectStartClicked()
{
    bool useWarnsdorff = (algorithmCombo->currentIndex() == 1);
    QPoint startPos = chessBoard->findClosedTourStart(useWarnsdorff);

    if (startPos.x() >= 0 && startPos.y() >= 0) {
        chessBoard->setStartPosition(startPos.x(), startPos.y());
        QString letters = "abcdefgh";
        if (startPos.y() >= 0 && startPos.y() < letters.length() &&
            startPos.x() >= 0 && startPos.x() < 8) {
            QString posStr = QString("%1%2").arg(letters[startPos.y()]).arg(8 - startPos.x());
            statusLabel->setText(tr("已自动选择起点: %1").arg(posStr));
        }
        updateInfoPanel();
    } else {
        QMessageBox::warning(this, tr("失败"),
                             tr("未能找到可闭合巡游的起点。\n"
                                "请尝试手动选择起始位置。"));
    }
}

void MainWindow::onOpenMapClicked()
{
    if (chessBoard->getTourPath().isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先完成一次巡游！"));
        return;
    }

    if (!tourMapWindow) {
        tourMapWindow = new TourMapWindow(this);
        connect(tourMapWindow, &TourMapWindow::stepSelected, this, [this](int step) {
            chessBoard->goToStep(step);
            stepSlider->setValue(step);
        });
    }

    tourMapWindow->setTourPath(chessBoard->getTourPath());
    tourMapWindow->highlightStep(chessBoard->getCurrentStep());
    tourMapWindow->show();
    tourMapWindow->raise();
    tourMapWindow->activateWindow();
}

void MainWindow::onThemeChanged(QAction *action)
{
    Q_UNUSED(action)
    // 主题切换已在菜单中直接连接
}

void MainWindow::onShowWarnsdorffToggled(bool checked)
{
    chessBoard->setShowWarnsdorffDegrees(checked);
}

void MainWindow::onStepChanged(int step)
{
    int totalSteps = chessBoard->getTotalSteps();
    stepLabel->setText(tr("步骤: %1/%2").arg(step, totalSteps));

    if (stepSlider->maximum() != totalSteps) {
        stepSlider->setMaximum(totalSteps);
    }
    stepSlider->setValue(step);

    if (tourMapWindow && tourMapWindow->isVisible()) {
        tourMapWindow->highlightStep(step);
    }
}

void MainWindow::onHoveredCellChanged(int row, int col, bool canClose)
{
    if (row >= 0 && col >= 0 && row < 8 && col < 8) {
        QString letters = "abcdefgh";
        QString posStr = QString("%1%2").arg(letters[col]).arg(8 - row);
        QString status = canClose ? tr("可闭合") : tr("不可闭合");
        statusLabel->setText(tr("悬停: %1 - %2").arg(posStr, status));
    } else {
        statusLabel->setText(tr("请点击棋盘选择起始位置（或使用自动选取）"));
    }
}

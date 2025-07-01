#include "MainWindow.h"
#include "PlayerWidget.h"
#include "spdlog/spdlog.h"
#include "ProgressBar.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    PlayController* controller = new PlayController();
    controller->start();
    controller->setSource("C:\\Users\\qunqing\\Desktop\\v1.mp4");
    PlayerWidget* widget = new PlayerWidget(controller, this);
    ui.horizontalLayout_2->addWidget(widget);

    auto progressBar = new ProgressBar();
    progressBar->setRange(10000);

    ui.widget->layout()->addWidget(progressBar);
    
    connect(ui.pushButton_2, &QPushButton::clicked, this, [=]() {
        static bool status = false;
        if (status)
            controller->setState(PlayController::running);
        else
            controller->setState(PlayController::pause);
        status = !status;
        });
    bool* isPts = new bool(false);
    connect(controller, &PlayController::ptsChanged, this, [=](double position) {
        if (progressBar->IsDragged() == false)
        {
            progressBar->setValue(position * 10000 + 1);
        }
        //*isPts = true;
        }, Qt::QueuedConnection);
    connect(progressBar, &ProgressBar::ValueChanged, this, [=](int value) {
        spdlog::info("progress bar value: {}", value);
        controller->seek(value * 1.0 / 10000);
        });
}

MainWindow::~MainWindow()
{}


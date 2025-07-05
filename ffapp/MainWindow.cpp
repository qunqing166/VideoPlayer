#include "MainWindow.h"
#include "PlayerWidget.h"
#include "spdlog/spdlog.h"
#include "ProgressBar.h"
#include <QStringListModel>
#include <QFileInfo>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), settings("qunqing166", "MediaPlayer")
{
    ui.setupUi(this);

    controller = std::make_unique<PlayController>(new PlayController());
    PlayerWidget* widget = new PlayerWidget(controller.get(), this);
    ui.horizontalLayout_2->addWidget(widget);

    ui.pushButton_2->setIcon(QIcon(":/resource/icon/stopping.png"));
    ui.pushButton_2->setIconSize(QSize(10, 10));

    auto progressBar = new ProgressBar();
    progressBar->setRange(10000);

    ui.widget->layout()->addWidget(progressBar);

    ui.listView->setModel(new QStringListModel());

    controller->setOnMediaPlayFinished([=]() {
        int index =  ui.listView->currentIndex().row();
        int size = ui.listView->model()->rowCount();
        index = (index + 1) % size;
        ui.listView->setCurrentIndex(ui.listView->model()->index(index, 0));
        std::string name = ui.listView->model()->data(ui.listView->currentIndex()).toString().toStdString();
        controller->setSource(_nameToPath[name], PlayController::MP4);
        });

    QList<QVariant> list = settings.value("items").toList();
    for (auto item : list)
    {
        insertVideoItem(item.toString().toStdString());
    }

    connect(ui.listView, &QListView::clicked, this, [=](const QModelIndex& index) {
        std::string name = ui.listView->model()->data(index).toString().toStdString();
        controller->setSource(_nameToPath[name], PlayController::MP4);
        playState = true;
        ui.pushButton_2->setIcon(QIcon(":/resource/icon/playing.png"));
        });

    connect(ui.pushButton_6, &QPushButton::clicked, this, [=]() {
        controller->setSource(ui.lineEdit->text().toStdString(), PlayController::NetStream);
        });

    connect(ui.pushButton_4, &QPushButton::clicked, this, [&]() {
        auto files = QFileDialog::getOpenFileNames(this, QString(), QString(),  "Medias (*.mp4 *.mp3);");
        for (auto file : files)
        {
            QList<QVariant> list = settings.value("items").toList();
            list.push_front(file);
            settings.setValue("items", list);
            this->insertVideoItem(file.toStdString());
        }
        });

    
    connect(ui.pushButton_2, &QPushButton::clicked, this, [=]() {
        if (controller->getState() == PlayController::idle)return;
        if (playState)
        {
            ui.pushButton_2->setIcon(QIcon(":/resource/icon/stopping.png"));
            controller->setState(PlayController::pause);
        }
        else
        {
            ui.pushButton_2->setIcon(QIcon(":/resource/icon/playing.png"));
            controller->setState(PlayController::running);
        }
        playState = !playState;
        this->update();
        });
    bool* isPts = new bool(false);
    connect(controller.get(), &PlayController::ptsChanged, this, [=](double position) {
        if (progressBar->IsDragged() == false)
        {
            progressBar->setValue(position * 10000 + 1);
        }
        }, Qt::QueuedConnection);
    connect(progressBar, &ProgressBar::ValueChanged, this, [=](int value) {
        spdlog::info("progress bar value: {}", value);
        controller->seek(value * 1.0 / 10000);
        });
    connect(ui.pushButton_3, &QPushButton::clicked, this, [&]() {
        int index = ui.listView->currentIndex().row();
        int size = ui.listView->model()->rowCount();
        index = (index + size - 1) % size;
        ui.listView->setCurrentIndex(ui.listView->model()->index(index, 0));
        std::string name = ui.listView->model()->data(ui.listView->currentIndex()).toString().toStdString();
        controller->setSource(_nameToPath[name], PlayController::MP4);
        });
    connect(ui.pushButton, &QPushButton::clicked, this, [&]() {
        int index = ui.listView->currentIndex().row();
        int size = ui.listView->model()->rowCount();
        index = (index + 1) % size;
        ui.listView->setCurrentIndex(ui.listView->model()->index(index, 0));
        std::string name = ui.listView->model()->data(ui.listView->currentIndex()).toString().toStdString();
        controller->setSource(_nameToPath[name], PlayController::MP4);
        });
}

MainWindow::~MainWindow()
{
    
}

void MainWindow::insertVideoItem(const std::string & url)
{
    QFileInfo info(url.c_str());
    _nameToPath.insert(std::pair(info.fileName().toStdString(), url));
    ui.listView->model()->insertRow(0);
    ui.listView->model()->setData(ui.listView->model()->index(0, 0), info.fileName());
    spdlog::info("insert new video item, url: {}, filename: {}", 
        QString(url.c_str()).toLocal8Bit().toStdString(), 
        info.fileName().toLocal8Bit().toStdString());
}


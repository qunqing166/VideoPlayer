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

    PlayController* controller = new PlayController();
    PlayerWidget* widget = new PlayerWidget(controller, this);
    ui.horizontalLayout_2->addWidget(widget);

    auto progressBar = new ProgressBar();
    progressBar->setRange(10000);

    ui.widget->layout()->addWidget(progressBar);

    ui.listView->setModel(new QStringListModel());

    QList<QVariant> list = settings.value("items").toList();
    for (auto item : list)
    {
        insertVideoItem(item.toString().toStdString());
    }

    connect(ui.listView, &QListView::clicked, this, [=](const QModelIndex& index) {
        std::string name = ui.listView->model()->data(index).toString().toStdString();
        controller->setSource(QString(_nameToPath[name].c_str()));
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
{

}

void MainWindow::insertVideoItem(const std::string & url)
{
    QFileInfo info(url.c_str());
    _nameToPath.insert(std::pair(info.fileName().toStdString(), url));
    ui.listView->model()->insertRow(0);
    ui.listView->model()->setData(ui.listView->model()->index(0, 0), info.fileName());
    spdlog::info("insert new video item, url: {}, filename: {}", url, info.fileName().toStdString());
}


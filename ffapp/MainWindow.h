#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include <map>
#include <QSettings>
#include "PlayController.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:

    void insertVideoItem(const std::string& url);

    Ui::MainWindowClass ui;

    std::unique_ptr<PlayController> controller;
    std::map<std::string, std::string> _nameToPath;
    QSettings settings;
};


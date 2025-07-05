#include "PlayerWidget.h"
#include <QPainter>
#include <QThread>
#include <QPushButton>
#include "spdlog/spdlog.h"

PlayerWidget::PlayerWidget(PlayController* controller, QWidget* parent):
    _controller(controller), QWidget(parent)
{
    controller->setParent(this);

    connect(controller, &PlayController::nextVideoFrame, this, [&](const QImage& img) {
        m_currentFrame = img;
        this->update();
        }, Qt::QueuedConnection);
    connect(controller, &PlayController::sourceChanged, this, [&]() {
        this->m_currentFrame = QImage();
        update();
        });
}

PlayerWidget::~PlayerWidget()
{
    
}

void PlayerWidget::setController(PlayController* controller)
{
    this->_controller = controller;
}

void PlayerWidget::setTimeBase(double base)
{
    qDebug() << "time_base: " << base * 1000;
    timeBase = base * 1000;
    timeBase = 1.0 / 36;
}

void PlayerWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing, true);
    if (!m_currentFrame.isNull()) {
        QImage&& tmp = m_currentFrame.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(rect().topLeft() + QPoint((this->width() - tmp.width()) / 2, (this->height() - tmp.height()) / 2), tmp);
    }
}

void PlayerWidget::resizeEvent(QResizeEvent* event)
{

}

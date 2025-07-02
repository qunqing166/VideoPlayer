#include "PlayerWidget.h"
#include <QPainter>
#include <QThread>
#include <QPushButton>

PlayerWidget::PlayerWidget(PlayController* controller, QWidget* parent):
    _controller(controller), QWidget(parent)
{
    controller->setParent(this);

    connect(controller, &PlayController::nextVideoFrame, this, [&](const QImage& img) {
        m_currentFrame = img;
        this->update();
        }, Qt::QueuedConnection);

    this->resize(1000, 800);
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
    //m_timer.setInterval(1);
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

#include "previewwidget.h"
#include "ui_previewwidget.h"
#include <QEvent>
#include <QKeyEvent>

const static auto ZOOMING_KB_VALUE = 2 * ScrollAreaPannable::WheelStep;

PreviewWidget::PreviewWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PreviewWidget)
{
    ui->setupUi(this);
    ui->scrollAreaZoom->installEventFilter(this);
}

PreviewWidget::~PreviewWidget()
{
    delete ui;
}

void PreviewWidget::setImage(const QImage &image, const QString &tooltip)
{
    getScrollArea()->setMaxZoom(image.size());
    ui->lblZoomPix->setPixmap(QPixmap::fromImage(image));
    ui->lblZoomPix->setStatusTip(tooltip);
}

void PreviewWidget::resetZoom() const
{
    ui->scrollAreaZoom->zoomFitWindow();
    ui->scrollAreaZoom->ensureVisible(0,0);
}

void PreviewWidget::zoomIn() const
{
    ui->scrollAreaZoom->zoomBy(ZOOMING_KB_VALUE);
}

void PreviewWidget::zoomOut() const
{
    ui->scrollAreaZoom->zoomBy(-ZOOMING_KB_VALUE);
}

QPointer<ScrollAreaPannable> PreviewWidget::getScrollArea() const
{
    return QPointer<ScrollAreaPannable>(ui->scrollAreaZoom);
}

void PreviewWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

bool PreviewWidget::eventFilter(QObject *src, QEvent *e)
{
    //mouse events for scrollzoom are overloaded in ScrollAreaPannable, do it there
    //if you change shortcuts here, please fix hint string into on_tabsWidget_currentChanged(int index)
    if (src == ui->scrollAreaZoom)
    {
        if (e->type() == QEvent::KeyPress)
        {
            const QKeyEvent *ke = static_cast<decltype (ke)>(e);
            const auto key = ke->key();
            if ((ke->modifiers() == Qt::ControlModifier) && (key == Qt::Key_Left || key == Qt::Key_Right))
            {
                if (key == Qt::Key_Left)
                    emit prevImageRequested();
                else
                    emit nextImageRequested();
                return true;
            }
            if (key == Qt::Key_Plus || key == Qt::Key_Equal || (key == Qt::Key_Up && ke->modifiers() == Qt::ShiftModifier))
            {
                zoomIn();
                return true;
            }

            if (key == Qt::Key_Minus || (key == Qt::Key_Down && ke->modifiers() == Qt::ShiftModifier))
            {
                zoomOut();
                return true;
            }
        }
    }
    return QWidget::eventFilter(src, e);
}

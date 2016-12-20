#include "scrollareapannable.h"
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

#define ZOOM_SPEED (0.075)
const double ScrollAreaPannable::WheelStep = 15. / 120.; //can be used for zoomBy

ScrollAreaPannable::ScrollAreaPannable(QWidget *parent):
    QScrollArea (parent),
    min_width(width()), min_height(height()),
    mousePosPan(0,0),
    maxZoom(0,0)
{

}

void ScrollAreaPannable::zoomSize(int w, int h)
{
    setWidgetResizable(false);
    widget()->resize(w, h);
}

void ScrollAreaPannable::zoomFitWindow()
{
    zoomSize(min_width, min_height);
}

void ScrollAreaPannable::zoom1_1()
{
    setWidgetResizable(true);
}


void ScrollAreaPannable::zoomBy(double times)
{
    times *= ZOOM_SPEED;
    int w = static_cast<decltype(w)>(widget()->width() + widget()->width()  * times);
    int h = static_cast<decltype(h)>(widget()->height()+ widget()->height() * times);

    if (w < min_width || h < min_height)
        zoomSize(min_width, min_height);
    else
    {
        if (w > maxZoom.width() || h > maxZoom.height())
        {
            if (maxZoom.width() > width() || maxZoom.height() > height())
                zoom1_1();
            else
                zoomSize(maxZoom.width(), maxZoom.height());
        }
        else
            zoomSize(w, h);
    }
}

void ScrollAreaPannable::setMaxZoom(const QSize &maxz)
{
    double p = (double) maxz.width() / maxz.height();
    auto mh  = std::max(height(), maxz.height());
    auto mw  = std::max(static_cast<decltype(mh)>(mh * p), maxz.width());
    maxZoom  = QSize(mw, mh);

    min_height = std::min(height(), maxz.height());
    min_width = p * min_height;
}

QSize ScrollAreaPannable::getCurrentZoomedSize() const
{
    if (widgetResizable())
        return maxZoom;
    else
        return widget()->size();
}

void ScrollAreaPannable::mousePressEvent(QMouseEvent *e)
{

    if (e->button() == Qt::LeftButton)
    {
        mousePosPan = e->pos();
        setCursor(Qt::OpenHandCursor);
    }

    if (e->button() == Qt::RightButton)
    {
        //todo: button will be used to make selection
        //possibly will need option to fit content to viewport, so user can draw selection easier
        setCursor(Qt::CrossCursor);
        //        if (widgetResizable())
        //            zoomFitWindow();
        //        else
        //            zoom1_1();
    }
}

//keyboard events are filtered and intercepted by mainform, so to add keys - do it there

void ScrollAreaPannable::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() == Qt::LeftButton)
    {
        setCursor(Qt::ClosedHandCursor);
        QPoint diff = mousePosPan - e->pos();
        mousePosPan = e->pos();
        verticalScrollBar()->  setValue(verticalScrollBar()->value()   + diff.y());
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + diff.x());
    }
}

void ScrollAreaPannable::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e);
    setCursor(Qt::ArrowCursor);
    mousePosPan = QPoint(0, 0);
}

void ScrollAreaPannable::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() == Qt::ShiftModifier || event->buttons() == Qt::LeftButton)
    {
        auto angle = event->angleDelta();
        double val = (angle.y() + angle.x()) / 120.;
        zoomBy(val);
    }
    else
    {
        event->ignore();
        QScrollArea::wheelEvent(event);
    }
}


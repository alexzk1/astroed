#include "scrollareapannable.h"
#include <QMouseEvent>
#include <QScrollBar>
#include <QDebug>

#define RIGHT_BUTTON_SPEEDUP (2)

ScrollAreaPannable::ScrollAreaPannable(QWidget *parent):
    QScrollArea (parent),
    mousePosPan(0,0)
{

}

void ScrollAreaPannable::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
    {
        mousePosPan = e->pos();
        setCursor(Qt::OpenHandCursor);
    }

    if (e->button() == Qt::LeftButton)
    {
        //todo: left button will be used to make selection
        setCursor(Qt::CrossCursor);
    }
}

void ScrollAreaPannable::mouseMoveEvent(QMouseEvent *e)
{
    if((e->buttons() & Qt::RightButton) == e->buttons())
    {
        setCursor(Qt::ClosedHandCursor);
        QPoint diff = mousePosPan - e->pos();
        mousePosPan = e->pos();
        verticalScrollBar()->setValue(verticalScrollBar()->value()     + diff.y());
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() + diff.x());
    }
}

void ScrollAreaPannable::mouseReleaseEvent(QMouseEvent *e)
{
    setCursor(Qt::ArrowCursor);
    mousePosPan = QPoint(0, 0);
}

#include "customtableview.h"
#include "custom_roles.h"
#include <QMouseEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QModelIndex>
#include <QAbstractTableModel>
#include <map>

CustomTableView::CustomTableView(QWidget *parent):
    QTableView (parent),
    m_lastIndex()
{
    setMouseTracking(true);
    installEventFilter(this);
}

void CustomTableView::dataChangedInModel(const QModelIndex &start, const QModelIndex &end)
{
    for (int row = start.row(), rsz = end.row(); row <= rsz; ++row)
    {
        resizeColumnToContents(start.column());
        resizeRowToContents(row);
    }
}

void CustomTableView::mouseMoveEvent(QMouseEvent *event)
{
    QAbstractItemModel *m(model());
    // Only do something when a model is set.
    if (m)
    {
        QModelIndex index = indexAt(event->pos());
        if (index.isValid())
        {
            // When the index is valid, compare it to the last row.
            // Only do something when the the mouse has moved to a new row.
            if (index != m_lastIndex)
            {
                // Request the data for the MyMouseCursorRole.
                QVariant data = m->data(index, MyMouseCursorRole);
                Qt::CursorShape shape = Qt::ArrowCursor;
                if (!data.isNull())
                    shape = static_cast<decltype(shape)>(data.toInt());
                setCursor(shape);
            }
        }
        m_lastIndex = index;
    }
    QTableView::mouseMoveEvent(event);
}


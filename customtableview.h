#ifndef CUSTOMTABLEVIEW_H
#define CUSTOMTABLEVIEW_H
#include <QTableView>
#include <QModelIndex>
#include <map>

class CustomTableView : public QTableView
{
public:
    explicit CustomTableView(QWidget *parent);
    void dataChangedInModel(const QModelIndex& start, const QModelIndex &end);

private:
    QModelIndex m_lastIndex;

protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // CUSTOMTABLEVIEW_H

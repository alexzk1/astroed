#ifndef CUSTOMTABLEVIEW_H
#define CUSTOMTABLEVIEW_H
#include <QTableView>
#include <QModelIndex>
class CustomTableView : public QTableView
{
public:
    explicit CustomTableView(QWidget *parent);

    void dataChangedInModel(const QModelIndex& index);
private:
    QModelIndex m_lastIndex;
protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;
};

#endif // CUSTOMTABLEVIEW_H

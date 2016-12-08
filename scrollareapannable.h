#ifndef SCROLLAREAPANNABLE_H
#define SCROLLAREAPANNABLE_H
#include <QScrollArea>

class ScrollAreaPannable : public QScrollArea
{
public:
    explicit ScrollAreaPannable(QWidget *parent);
private:
    QPoint mousePosPan;
protected:
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
};

#endif // SCROLLAREAPANNABLE_H

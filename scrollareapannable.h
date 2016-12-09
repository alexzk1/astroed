#ifndef SCROLLAREAPANNABLE_H
#define SCROLLAREAPANNABLE_H
#include <QScrollArea>

class ScrollAreaPannable : public QScrollArea
{
public:
    explicit ScrollAreaPannable(QWidget *parent);
    void zoomFitWindow();
    void zoom1_1();
    void zoomBy(double times); //grow percentage
    void setMaxZoom(const QSize& maxz);
    QSize getCurrentZoomedSize() const;
private:
    int   min_width, min_height;
    QPoint mousePosPan;
    QSize  maxZoom;
protected:
    void zoomSize(int w, int h);
    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;
    virtual void mouseReleaseEvent(QMouseEvent *e) override;
    virtual void wheelEvent(QWheelEvent *event) override;
};

#endif // SCROLLAREAPANNABLE_H

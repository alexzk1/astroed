#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QWidget>
#include <QPointer>
#include "scrollareapannable.h"

namespace Ui {
    class PreviewWidget;
}

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);
    virtual ~PreviewWidget() override;

    QPointer<ScrollAreaPannable> getScrollArea() const;
    const static QString &getKeyboardUsage();
public slots:
    void setImage(const QImage& image, const QString& tooltip = "");
    void resetZoom() const;
    void zoomIn() const;
    void zoomOut() const;
signals:
    void nextImageRequested() const; //user lists to next image on preview
    void prevImageRequested() const;
protected:
    virtual void changeEvent(QEvent *e) override;
    bool eventFilter(QObject *src, QEvent *e) override;
private:
    Ui::PreviewWidget *ui;
};

#endif // PREVIEWWIDGET_H

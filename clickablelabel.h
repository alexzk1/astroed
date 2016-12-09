#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H
#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel( const QString& text="", QWidget* parent=0 ): QLabel(text, parent){}
    explicit ClickableLabel( QWidget* parent=0 ): QLabel(parent){}
signals:
    void clicked();
protected:
    virtual void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
            emit clicked();
    }
};

#endif // CLICKABLELABEL_H

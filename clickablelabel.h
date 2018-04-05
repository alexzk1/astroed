#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H
#include <QLabel>
#include <QMouseEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel( const QString& text="", QWidget* parent=nullptr ): QLabel(text, parent){}
    explicit ClickableLabel( QWidget* parent=nullptr ): QLabel(parent){}
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

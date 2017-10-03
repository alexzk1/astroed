#ifndef LABELEDSLIDER_H
#define LABELEDSLIDER_H

#include <QWidget>
#include <QLabel>
#include <QSlider>

namespace Ui {
    class LabeledSlider;
}

class LabeledSlider : public QWidget
{
    Q_OBJECT
public:
    explicit LabeledSlider(QWidget *parent = nullptr);
    ~LabeledSlider();

    //i'm too lazy to make delegates etc ... just don't delete those pointers
    QSlider* getSlider() const;
    QLabel*  getText() const;
protected:
    void changeEvent(QEvent *e);

private:
    Ui::LabeledSlider *ui;

};

#endif // LABELEDSLIDER_H

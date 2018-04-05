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
    virtual ~LabeledSlider() override;

    //i'm too lazy to make delegates etc ... just don't delete those pointers
    QSlider* getSlider() const;
    QLabel*  getText() const;
protected:
    virtual void changeEvent(QEvent *e) override;

private:
    Ui::LabeledSlider *ui;

};

#endif // LABELEDSLIDER_H

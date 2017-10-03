#ifndef SLIDERDROP_H
#define SLIDERDROP_H

#include <QObject>
#include <QWidget>
#include <QWidgetAction>
#include <QPointer>
#include "labeledslider.h"
class SliderDrop : public QWidgetAction
{
    Q_OBJECT
public:
    SliderDrop(QWidget *p);
    QPointer<LabeledSlider> slider();
private:
    QPointer<LabeledSlider> m_slider;
};

#endif // SLIDERDROP_H

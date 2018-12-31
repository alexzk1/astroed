#include "sliderdrop.h"
#include "labeledslider.h"

SliderDrop::SliderDrop(QWidget *p):
    QWidgetAction (p),
    m_slider(new LabeledSlider(p))
{
    setDefaultWidget(m_slider);
}

QPointer<LabeledSlider> SliderDrop::slider()
{
    return m_slider;
}

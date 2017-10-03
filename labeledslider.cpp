#include "labeledslider.h"
#include "ui_labeledslider.h"
#include <QEvent>

LabeledSlider::LabeledSlider(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LabeledSlider)
{
    ui->setupUi(this);
}

LabeledSlider::~LabeledSlider()
{
    delete ui;
}

QSlider *LabeledSlider::getSlider() const
{
    return ui->slider;
}

QLabel *LabeledSlider::getText() const
{
    return ui->text;
}

void LabeledSlider::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

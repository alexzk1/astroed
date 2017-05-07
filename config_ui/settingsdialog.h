//License: MIT, (c) Oleksiy Zakharov, 2016, alexzkhr@gmail.com

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "utils/saveable_widget.h"

namespace Ui {
    class SettingsDialog;
}

class SettingsDialog : public QDialog, protected virtual utility::SaveableWidget<SettingsDialog>
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H

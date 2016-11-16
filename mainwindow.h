#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QFileSystemModel>
#include "utils/inst_once.h"
#include "utils/saveable_widget.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow, public utility::ItCanBeOnlyOne<MainWindow>, protected utility::SaveableWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QString getSelectedFolder();
    void selectPath(const QString &path, bool collapse = true);
protected:
    void changeEvent(QEvent *e);
    virtual void recurseWrite(QSettings& settings, QObject* object);
    virtual void recurseRead(QSettings& settings, QObject* object);

    void currentDirChanged(const QString& dir);
private:
    Ui::MainWindow *ui;
    QPointer<QFileSystemModel> dirsModel;
    void setupFsBrowsing();
};

#endif // MAINWINDOW_H

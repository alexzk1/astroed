#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QFileSystemModel>
#include <QLabel>
#include "utils/inst_once.h"
#include "utils/saveable_widget.h"
#include "previewsmodel.h"

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
    QLabel &openPreviewTab();

    QString styler;
protected:
    void changeEvent(QEvent *e);
    virtual void recurseWrite(QSettings& settings, QObject* object);
    virtual void recurseRead(QSettings& settings, QObject* object);

    void currentDirChanged(const QString& dir);
private:
    Ui::MainWindow *ui;
    QPointer<QFileSystemModel> dirsModel;
    QPointer<PreviewsModel>    previewsModel;
    QPointer<PreviewsDelegate> previewsDelegate;
    QPointer<QLabel>           memoryLabel;
    void setupFsBrowsing();
};

#endif // MAINWINDOW_H

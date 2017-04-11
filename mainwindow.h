#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QFileSystemModel>
#include <QLabel>
#include <QProgressBar>
#include <QLayout>
#include <QToolBar>
#include <QActionGroup>

#include "utils/inst_once.h"
#include "utils/saveable_widget.h"
#include "previewsmodel.h"

namespace Ui
{
class MainWindow;
}

class ClickableLabel;
class QSortFilterProxyModel;
class SettingsDialog;

class MainWindow : public QMainWindow, public utility::ItCanBeOnlyOne<MainWindow>, protected utility::SaveableWidget
{
    Q_OBJECT
public:
    QString styler;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString getSelectedFolder();
    void loadProjectFromFile(const QString& name);
public slots:
    void openPreviewTab(const imaging::image_buffer_ptr &image, const QString &fileName, size_t pictureRole);
    void resetPreview();
    void selectPath(const QString &path, bool collapse = true);
    void showTempNotify(const QString& text, int delay = 10000);
protected:
    virtual void changeEvent(QEvent *e) override;
    virtual bool eventFilter(QObject *src, QEvent *e) override;

    virtual void recurseWrite(QSettings& settings, QObject* object) override;
    virtual void recurseRead(QSettings& settings, QObject* object) override;

    void currentDirChanged(const QString& dir);
private slots:
    void on_tabsWidget_currentChanged(int index);
    void on_actionNewtone_toggled(bool checked);
    void on_actionGuess_Darks_triggered();

    void on_actionSettings_triggered();

    void on_actionSave_As_triggered();

    void on_actionSet_All_Source_triggered();

    void on_actionSet_All_Ignored_triggered();

    void on_actionSet_All_Darks_triggered();

    void on_actionSaveProject_triggered();

    void on_actionCopy_as_Lua_triggered();

    void on_actionLoad_project_triggered();

    void on_actionReload_triggered();

private:
    Ui::MainWindow *ui;
    QPointer<QFileSystemModel> dirsModel;
    QPointer<PreviewsModel>    previewsModel;
    QPointer<PreviewsDelegate> previewsDelegate;
    QPointer<ClickableLabel>   memoryLabel;
    QPointer<ClickableLabel>   fileNameLabel;
    QPointer<QProgressBar>     loadingProgress;
    QPointer<SettingsDialog>   settDialog;
    int previewShift;
    QSize lastPreviewSize;
    const QString originalStylesheet;
    QString lastPreviewFileName;
    std::vector<QPointer<QAction>> zoomPicModeActions;
    QPointer<QActionGroup> zoomPicModeActionsGroup;
    void setupFsBrowsing();
    void setupZoomGui();

    const static QString zoomKbHintText;


    QToolBar* addToolbarToLayout(QLayout* src, int pos = 0);
};

#endif // MAINWINDOW_H

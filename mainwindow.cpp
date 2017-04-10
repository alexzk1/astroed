#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include <QDebug>
#include <QApplication>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QStatusBar>
#include <QTimer>
#include <QFileInfo>
#include <QHeaderView>
#include <QFileDialog>
#include <QClipboard>
#include <QActionGroup>
#include "clickablelabel.h"
#include "editor/luaeditor.h"
#include "config_ui/settingsdialog.h"
#include "custom_roles.h"
#include "utils/strutils.h"

const static auto ZOOMING_KB_VALUE = 2 * ScrollAreaPannable::WheelStep;

const static QString defaultProjFile = "/ae_project.luap";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dirsModel(nullptr),
    previewsModel(new PreviewsModel(this)),
    previewsDelegate(new PreviewsDelegate(this)),
    memoryLabel(new ClickableLabel(this)),
    fileNameLabel(new ClickableLabel(this)),
    loadingProgress(new QProgressBar(this)),
    settDialog(new SettingsDialog(this)),
    previewShift(0),
    lastPreviewSize(-2, -2), //i think this should differ from what delegate has (-1, -1) to ensure initial reset
    originalStylesheet(qApp->styleSheet()),
    lastPreviewFileName(),
    zoomPicModeActions(),
    zoomPicModeActionsGroup()
{
    ui->setupUi(this);

    ui->previewsTable->setModel(previewsModel);
    ui->previewsTable->setItemDelegate(previewsDelegate);

    ui->tabsWidget->setCurrentWidget(ui->tabFiles);


    QFile style(":/styles/darkorange");
    style.open(QIODevice::ReadOnly | QFile::Text);
    styler = style.readAll();


    //ok, i dont like that style for the whole app, but need some good visible scrollbars with huge black space picture
    ui->scrollAreaZoom->setStyleSheet(styler);

    auto hdr = ui->previewsTable->horizontalHeader();
    if (hdr)
    {
        //ui will be not responsive with this, need to do manualy
        //hdr->setSectionResizeMode(QHeaderView::ResizeToContents);
        hdr->setStretchLastSection(true);
    }

    connect(previewsModel, &PreviewsModel::startedPreviewsLoad, this, [this](bool scroll){
        if (ui->previewsTable && scroll)
            ui->previewsTable->scrollToTop();
        if (loadingProgress)
        {
            loadingProgress->setVisible(true);
            loadingProgress->setRange(0, 100);
            loadingProgress->setValue(0);
        }
    }, Qt::QueuedConnection);

    connect(previewsModel, &PreviewsModel::loadProgress, this, [this](int pr){
        if (loadingProgress)
            loadingProgress->setValue(pr);
    }, Qt::QueuedConnection);

    connect(previewsModel, &PreviewsModel::finishedPreviewsLoad, this, [this](){
        if (loadingProgress)
            loadingProgress->setVisible(false);
        //qDebug() << ((PreviewsModel*)ui->previewsTable->model())->generateLuaString().c_str();
    }, Qt::QueuedConnection);

    connect(previewsModel, &QAbstractTableModel::dataChanged, this, [this](const auto start, const auto end, const auto&)
    {
        if (ui->previewsTable)
            ui->previewsTable->dataChangedInModel(start, end);

    }, Qt::QueuedConnection);

    setWindowTitle("");
    setupFsBrowsing();
    readSettings(this);

    statusBar()->addPermanentWidget(memoryLabel);
    statusBar()->addPermanentWidget(fileNameLabel);
    memoryLabel->setToolTip(tr("The approximate size of memory used. Click to GC."));

    statusBar()->addPermanentWidget(loadingProgress);
    loadingProgress->resize(50, statusBar()->height() - 3);
    loadingProgress->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    loadingProgress->setVisible(false);
    loadingProgress->setToolTip(tr("Previews' loading progress."));

    auto updateMemoryLabel = [this](bool force)
    {
        IMAGE_LOADER.gc(force);
        PREVIEW_LOADER.gc(force);
        QLabel *p = memoryLabel;
        if (p)
        {
            const static auto mb = 1024 * 1024;
            p->setText(QString("Total: %1 MB; Previews: %2 MB").arg((IMAGE_LOADER.getMemoryUsed() + PREVIEW_LOADER.getMemoryUsed()) /mb).arg(PREVIEW_LOADER.getMemoryUsed() / mb));
        }
    };

    //done: add some doubleclick handler to the label which calls gc() (maybe even forced gc(true))
    connect(memoryLabel, &ClickableLabel::clicked, this, std::bind(updateMemoryLabel, true));
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, std::bind(updateMemoryLabel, false));
    timer->start(2000);

    setupZoomGui();

    //lexer must be created prior widget
    //todo: add complete api list here to lexer
    LuaEditor::createSharedLuaLexer({{"apiTest", "param", "test function"}});
    ui->tabScript->layout()->addWidget(new LuaEditor(this));
}

MainWindow::~MainWindow()
{
    writeSettings(this);
    delete ui;
}

void MainWindow::selectPath(const QString &path, bool collapse)
{
    if (!path.isEmpty())
    {
        if (dirsModel)
        {
            auto index = dirsModel->index(path);
            if (collapse)
                ui->dirsTree->collapseAll();

            if (index.isValid() && ui->dirsTree)
            {
                ui->dirsTree->setCurrentIndex(index);
                while (index.isValid())
                {
                    ui->dirsTree->expand(index);
                    index = index.parent();
                }
            }
        }
    }
}

void MainWindow::openPreviewTab(const imaging::image_buffer_ptr& image, const QString& fileName, size_t pictureRole)
{
    lastPreviewFileName = fileName;
    const auto txt = (fileName.isEmpty())? "":QString(tr("File: %1")).arg(QFileInfo(fileName).fileName());
    ui->tabsWidget->setCurrentWidget(ui->tabZoomed);
    ui->scrollAreaZoom->setMaxZoom(lastPreviewSize = image->size());
    fileNameLabel->setText(txt);
    if (!fileName.isEmpty())
    {
        auto meta = IMAGE_LOADER.getExif(fileName);
        ui->lblZoomPix->setStatusTip(meta.getStringValue());
    }
    ui->lblZoomPix->setPixmap(QPixmap::fromImage(*image));
    if (zoomPicModeActions.size() > pictureRole && zoomPicModeActions.at(pictureRole))
    {
        if (zoomPicModeActionsGroup)
            zoomPicModeActionsGroup->blockSignals(true);
        zoomPicModeActions.at(pictureRole)->setChecked(true);
        if (zoomPicModeActionsGroup)
            zoomPicModeActionsGroup->blockSignals(false);
    }
}

void MainWindow::showTempNotify(const QString &text, int delay)
{
    statusBar()->showMessage(text, delay);
}

void MainWindow::resetPreview()
{
    previewShift = 0;
    ui->scrollAreaZoom->zoomFitWindow();
    ui->scrollAreaZoom->ensureVisible(0,0);
    if (zoomPicModeActions.size() && zoomPicModeActions.at(0))
    {
        if (zoomPicModeActionsGroup)
            zoomPicModeActionsGroup->blockSignals(true);
        zoomPicModeActions.at(0)->setChecked(true);
        if (zoomPicModeActionsGroup)
            zoomPicModeActionsGroup->blockSignals(false);
    }
    //qDebug()  <<"resetPreview()";
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type())
    {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
    QMainWindow::changeEvent(e);
}

const QString MainWindow::zoomKbHintText = MainWindow::tr("LBM /arrows - pan, RBM - select, LBM + wheel(shift + wheel, +-, shift+up/down) - zoom. ctrl + L/R arrows to list files.");
void MainWindow::on_tabsWidget_currentChanged(int index)
{
    //active tab changed slot
    Q_UNUSED(index);
    if (ui->tabsWidget->currentWidget() == ui->tabZoomed)
    {
        //making all dark on big preview, because it's bad for eyes when around black space u see white window borders
        qApp->setStyleSheet(styler);
        static bool session_once = true;
        if (session_once)
        {
            showTempNotify(zoomKbHintText, 15000);
            session_once = false;
            statusBar()->setToolTip(zoomKbHintText);
        }
    }
    else
    {
        fileNameLabel->setText("");
        qApp->setStyleSheet(originalStylesheet);
    }

    if (ui->tabsWidget->currentWidget() == ui->tabFiles && !lastPreviewFileName.isEmpty() && ui->previewsTable)
    {
        //synchronizing list to current zoomed image
        auto m = ui->previewsTable->model();
        if (m)
        {
            auto lst = m->match(m->index(0, 0), MyGetPathRole, lastPreviewFileName);
            if (lst.size())
                ui->previewsTable->scrollTo(lst.at(0));
        }
    }
}

bool MainWindow::eventFilter(QObject *src, QEvent *e)
{
    //mouse events for scrollzoom are overloaded in ScrollAreaPannable, do it there
    //if you change shortcuts here, please fix hint string into on_tabsWidget_currentChanged(int index)
    if (e->type() == QEvent::KeyPress && src == ui->scrollAreaZoom)
    {
        const QKeyEvent *ke = static_cast<decltype (ke)>(e);
        auto key = ke->key();
        if ((ke->modifiers() == Qt::ControlModifier) && (key == Qt::Key_Left || key == Qt::Key_Right))
        {
            auto s = previewShift + ((key == Qt::Key_Left)?-1:((key == Qt::Key_Right)?1 : 0));
            auto dele = dynamic_cast<PreviewsDelegate*>(ui->previewsTable->itemDelegate());
            if (dele)
            {
                if (dele->showLastClickedPreview(s, lastPreviewSize))
                {
                    previewShift = s;
                    //qDebug() << "set shift "<<s;
                }
                return true;
            }
        }

        if (key == Qt::Key_Plus || key == Qt::Key_Equal || (key == Qt::Key_Up && ke->modifiers() == Qt::ShiftModifier))
            ui->scrollAreaZoom->zoomBy(ZOOMING_KB_VALUE);

        if (key == Qt::Key_Minus || (key == Qt::Key_Down && ke->modifiers() == Qt::ShiftModifier))
            ui->scrollAreaZoom->zoomBy(-ZOOMING_KB_VALUE);
    }
    return false;
}

void MainWindow::recurseWrite(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    settings.setValue("LastDirSelection", getSelectedFolder());
    settings.setValue("splitter", ui->splitter->saveState());
    settings.setValue("mainwinstate", this->saveState());
    settings.setValue("maximized", this->isMaximized());
    settings.setValue("newtone", this->ui->actionNewtone->isChecked());
}

void MainWindow::recurseRead(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    this->restoreState(settings.value("mainwinstate").toByteArray());
    if (settings.value("maximized", false).toBool())
        showMaximized();
    else
        showNormal();
    this->ui->actionNewtone->setChecked(settings.value("newtone", false).toBool()); //load prior dir, so skip unneeded files list
    auto s = settings.value("LastDirSelection", QDir::homePath()).toString();


    QTimer::singleShot(500, [this, s](){
        selectPath(s);
    });
}

void MainWindow::currentDirChanged(const QString &dir)
{
    //qDebug() << "currentDirChanged: " << dir;
    previewsModel->setCurrentFolder(dir, ui->actionRecursive_Listing->isChecked());
}

QString MainWindow::getSelectedFolder()
{
    auto index = ui->dirsTree->selectionModel()->currentIndex();
    if (index.isValid())
        return dirsModel->data(index, QFileSystemModel::FilePathRole).toString();
    return "";
}

void MainWindow::setupFsBrowsing()
{
    dirsModel = new QFileSystemModel(this);
    dirsModel->setRootPath("/");
    dirsModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    dirsModel->setReadOnly(true);
    //ui->dirsTree->setModel(dirsModel);

    ui->dirsTree->setModel(dirsModel);
    //fixme: on windows that maybe incorrect, need to select some drive or so
    //if you have files ouside home folder on nix, then just symlink it, don't want to bother with full FS explorer yet
    ui->dirsTree->setRootIndex(dirsModel->index(QDir::homePath()));
    dirsModel->sort(0);
#ifdef Q_OS_WIN
#warning Revise this piece of code, on windows users dont like home folders.
#endif
    ui->dirsTree->hideColumn(1);
    ui->dirsTree->hideColumn(2);
    ui->dirsTree->hideColumn(3);


    const std::shared_ptr<QString> lastFolder(new QString());
    connect(ui->dirsTree->selectionModel(), &QItemSelectionModel::currentChanged, this, [this, lastFolder](auto p1, auto p2)
    {
        Q_UNUSED(p2);
        if (p1.isValid() && dirsModel)
        {
            *lastFolder = dirsModel->data(p1, QFileSystemModel::FilePathRole).toString();
            this->currentDirChanged(*lastFolder);
        }
    }, Qt::QueuedConnection);

    auto toolBox = addToolbarToLayout(ui->layoutFilesTree);
    if (toolBox)
    {
        toolBox->setToolButtonStyle(Qt::ToolButtonIconOnly);

        toolBox->addAction(ui->actionRecursive_Listing);
        connect(ui->actionRecursive_Listing, &QAction::triggered, this, [this, lastFolder]()
        {
            this->currentDirChanged(*lastFolder);
        }, Qt::QueuedConnection);

        //fixme: ok, that is really wrong placement, better to make 1 new toolbar, but then I need layout ... couldn't click it proper in editor,
        //any way, I want next buttons be on 1st tab (and hidden when tab switched)
        toolBox->addSeparator();
        toolBox->addAction(ui->actionSet_All_Source);
        toolBox->addAction(ui->actionSet_All_Ignored);
        toolBox->addAction(ui->actionSet_All_Darks);

        toolBox->addSeparator();
        toolBox->addAction(ui->actionGuess_Darks);
        toolBox->addAction(ui->actionCopy_as_Lua);
    }
}

void MainWindow::setupZoomGui()
{
    ui->scrollAreaZoom->installEventFilter(this);
    auto zoomToolbar = addToolbarToLayout(ui->tabZoomed->layout());
    if (zoomToolbar)
    {
        zoomToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        zoomToolbar->addAction(ui->actionSave_As);
        zoomToolbar->addSeparator();
        zoomPicModeActions = {
            zoomToolbar->addAction(QIcon(":/icons/icons/Science-Minus2-Math-icon.png"), "Ignored"),
            zoomToolbar->addAction(QIcon(":/icons/icons/Science-Plus2-Math-icon.png"), "Source"),
            zoomToolbar->addAction(QIcon(":/icons/icons/Cloud-app-icon.png"), "Dark")
        };
        zoomPicModeActionsGroup = new QActionGroup(this); //have to use class-member because will need to block signals, so will not fall into recursion model->group->model->group
        zoomPicModeActionsGroup->setExclusive(true);
        for (size_t i = 0, sz = zoomPicModeActions.size(); i < sz; ++i)
        {
            const auto& a = zoomPicModeActions.at(i);
            a->setCheckable(true);
            a->setToolTip(a->text());
            a->setProperty("model_role", static_cast<int>(i));
            zoomPicModeActionsGroup->addAction(a);
        }

        connect(zoomPicModeActionsGroup, &QActionGroup::triggered, this, [this](QAction *a)
        {
            if (previewsModel && a)
                previewsModel->setRoleFor(lastPreviewFileName, a->property("model_role").toInt());
        }, Qt::QueuedConnection);
    }
}

QToolBar *MainWindow::addToolbarToLayout(QLayout *src, int pos)
{
    QToolBar* result = nullptr;
    auto hbox = qobject_cast<QHBoxLayout*>(src);
    if (hbox)
    {
        result = new QToolBar(this);
        result->setOrientation(Qt::Vertical);
        hbox->insertWidget(pos, result);
    }
    else
    {
        auto vbox = qobject_cast<QVBoxLayout*>(src);
        if (vbox)
        {
            result = new QToolBar(this);
            result->setOrientation(Qt::Horizontal);

            vbox->insertWidget(pos, result);
        }
    }

    if (result)
    {
        auto st = (ui->mainToolBar)?ui->mainToolBar->toolButtonStyle():Qt::ToolButtonFollowStyle;
        result->setToolButtonStyle(st);
    }

    return result;
}

void MainWindow::on_actionNewtone_toggled(bool checked)
{
    QString lastFolder = getSelectedFolder();
    IMAGE_LOADER.setNewtoneTelescope(checked);
    PREVIEW_LOADER.setNewtoneTelescope(checked);
    if (!lastFolder.isEmpty())
        currentDirChanged(lastFolder);
}

void MainWindow::on_actionGuess_Darks_triggered()
{
    if (previewsModel)
        previewsModel->guessDarks();
}

void MainWindow::on_actionSettings_triggered()
{
    if (settDialog)
        settDialog->show();
}

void MainWindow::on_actionSave_As_triggered()
{
    auto img = IMAGE_LOADER.getImage(lastPreviewFileName);
    if (img && !img->isNull())
    {
        QSettings sett;

        QString lastFolder = sett.value("LastExportFolder", QDir::homePath()).toString();
        if (!QDir(lastFolder).exists())
            lastFolder = QDir::homePath();

        QUrl tmpu(lastPreviewFileName);
        if (IMAGE_LOADER.isProperVfs(tmpu))
        {
            //if we have movie frame, lets name it so as original
            lastFolder = QString("%3/%1_%2.png").arg(tmpu.fileName().replace('.','_')).arg(tmpu.fragment()).arg(lastFolder);
        }

        auto name = QFileDialog::getSaveFileName(this, tr("Export Image"), lastFolder, "png (*.png)");
        if (!name.isEmpty())
        {
            QFileInfo tmp(name);
            sett.setValue("LastExportFolder", tmp.absolutePath());
            if (tmp.suffix().isEmpty())
                name = name +".png";
            img->save(name);
            sett.sync();
        }
    }
}

void MainWindow::on_actionSet_All_Source_triggered()
{
    //this magic number is hardly bound to fileRoles inside model's cpp (it is index in list)
    if (previewsModel)
        previewsModel->setAllRole(1);
}

void MainWindow::on_actionSet_All_Ignored_triggered()
{
    //this magic number is hardly bound to fileRoles inside model's cpp (it is index in list)
    if (previewsModel)
        previewsModel->setAllRole(0);
}

void MainWindow::on_actionSet_All_Darks_triggered()
{
    //this magic number is hardly bound to fileRoles inside model's cpp (it is index in list)
    if (previewsModel)
        previewsModel->setAllRole(2);
}

void MainWindow::on_actionSaveProject_triggered()
{
    QSettings sett;

    QString lastFolder = getSelectedFolder();
    if (lastFolder.isEmpty())
        sett.value("LastSaveProjectFolder", QDir::homePath()).toString();

    lastFolder =lastFolder + defaultProjFile;
    auto name = QFileDialog::getSaveFileName(this, tr("Save Project"), lastFolder, "luap (*.luap)");
    if (!name.isEmpty())
    {
        QFileInfo tmp(name);
        sett.setValue("LastSaveProjectFolder", tmp.absolutePath());
        if (tmp.suffix().isEmpty())
            name = name +".luap";
        sett.sync();

        std::fstream fs(name.toStdString(), std::ios_base::out);

        if (previewsModel)
            previewsModel->generateProjectCode(fs);

        //todo: add more things to save as project

        fs.flush();
        fs.close();
    }
}

void MainWindow::on_actionCopy_as_Lua_triggered()
{
    if (previewsModel)
    {
        auto str = previewsModel->generateProjectCodeString();
        QApplication::clipboard()->setText(str.c_str(), QClipboard::Clipboard);
    }
}

void MainWindow::loadProjectFromFile(const QString &name)
{
    std::fstream fs(name.toStdString(), std::ios_base::in);
    std::string src = utility::read_stream_into_container(fs);
    fs.close();

    if (previewsModel)
        previewsModel->loadProjectCode(src);

    //todo: add more things to load as project

}


void MainWindow::on_actionLoad_project_triggered()
{
    if (previewsModel)
    {
        QSettings sett;
        QString lastFolder = getSelectedFolder();
        if (lastFolder.isEmpty())
            sett.value("LastSaveProjectFolder", QDir::homePath()).toString();
        else
        {
            auto tmp = lastFolder + defaultProjFile;
            if (QFile::exists(tmp))
                lastFolder = tmp;
        }
        auto name = QFileDialog::getOpenFileName(this, tr("Load Project"), lastFolder, "luap (*.luap)");
        if (!name.isEmpty())
        {
            QFileInfo tmp(name);
            sett.setValue("LastSaveProjectFolder", tmp.absolutePath());
            sett.sync();
            loadProjectFromFile(name);
        }
    }
}

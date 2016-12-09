#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include <QDebug>
#include <QApplication>
#include <QItemSelectionModel>
#include <QStatusBar>
#include <QTimer>
#include <QHeaderView>
#include "clickablelabel.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dirsModel(nullptr),
    previewsModel(new PreviewsModel(this)),
    previewsDelegate(new PreviewsDelegate(this)),
    memoryLabel(new ClickableLabel(this)),
    previewShift(0),
    originalStylesheet(qApp->styleSheet())
{
    ui->setupUi(this);
    ui->previewsTable->setItemDelegate(previewsDelegate);
    ui->previewsTable->setModel(previewsModel);
    ui->tabsWidget->setCurrentWidget(ui->tabFiles);

    QFile style(":/styles/darkorange");
    style.open(QIODevice::ReadOnly | QFile::Text);
    styler = style.readAll();


    //ok, i dont like that style for the whole app, but need some good visible scrollbars with huge black space picture
    ui->scrollAreaZoom->setStyleSheet(styler);

    auto hdr = ui->previewsTable->horizontalHeader();
    if (hdr)
        hdr->setStretchLastSection(true);

    connect(previewsModel, &QAbstractTableModel::modelReset, this, [this](){
        qDebug() << "Views model-reset";
        if (ui->previewsTable)
        {
            ui->previewsTable->resizeColumnsToContents();
            ui->previewsTable->resizeRowsToContents();
        }
    }, Qt::QueuedConnection);


    connect(previewsModel, &QAbstractTableModel::dataChanged, this, [this](const auto ind, const auto, const auto&){
        if (ui->previewsTable)
        {
            if (ind.isValid())
            {
                ui->previewsTable->resizeColumnToContents(ind.column());
                ui->previewsTable->resizeRowToContents(ind.row());
            }
        }
    }, Qt::QueuedConnection);

    setWindowTitle("");
    setupFsBrowsing();
    readSettings(this);
    statusBar()->addPermanentWidget(memoryLabel);
    memoryLabel->setToolTip(tr("The approximate size of memory used. Click to GC."));

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

    ui->scrollAreaZoom->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    writeSettings(this);
    delete ui;
}

QString MainWindow::getSelectedFolder()
{
    auto index = ui->dirsTree->selectionModel()->currentIndex();
    return dirsModel->data(index, QFileSystemModel::FilePathRole).toString();
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
            //qApp->processEvents();
            //qApp->flush();
        }
    }
}

QLabel &MainWindow::openPreviewTab(const QSize& maxSize)
{
    ui->tabsWidget->setCurrentWidget(ui->tabZoomed);
    ui->scrollAreaZoom->setMaxZoom(maxSize);
    qDebug()  <<"openPreviewTab()";

    return *ui->lblZoomPix;
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

    qDebug()  <<"resetPreview()";
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}

bool MainWindow::eventFilter(QObject *src, QEvent *e)
{
    if (e->type() == QEvent::KeyPress && src == ui->scrollAreaZoom)
    {
        const QKeyEvent *ke = static_cast<decltype (ke)>(e);
        auto key = ke->key();
        if (key == Qt::Key_Left || key == Qt::Key_Right)
        {
            auto s = previewShift + ((key == Qt::Key_Left)?-1:((key == Qt::Key_Right)?1 : 0));
            auto dele = dynamic_cast<PreviewsDelegate*>(ui->previewsTable->itemDelegate());
            if (dele)
            {
                if (dele->showLastClickedPreview(s, false))
                {
                    previewShift = s;
                    qDebug() << "set shift "<<s;
                }
                return true;
            }
        }
    }
    return false;
}

void MainWindow::recurseWrite(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    settings.setValue("LastDirSelection", getSelectedFolder());
    settings.setValue("splitter", ui->splitter->saveState());
    settings.setValue("mainwinstate", this->saveState());
}

void MainWindow::recurseRead(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    ui->splitter->restoreState(settings.value("splitter").toByteArray());
    this->restoreState(settings.value("mainwinstate").toByteArray());

    auto s = settings.value("LastDirSelection", QDir::homePath()).toString();
    qDebug() << "Read: "<<s;
    selectPath(s);

}

void MainWindow::currentDirChanged(const QString &dir)
{
    previewsModel->setCurrentFolder(dir, true);
}

void MainWindow::setupFsBrowsing()
{
    dirsModel = new QFileSystemModel(this);
    dirsModel->setRootPath("/");

    dirsModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    dirsModel->setReadOnly(true);
    ui->dirsTree->setModel(dirsModel);
    //fixme: on windows that maybe incorrect, need to select some drive or so
    //if you have files ouside home folder on nix, then just symlink it, don't want to bother with full FS explorer yet
    ui->dirsTree->setRootIndex(dirsModel->index(QDir::homePath()));
#ifdef Q_OS_WIN
#warning Revise this piece of code, on windows users dont like home folders.
#endif
    ui->dirsTree->hideColumn(1);
    ui->dirsTree->hideColumn(2);
    ui->dirsTree->hideColumn(3);

    connect(ui->dirsTree->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](auto p1, auto p2)
    {
        Q_UNUSED(p2);
        if (p1.isValid() && dirsModel)
            this->currentDirChanged(dirsModel->data(p1, QFileSystemModel::FilePathRole).toString());
    }, Qt::QueuedConnection);
}

void MainWindow::on_tabsWidget_currentChanged(int index)
{
    //active tab changed slot
    Q_UNUSED(index);
    qApp->setStyleSheet(originalStylesheet);
    if (ui->tabsWidget->currentWidget() == ui->tabZoomed)
    {
        //making all dark on big preview, because it's bad for eyes when around black space u see white window borders
        qApp->setStyleSheet(styler);
        showTempNotify(tr("LBM - pan, RBM - select, LBM + wheel(shift + wheel) - zoom. L/R arrows to list files."), 20000);
    }

}

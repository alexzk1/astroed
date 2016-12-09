#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include <QDebug>
#include <QApplication>
#include <QItemSelectionModel>
#include <QStatusBar>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dirsModel(nullptr),
    previewsModel(new PreviewsModel(this)),
    previewsDelegate(new PreviewsDelegate(this)),
    memoryLabel(new QLabel(this))
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
    memoryLabel->setToolTip(tr("The approximate size of memory used."));

    //todo: add some doubleclick handler to the label which calls gc() (maybe even forced gc(true))

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]()
    {
        QLabel *p = memoryLabel;
        if (p) //need to check, because timer maybe deleted after label
        {
            IMAGE_LOADER.gc();
            PREVIEW_LOADER.gc();
            size_t mb = (IMAGE_LOADER.getMemoryUsed() + PREVIEW_LOADER.getMemoryUsed()) / (1024 * 1024);
            p->setText(QString("%1 MB").arg(mb));
        }
    });
    timer->start(2000);

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
    ui->scrollAreaZoom->ensureVisible(0,0);
    ui->scrollAreaZoom->setMaxZoom(maxSize);
    ui->scrollAreaZoom->zoomFitWindow();
    return *ui->lblZoomPix;
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

void MainWindow::recurseWrite(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
    settings.setValue("LastDirSelection", getSelectedFolder());
}

void MainWindow::recurseRead(QSettings &settings, QObject *object)
{
    Q_UNUSED(object);
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

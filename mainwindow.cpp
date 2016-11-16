#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTreeView>
#include <QDebug>
#include <QApplication>
#include <QItemSelectionModel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dirsModel(nullptr),
    previewsModel(new PreviewsModel(this)),
    previewsDelegate(new PreviewsDelegate(this))
{
    ui->setupUi(this);
    ui->previewsTable->setItemDelegate(previewsDelegate);
    ui->previewsTable->setModel(previewsModel);

    auto hdr = ui->previewsTable->horizontalHeader();
    if (hdr)
        hdr->setStretchLastSection(true);

    connect(previewsModel, &QAbstractTableModel::modelReset, this, [this](){
        if (ui->previewsTable)
        {
            ui->previewsTable->resizeColumnsToContents();
            ui->previewsTable->resizeRowsToContents();
        }
    }, Qt::QueuedConnection);

    setWindowTitle("");
    setupFsBrowsing();
    readSettings(this);
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
            qApp->flush();
        }
    }
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

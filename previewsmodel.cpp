#include "previewsmodel.h"
#include "custom_roles.h"
#include "logic/theapi.h"

#include <vector>
#include <algorithm>

#include <QDirIterator>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QVector>
#include <QDebug>
#include <QCollator>
#include <QApplication>
#include <QTimer>
#include <QLabel>
#include <QMouseEvent>
#include <QDesktopServices>
#include <QApplication>
#include "mainwindow.h"

#include <QDebug>
#include <queue>

//----------------------------------------------------------------------------------------------------------------------------
//----------------------CONFIG------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
enum class DelegateMode {IMAGE_PREVIEW, IMAGE_META, CHECKBOX, FILE_HYPERLINK};

struct SectionDescr //just in case I will want some more static data later
{
    QString text;
    DelegateMode mode;
    QString tooltip;
};

const static std::vector<SectionDescr> captions =
{
    {QObject::tr("Preview"),   DelegateMode::IMAGE_PREVIEW,  QObject::tr("Click to zoom")},
    {QObject::tr("Info"),      DelegateMode::IMAGE_META,     QObject::tr("EXIF if present in image")},
    {QObject::tr("Use"),       DelegateMode::CHECKBOX,       QObject::tr("Lock for processing.")},
    {QObject::tr("File Name"), DelegateMode::FILE_HYPERLINK, QObject::tr("Click to start external viewer.")},
};

//todo: add more file formats (should be supported by Qt)
const static QStringList supportedExt =
{
    "jpeg",
    "jpg",
    "png",
    "ppm",
    "bmp",
    "pbm",
    "pgm",
    "xbm",
    "xpm"
};

//----------------------------------------------------------------------------------------------------------------------------
//----------------------MODEL-------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

PreviewsModel::PreviewsModel(QObject *parent)
    : QAbstractTableModel(parent),
      listFiles(nullptr),
      loadPreviews(nullptr),
      listMut()
{
    connect(this, &QAbstractTableModel::modelReset, this, [this]()
    {
        //qDebug() << "Previews model-reset";
        loadPreviews = utility::startNewRunner([this](auto stop)
        {
            using namespace std::literals;

            // qDebug() << "Previews started load";
            emit this->startedPreviewsLoad();
            std::this_thread::sleep_for(50ms);


            const static QVector<int> roles = {Qt::DisplayRole};

            size_t sz = 0;
            {
                //we still need all those lock_guards, because 3 threads access it, 2 are serialized (list files, load files) but 3rd is GUI and it is not
                std::lock_guard<decltype (listMut)> grd(listMut);
                sz = modelFiles.size();
            }

            if (sz) //don't div by zero!
            {
                const double mul = 100. / sz; //some optimization of the loop
                const auto work = [&stop](){return !(*stop);}; //operations may take significant time during it thread could be stopped, so want to check latest always

                for (decltype(sz) i = 0; i < sz && work(); ++i)
                {
                    bool loaded = false;
                    {
                        //we still need all those lock_guards, because 3 threads access it, 2 are serialized (list files, load files) but 3rd is GUI and it is not
                        std::lock_guard<decltype (listMut)> grd(listMut);
                        if (sz != modelFiles.size())
                            break;
                        loaded   = modelFiles.at(i).loadPreview();
                    }

                    if (loaded && work()) //stop check is important here, or GUI may stack if thread interrupted and signal is out
                    {
                        //updating preview
                        QModelIndex k = this->index(static_cast<int>(i), 0);
                        emit this->dataChanged(k, k, roles);

                        std::this_thread::sleep_for(25ms); //allowing gui to process items
                    }

                    if (i)
                    {
                        if (i % 10 == 0 && work())
                            emit this->loadProgress(i * mul);

                        if (i % 40 == 0 && work())
                        {
                            IMAGE_LOADER.gc(true); //because of the hard pressure of loading many files for preview, need to cleanse cache asap
                            if (work())
                                std::this_thread::sleep_for(100ms);
                        }
                    }
                }
                if (work())
                    emit this->finishedPreviewsLoad();
            }
            //qDebug() << "Previews loaded" << modelFiles.size();
        });
    }, Qt::QueuedConnection);
}

QVariant PreviewsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant res;
    if (orientation == Qt::Horizontal)
    {
        if (section > -1)
        {
            auto index = static_cast<size_t>(section);
            if (role == Qt::DisplayRole)
                res = captions.at(index).text;
        }
    }
    else
    {
        //vertical numbers shown (left-side)
        if (role == Qt::DisplayRole)
            res = section + 1;
    }
    return res;
}

bool PreviewsModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role))
    {
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}

int PreviewsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    std::lock_guard<decltype (listMut)> grd(const_cast<PreviewsModel*>(this)->listMut);
    return static_cast<int>(modelFiles.size());
}

int PreviewsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(captions.size());
}

QVariant PreviewsModel::data(const QModelIndex &index, int role) const
{
    QVariant res;
    if (index.isValid())
    {
        int col  = index.column();
        size_t row = static_cast<decltype (row)>(index.row());
        std::lock_guard<decltype (listMut)> grd(const_cast<PreviewsModel*>(this)->listMut);

        const auto& itm = modelFiles.at(row);
        const auto col_mode = captions.at(col).mode;

        if (role == MyGetPathRole)
            res = itm.filePath;

        if (role == Qt::DisplayRole)
        {

            switch (col_mode)
            {
                case DelegateMode::FILE_HYPERLINK:
                    res = itm.filePath;
                    break;
                case DelegateMode::IMAGE_PREVIEW:
                    res = itm.getPreview();
                    break;
                case DelegateMode::IMAGE_META:
                    res = itm.getPreviewInfo();
                    break;
                case DelegateMode::CHECKBOX:
                    break;
            }

        }

        if (role == Qt::CheckStateRole)
        {
            if (col_mode == DelegateMode::CHECKBOX)
                res = (itm.selected)?Qt::Checked:Qt::Unchecked;
        }

        if (role == MyMouseCursorRole)
        {
            if (col_mode == DelegateMode::FILE_HYPERLINK || col_mode == DelegateMode::IMAGE_PREVIEW)
                return static_cast<int>(Qt::PointingHandCursor);
        }

        if (role == Qt::ToolTipRole)
            return captions.at(col).tooltip;
    }
    return res;
}

bool PreviewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && captions.at(index.column()).mode == DelegateMode::CHECKBOX)
    {
        size_t row = static_cast<decltype (row)>(index.row());
        {
            std::lock_guard<decltype (listMut)> grd(const_cast<PreviewsModel*>(this)->listMut);
            auto& itm = modelFiles.at(row);
            itm.selected = value == Qt::Checked;
        }
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }

    return false;
}

Qt::ItemFlags PreviewsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || captions.at(index.column()).mode != DelegateMode::CHECKBOX)
        return Qt::NoItemFlags;

    return Qt::ItemIsUserCheckable | QAbstractTableModel::flags(index);
}

void PreviewsModel::setCurrentFolder(const QString &path, bool recursive)
{
    static QStringList filter;

    if (filter.isEmpty())
    {
        for (const auto& s : supportedExt)
        {
            filter << "*."+s.toLower();
            filter << "*."+s.toUpper();
        }
    }

    loadPreviews.reset();
    listFiles.reset();



    listFiles = utility::startNewRunner([this, path, recursive](auto stop)
    {
        std::vector<QFileInfo> pathes;
        pathes.reserve(500);

        QDir dir(path, QString(), QDir::Name | QDir::IgnoreCase, QDir::Files);
        QDirIterator it(dir.absolutePath(), filter, QDir::Files, (recursive)?QDirIterator::Subdirectories:QDirIterator::NoIteratorFlags);
        while (it.hasNext() && !(*stop))
        {
            it.next();
            if (!it.fileInfo().isDir())
                pathes.push_back(QFileInfo(it.filePath()));

            if (pathes.capacity() - 5 < pathes.size())
                pathes.reserve(pathes.capacity() + 50);
        }

        if (*stop)
            pathes.clear(); //forced interrupted
        else
        {
            this->beginResetModel();
            this->haveFilesList(pathes, stop);
            this->endResetModel();
        }
    });
}

void PreviewsModel::simulateModelReset()
{
    beginResetModel();
    endResetModel();
}

PreviewsModel::~PreviewsModel()
{
    listFiles.reset();
    loadPreviews.reset();
}

void PreviewsModel::haveFilesList(const PreviewsModel::files_t &list, const utility::runnerint_t &stop)
{
    std::lock_guard<decltype (listMut)> grd(listMut);

    const auto sort_files = [this]()
    {
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);

        std::sort(modelFiles.begin(), modelFiles.end(), [&collator](const auto& i1, const auto& i2)
        {
            return collator.compare(i1.filePath, i2.filePath) < 0;
        });

    };


    if (modelFiles.size())
        modelFiles.erase(std::remove_if(modelFiles.begin(), modelFiles.end(), [](const auto& v)
        {
            return !v.selected;
        }), modelFiles.end());
    modelFiles.reserve(modelFiles.size() + list.size());

    if (!*stop)
        sort_files();

    auto old = std::end(modelFiles);

    for (const auto& fi : list)
    {
        if (*stop)
            break;
        auto s = fi.absoluteFilePath();
        if (std::find_if(modelFiles.begin(), old, [&s](const auto& v){return v.filePath == s;}) == old)
            modelFiles.emplace_back(s);
    }
    if (!*stop)
        sort_files();
    else
        modelFiles.clear();
    //qDebug() << "Files listed "<<modelFiles.size();
}

//----------------------------------------------------------------------------------------------------------------------------
//----------------------DELEGATE----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

bool PreviewsDelegate::showLastClickedPreview(int shift, bool reset)
{
    bool res = false;
    if (lastClickedPreview.isValid())
    {
        QModelIndex ind = lastClickedPreview.model()->index(lastClickedPreview.row() + shift, lastClickedPreview.column());
        if ((res = ind.isValid()))
            THEAPI.showPreview(ind.data(MyGetPathRole).toString(), reset);
    }
    return res;
}

void PreviewsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QLabel label;
    auto draw = [&label, painter, &option](const QRect& g)
    {
        label.setGeometry(g);
        painter->translate(option.rect.topLeft());
        label.render(painter);
        painter->translate(-option.rect.topLeft());
    };

    if (index.isValid())
        switch (captions.at(index.column()).mode)
        {
            case DelegateMode::IMAGE_PREVIEW:
                if (index.data().canConvert<QImage>())
                {
                    QPixmap img = QPixmap::fromImage(qvariant_cast<QImage>(index.data()));
                    label.setPixmap(img);
                    //if we have different sized images need to ensure we will not stretch
                    QRect rect = option.rect;
                    rect.setHeight(std::min(img.height(), rect.height()));
                    rect.setWidth(std::min(img.width(), rect.width()));
                    draw(rect);
                    break;
                }
            case DelegateMode::FILE_HYPERLINK:
            {
                label.setText(QString("<a href='file://%1'>%1</a>").arg(index.data().toString()));
                label.setTextFormat(Qt::RichText);
                label.setStyleSheet("QLabel { background-color : transparent; }");
                draw(option.rect);
            }
                break;
            case DelegateMode::IMAGE_META:
            case DelegateMode::CHECKBOX:
                QStyledItemDelegate::paint(painter, option, index);
                break;
        }
}

QSize PreviewsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    if (index.isValid())
    {
        switch (captions.at(index.column()).mode)
        {
            case DelegateMode::IMAGE_PREVIEW:
                if (index.data().canConvert<QImage>())
                {
                    auto img = qvariant_cast<QImage>(index.data());
                    sz = img.size();
                    break;
                }
            default:
                break;
        }
    }
    return sz;
}

bool PreviewsDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    auto me = dynamic_cast<QMouseEvent*>(event);
    if (me && index.isValid())
    {
        if (me->button() == Qt::LeftButton && me->type() == QEvent::MouseButtonRelease)
        {
            if (captions.at(index.column()).mode == DelegateMode::FILE_HYPERLINK)
            {
                QDesktopServices::openUrl(QString("file://%1").arg(index.data().toString()));
                return true;
            }

            if (captions.at(index.column()).mode == DelegateMode::IMAGE_PREVIEW)
            {
                lastClickedPreview = index;
                showLastClickedPreview();
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

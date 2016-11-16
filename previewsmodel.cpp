#include "previewsmodel.h"
#include <vector>
#include <QDirIterator>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QVector>
#include <QDebug>
#include <QCollator>

struct SectionDescr //just in case I will want some more static data later
{
    QString text;
};

const static std::vector<SectionDescr> captions =
{
    {QObject::tr("Preview")},
    {QObject::tr("Use")},
    {QObject::tr("Name")}
};

const static QStringList supportedExt =
{
    "jpeg",
    "jpg",
    "png",
    "ppm"
};

PreviewsModel::PreviewsModel(QObject *parent)
    : QAbstractTableModel(parent),
      listFiles(nullptr),
      loadPreviews(nullptr),
      listMut()
{
    connect(this, &QAbstractTableModel::modelReset, this, [this]()
    {
        loadPreviews = utility::startNewRunner([this](auto stop)
        {
            using namespace std::literals;
            int ind = 0;
            const static QVector<int> roles = {Qt::DisplayRole};
            for (auto& i : this->modelFiles)
            {
                if (*stop)
                    break;
                if (i.loadPreview(listMut))
                {
                    auto k = this->index(ind, 0);
                    emit this->dataChanged(k, k, roles);
                }
                ++ind;
                if ( ind % 30 == 0)
                    std::this_thread::sleep_for(150ms);
            }
        });
    }, Qt::QueuedConnection);
}

QVariant PreviewsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant res;
    // FIXME: Implement me!
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
        if (role == Qt::DisplayRole)
            res = section;
    }
    return res;
}

bool PreviewsModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        // FIXME: Implement me!
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}

int PreviewsModel::rowCount(const QModelIndex &parent) const
{
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

        if (role == Qt::DisplayRole)
        {

            if (col == 2)
                res = itm.filePath;

            if (col == 0)
            {
                res = itm.getPreview();
            }
        }

        if (role == Qt::CheckStateRole)
        {
            if (col == 1)
                res = (itm.selected)?Qt::Checked:Qt::Unchecked;
        }
    }
    return res;
}

bool PreviewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole && index.column() == 1)
    {
        size_t row = static_cast<decltype (row)>(index.row());
        std::lock_guard<decltype (listMut)> grd(const_cast<PreviewsModel*>(this)->listMut);
        auto& itm = modelFiles.at(row);
        itm.selected = value == Qt::Checked;
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }

    return false;
}

Qt::ItemFlags PreviewsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || index.column() != 1)
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
    listFiles = utility::startNewRunner([this, path, recursive](auto stop)
    {
        std::vector<QFileInfo> pathes; pathes.reserve(500);

        QDir dir(path, QString(), QDir::Name | QDir::IgnoreCase, QDir::Files);
        QDirIterator it(dir.absolutePath(), filter, QDir::Files, (recursive)?QDirIterator::Subdirectories:QDirIterator::NoIteratorFlags);
        while (it.hasNext() && !*stop)
        {
            it.next();
            if (!it.fileInfo().isDir())
                pathes.push_back(QFileInfo(it.filePath()));

            if (pathes.capacity() - 5 < pathes.size())
                pathes.reserve(pathes.capacity() + 50);
        }

        if (*stop)
            pathes.clear(); //forced interrupted

        this->haveFilesList(pathes);
    });
}

PreviewsModel::~PreviewsModel()
{
    listFiles.reset();
    loadPreviews.reset();
}

void PreviewsModel::haveFilesList(const PreviewsModel::files_t &list)
{
    std::lock_guard<decltype (listMut)> grd(listMut);
    beginResetModel();
    if (modelFiles.size())
        modelFiles.erase(std::remove_if(modelFiles.begin(), modelFiles.end(), [](const auto& v){return !v.selected;}), modelFiles.end());
    modelFiles.reserve(modelFiles.size() + list.size());
    for (const auto& fi : list)
    {
        modelFiles.emplace_back(fi.absoluteFilePath());
    }

    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(modelFiles.begin(), modelFiles.end(), [&collator](const auto& i1, const auto& i2)
    {
        return collator.compare(i1.filePath, i2.filePath) < 0;
    });

    endResetModel();
}

void PreviewsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.data().canConvert<QImage>())
    {
        auto img = qvariant_cast<QImage>(index.data());

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());
        painter->drawImage(option.rect, img);

    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize PreviewsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz;
    if (index.data().canConvert<QImage>())
    {
        auto img = qvariant_cast<QImage>(index.data());
        sz = img.size();
    }
    else
        sz = QStyledItemDelegate::sizeHint(option, index);
    return sz;
}

#include "previewsmodel.h"
#include "custom_roles.h"
#include "logic/theapi.h"

#include <vector>
#include <queue>
#include <set>
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
#include <QComboBox>
#include <QDesktopServices>
#include <QApplication>
#include "mainwindow.h"

#include <QDebug>


#include "utils/no_const.h"
#include "utils/strutils.h"
#include "utils/cont_utils.h"

//----------------------------------------------------------------------------------------------------------------------------
//----------------------CONFIG------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//I think it is 1st step to re-invent qt-quick but C++ based >: ...hate JS

enum class DelegateMode {IMAGE_PREVIEW, IMAGE_META, CHECKBOX, FILE_HYPERLINK, FIXED_COMBO_BOX};


using create_widget_t = std::function<QWidget*(QWidget* parent, const QStyleOptionViewItem &option, const QModelIndex& index)>;
const static create_widget_t create_widget_t_defimpl = [](auto, const auto&, const auto&)noexcept{return nullptr;};

static bool isEditable(DelegateMode mode)
{
    return mode != DelegateMode::FILE_HYPERLINK && mode != DelegateMode::IMAGE_PREVIEW && mode != DelegateMode::IMAGE_META;
}

//section of the preview table description, code below will build and process according declaration here
struct sectiondescr_t
{
    const QString text;
    const DelegateMode mode; //determinates how it will be processed, not special things are just stored/returned as variants
    const QString tooltip;
    const create_widget_t editor; //widget which allows to choose values, like combobox
    const QVariant initialValue; //initial default value for things like combobox
};

//texts can be translated, but for lua need to have some fixed values
struct fileroles_t
{
    const QString humanRole;
    const int     luaRole;
};

//order is important here, lua-generator below relays on it
//PreviewsModel::guessDarks() relays on it
const static std::vector<fileroles_t> fileRoles =
{
    {QObject::tr("Stack source"), 0}, //must be 1st (1st will be set as default)
    {QObject::tr("Ignored"), -1},
    {QObject::tr("Dark"), 1},
};




//if you change order in arrays here, something below may break bcs assumes fixed index
//PreviewsModel::guessDarks() relays on it
const static std::vector<sectiondescr_t> captions =
{
    {QObject::tr("Preview"),   DelegateMode::IMAGE_PREVIEW,  QObject::tr("Click to zoom"),                   create_widget_t_defimpl, QVariant()},
    {QObject::tr("Info"),      DelegateMode::IMAGE_META,     QObject::tr("EXIF if present in image"),        create_widget_t_defimpl, QVariant()},
    {QObject::tr("Role"),      DelegateMode::FIXED_COMBO_BOX,QObject::tr("Select usage for this image."), //dont forget update keepInList() method
     [](QWidget* parent, const QStyleOptionViewItem &option, const QModelIndex& index){
         Q_UNUSED(option);
         Q_UNUSED(index);
         QComboBox *editor = new QComboBox(parent);
         editor->setEditable(false);
         for (const auto& i : fileRoles) editor->addItem(i.humanRole);
         return editor;
     }, 0 //FIXED_COMBO_BOX stores indexes in model
    },
    {QObject::tr("File Name"), DelegateMode::FILE_HYPERLINK, QObject::tr("Click to start external viewer."), create_widget_t_defimpl, QVariant()},
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

#ifdef USING_VIDEO_FS
const static std::set<QString> supportedVids = {
    "mov",
    "mp4",
    "avi",
};
#endif

template <class T>
bool isDark(const T& path)
{
    //words filter to guess darks
    const static std::vector<QString> termsDark = {
        //use lowercase here
        "dark",
    };
    auto src = utility::toLower(path);
    return utility::strcontains(src, termsDark);
}

//----------------------------------------------------------------------------------------------------------------------------
//----------------------MODEL-------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
void PreviewsModel::generateLuaCode(std::ostream &out) const
{
    std::lock_guard<decltype (listMut)> (utility::noconst(this)->listMut);//that will break code if original model object is created as const, but who will do it ?!
    //generating lua code from internal state, hardly bound to arrays above (to their indexes, values, etc)
    out << "local guiSelectedFiles = {";
    for (const auto& file : modelFiles)
    {
        const auto& role = fileRoles.at(file.getValue(2, captions.at(2).initialValue).toInt());
        if (role.luaRole > -1)
        {
            out << "\n\t{\n\t\tfileName='"<<file.filePath.toStdString()<<"',\n\t\tfileMode="<<role.luaRole<< ", --" << role.humanRole.toStdString() << "\n\t},";
        }
    }
    out << "\n}" << std::endl;
}

PreviewsModel::PreviewsModel(QObject *parent)
    : QAbstractTableModel(parent),
      luavm::LuaGenerator(),
      listFiles(nullptr),
      loadPreviews(nullptr),
      listMut()
{
    connect(this, &QAbstractTableModel::modelReset, this, [this]()
    {
        //it's kinda limitation of qt-gui, same thread cannot list files & load files, bcs list is model reset and load is fillup of the model,
        //if we do in 1 thread, it will not be smooth, i.e. all empty until full load

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
                        //if cannot load preview - make it ignored (happened with 0th frame of the vids for me)
                        if (modelFiles.at(i).brokenPreview)
                            this->setData(this->index(i, 2), 1, Qt::EditRole);
                        else
                        {
                            //updating preview
                            QModelIndex k = this->index(static_cast<int>(i), 0);
                            emit this->dataChanged(k, k, roles);
                            std::this_thread::sleep_for(25ms); //allowing gui to process items
                        }
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
    std::lock_guard<decltype (listMut)> grd(utility::noconst(this)->listMut);
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
        std::lock_guard<decltype (listMut)> grd(utility::noconst(this)->listMut);

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
                case DelegateMode::FIXED_COMBO_BOX:
                {
                    //dumping string list from combobox just created and storing for future use in this column
                    //so u can add more columns
                    if (!fixedCombosLists.count(col))
                    {
                        QStringList items;
                        auto p1 = captions.at(col).editor(nullptr, QStyleOptionViewItem(), index);
                        QComboBox *cp = qobject_cast<QComboBox*>(p1);
                        if (cp)
                        {
                            for (int i = 0; i < cp->count(); ++i)
                                items << cp->itemText(i);
                        }
                        if (p1)
                            delete p1;
                        utility::noconst(this)->fixedCombosLists[col] = items;
                    }
                    const auto& sli = fixedCombosLists.at(col);
                    int index = itm.getValue(col, captions.at(col).initialValue).toInt();
                    if (index >-1 && index < sli.size())
                        res =  sli.at(index);
                }
                    break;
                default:
                    res = itm.getValue(col, captions.at(col).initialValue);
                    break;
            }

        }

        if (role == Qt::EditRole && isEditable(col_mode))
        {
            res = itm.getValue(col, captions.at(col).initialValue);
        }

        if (role == Qt::CheckStateRole)
        {
            if (col_mode == DelegateMode::CHECKBOX)
            {
                res = (itm.getValue(col, false))?Qt::Checked:Qt::Unchecked;
            }
        }

        if (role == MyMouseCursorRole)
        {
            //if (col_mode == DelegateMode::FILE_HYPERLINK || col_mode == DelegateMode::IMAGE_PREVIEW)
            if (col_mode == DelegateMode::IMAGE_PREVIEW)
                return static_cast<int>(Qt::PointingHandCursor);
        }

        if (role == Qt::ToolTipRole)
            return captions.at(col).tooltip;
    }
    return res;
}

bool PreviewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    bool changed = false;
    if (index.isValid())
    {
        const int col    = index.column();
        const auto mode = captions.at(col).mode;
        auto set = [&col, &index, &changed, this](const QVariant& v)
        {
            size_t row = static_cast<decltype (row)>(index.row());
            std::lock_guard<decltype (this->listMut)> grd(this->listMut);
            auto& itm = modelFiles.at(row);
            itm.valuesPerColumn[col] = v;
            changed = true;
        };

        if (role == Qt::CheckStateRole && mode == DelegateMode::CHECKBOX)
        {
            set(value == Qt::Checked);
        }

        if (role == Qt::EditRole && isEditable(mode))
        {
            set(value);
        }

        if (changed)
            emit dataChanged(index, index, QVector<int>() << role);
    }
    return changed;
}


void PreviewsModel::guessDarks()
{
    //trying to guess dark frames according to path name, which should contain "dark" word
    for (int row = 0, size = rowCount(); row < size; ++row)
    {
        const auto& itm = modelFiles.at(static_cast<size_t>(row));
        if (isDark(itm.getFilePath()))
            setData(index(row, 2), 2, Qt::EditRole);
    }
}

Qt::ItemFlags PreviewsModel::flags(const QModelIndex &index) const
{
    QFlags<Qt::ItemFlag> res = Qt::NoItemFlags;

    if (index.isValid())
    {
        auto mode = captions.at(static_cast<size_t>(index.column())).mode;
        if (mode == DelegateMode::CHECKBOX)
            res = Qt::ItemIsUserCheckable;
        else
        {
            if (isEditable(mode))
                res = Qt::ItemIsEditable;
        }
        res = res | QAbstractTableModel::flags(index);
    }
    return res;
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
#ifdef USING_VIDEO_FS
        for (const auto& s: supportedVids)
        {
            filter << "*."+s.toLower();
            filter << "*."+s.toUpper();
        }
#endif
    }

    loadPreviews.reset();
    listFiles.reset();

    listFiles = utility::startNewRunner([this, path, recursive](auto stop)
    {
        using namespace utility;
        std::vector<QFileInfo> pathes;
        std::vector<QString>  subfolders;
        subfolders.reserve(50);
        pathes.reserve(500);
        QDir dir(path);
        const auto absPath = dir.absolutePath();

        //if not recursive we still must add "darks" recursive
        if (!recursive)
        {
            QDirIterator directories(absPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(directories.hasNext())
            {
                directories.next();
                subfolders.push_back(directories.filePath());
            }
        }
        const static QString cachef(VFS_CACHED_FOLDER);
        QDirIterator it(absPath, filter, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && !(*stop))
        {
            it.next();
            if (!it.fileInfo().isDir())
            {
                bool push = recursive;

                //darks must be always listed, even if "non-recursive" mode is, so "auto-guess" can work and it is more user friendly
                if (!push)
                    push = !strcontains(it.filePath(), subfolders) || isDark(it.filePath());

                push = push && !strcontains(it.filePath(), cachef); //excluding own cache from the list, will access it using regular URI

                if (push)
                    pathes.push_back(QFileInfo(it.filePath()));
            }

            if (pathes.capacity() - 5 < pathes.size())
                pathes.reserve(static_cast<size_t>(pathes.capacity() * 1.25 + 10));
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

    modelFiles.clear();
    modelFiles.reserve(list.size());

    for (const auto& fi : list)
    {
        if (*stop)
            break;
        auto s = fi.absoluteFilePath();

#ifdef USING_VIDEO_FS
        if (supportedVids.count(fi.suffix().toLower()))
        {
            auto frames = IMAGE_LOADER.getVideoFramesLinks(s);
            if (frames.size())
                for (const auto& f : frames)
                {
                    if (*stop)
                        break;
                    modelFiles.emplace_back(f);
                }
        }
        else
#endif
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
                }
                break;
            case DelegateMode::FILE_HYPERLINK:
            {
                label.setText(QString("&nbsp;&nbsp;<a href='file://%1'>%1</a>").arg(index.data().toString()));
                label.setTextFormat(Qt::RichText);
                label.setStyleSheet("QLabel { background-color : transparent; }");
                draw(option.rect);
            }
                break;
            default:
                QStyledItemDelegate::paint(painter, option, index);
                break;
        }
}

QSize PreviewsDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sz = QStyledItemDelegate::sizeHint(option, index);
    sz.setWidth(std::max(50, sz.width()));
    if (index.isValid())
    {
        switch (captions.at(index.column()).mode)
        {
            case DelegateMode::IMAGE_PREVIEW:
                if (index.data().canConvert<QImage>())
                {
                    auto img = qvariant_cast<QImage>(index.data());
                    sz = img.size();
                }
                break;
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
                QDesktopServices::openUrl(QString("file://%1").arg(IMAGE_LOADER.getFileLinkForExternalTools(index.data().toString())));
                return true;
            }

            if (captions.at(index.column()).mode == DelegateMode::IMAGE_PREVIEW)
            {
                lastClickedPreview = index;
                showLastClickedPreview();
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QWidget *PreviewsDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget *res = nullptr;
    if (index.isValid())
    {
        res = captions.at(index.column()).editor(parent, option, index);
        if (res)
        {
            res->setFocusPolicy(Qt::StrongFocus); //otherwise tableview will get mouse events
            //res->setAutoFillBackground(true); //if commented, view's background will be used
        }
    }
    return res;
}

void PreviewsDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.isValid() && editor)
    {
        if (captions.at(index.column()).mode == DelegateMode::FIXED_COMBO_BOX)
        {
            QComboBox *p = qobject_cast<QComboBox*>(editor);
            if (p)
            {
                int ind= index.data(Qt::EditRole).toInt();
                p->setCurrentIndex(ind);
            }
        }
    }
}

void PreviewsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    Q_UNUSED(model);
    if (index.isValid() && editor)
    {
        if (captions.at(index.column()).mode == DelegateMode::FIXED_COMBO_BOX)
        {
            QComboBox *p = qobject_cast<QComboBox*>(editor);
            if (p && model)
            {
                model->setData(index, p->currentIndex());
            }
        }
    }
}

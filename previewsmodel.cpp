#include "previewsmodel.h"
#include "custom_roles.h"

#include <vector>
#include <queue>
#include <set>
#include <map>

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
#include <QKeySequence>
#include <QKeyEvent>
#include <QMenu>
#include "utils/no_const.h"
#include "utils/strutils.h"
#include "utils/cont_utils.h"
#include "config_ui/globalsettings.h"
#include "fixedcombowithshortcut.h"
#include "../customtableview.h"
#include "lua/luasrc/customlua.h"
#include "lua/lua_params.h"
#include "utils/palgorithm.h"

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

//order is important here, lua-generator below relays on it
//PreviewsModel::guessDarks() relays on it
const static FileRolesList fileRoles =
{
    //1st will be set as default, also GUI actions are bound to index in this list
    {QObject::tr("Ignored"), 1, QKeySequence("-")},
    {QObject::tr("Source"), 0, QKeySequence("=")},
    {QObject::tr("Dark"), 2, QKeySequence("0")},
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
         FixedComboWithShortcut *editor = new FixedComboWithShortcut(parent);
         editor->setEditable(false);
         for (const auto& i : fileRoles) editor->addItemWithShortcut(i.humanRole, i.seq);

         return editor;
     }, 0 //FIXED_COMBO_BOX stores indexes in model
    },
    {QObject::tr("File Name"), DelegateMode::FILE_HYPERLINK, QObject::tr("Click to start external viewer."), create_widget_t_defimpl, QVariant()},
};

//todo: add more file formats (should be supported by Qt)
const static QStringList supportedExt
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
const QStringList supportedVids
{
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


constexpr static int64_t previews_half_range = 10;

//----------------------------------------------------------------------------------------------------------------------------
//----------------------MODEL-------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
void PreviewsModel::generateProjectCode(std::ostream &out) const
{
    std::lock_guard<decltype (listMut)> grd(listMut);
    //generating lua code from internal state, hardly bound to arrays above (to their indexes, values, etc)

    const auto spid = static_cast<size_t>(getFileRoleColumnId());
    QString path = (currentFolder.isDir())?currentFolder.canonicalFilePath():currentFolder.canonicalPath();
    out << "guiSelectedFilesBase = '" << currentFolder.canonicalFilePath().toStdString()<<"'"<<std::endl;
    QDir dir(path);

    out << "guiSelectedFilesList = {";
    for (const auto& file : modelFiles)
    {
        const auto& role = fileRoles.at(file.getValue(spid, captions.at(spid).initialValue).toInt());
        //out << "\n\t{\n\t\tfileName=utf8.format('%s/%s', guiSelectedFilesBase, '"<<dir.relativeFilePath(file.filePath).toStdString()<<"'),\n\t\tfileMode="<<role.luaRole<< ", --" << role.humanRole.toStdString() << "\n\t},";
        //lets append things on load from C++ code, so user has less control over how they can break it
        if (1 == role.luaRole)
            continue;
        QString fpr = file.filePath;
        fpr.remove(QString("%1://").arg(VFS_PREFIX));
        fpr.remove(QString("%1:/").arg(VFS_PREFIX));
        fpr = "/" + fpr;
        out << "\n\t{\n\t\tfileName = '"<<dir.relativeFilePath(fpr).toStdString()<<"',\n\t\tfileMode = "<<role.luaRole<< ", --" << role.humanRole.toStdString() << "\n\t},";
    }
    out << "\n}" << std::endl;
}

void PreviewsModel::loadProjectCode(const std::string &src)
{
    using namespace luavm;
    using namespace lua_templ;
    using internal_map = std::map<QString, QString>;

    static std::map<int64_t, int> lua2index_map;
    if (!lua2index_map.size())
    {
        for (size_t i = 0, sz = fileRoles.size(); i < sz; ++i)
            lua2index_map[fileRoles.at(i).luaRole] = static_cast<int>(i);
    }

    std::lock_guard<decltype (listMut)> grd(listMut);
    auto conn = std::make_shared<QMetaObject::Connection>();
    auto vm   = std::make_shared<luavm::LuaVM>();

    vm->doString(src);
    auto root = testGetGlobal<QString>(*vm, "guiSelectedFilesBase");

    *conn = connect(this, &PreviewsModel::filesAreListed, this, [this, conn, vm, root](const QString& dir)
    {
        //here is 3rd stage: gui starts list -> thread does list -> gui need update list from lua
        std::lock_guard<decltype (listMut)> grd(listMut);
        lua_State *L = *vm;
        int stack = lua_gettop(L);
        auto proj = testGetGlobal<std::vector<internal_map>>(L, "guiSelectedFilesList");
        for (const auto& itm : proj)
        {
            if (itm.count("fileName") && itm.count("fileMode"))
            {
                auto fn = itm.at("fileName");
                const static QString pref = QString("%1://").arg(VFS_PREFIX);
                auto fp   = QString("%3%1/%2").arg(dir).arg(fn).arg((fn.contains("#")?pref:""));
                int  mode = itm.at("fileMode").toInt();
                if (lua2index_map.count(mode))
                    setRoleForPriv(fp, lua2index_map.at(mode));
            }
        }
        if (stack!=lua_gettop(L))
            FATAL_RISE("Broken stack on project loading.");

        disconnect(*conn); //self-deleting, releasing captures
        emit loadedProject(root, false);

    }, Qt::QueuedConnection); //important, resolves cross-thread

    setCurrentFolder(root, false); //fixme: recursivness is not stored, do some signal/slot to reflect that in GUI
}

int PreviewsModel::getFileRoleColumnId() //dunno how to name it really
{
    return 2;
}

const static QVector<int> roles = {Qt::DisplayRole};

PreviewsModel::PreviewsModel(QObject *parent)
    : QAbstractTableModel(parent),
      luavm::ProjectSaver(),
      listFiles(nullptr),
      loadPreviews(nullptr),
      listMut(),
      modelFiles(),
      urgentRowScrolled(0),
      modelFilesAmount(0),
      scrollDelayedLoader(),
      currentFolder(),
      bestPicker(nullptr)
{
    /*
     * QObject::connect: Cannot queue arguments of type 'Qt::Orientation'
(Make sure 'Qt::Orientation' is registered using qRegisterMetaType().)
     * */
    qRegisterMetaType<Qt::Orientation>("Qt::Orientation");

    connect(this, &QAbstractTableModel::modelReset, this, [this]()
    {
        //it's kinda limitation of qt-gui, same thread cannot list files & load files, bcs list is model reset and load is fillup of the model,
        //if we do in 1 thread, it will not be smooth, i.e. all empty until full load
        bestPicker.reset();
        emit this->startedPreviewsLoad(true);
        urgentRowScrolled = 0;
        loadCurrentInterval();
    }, Qt::QueuedConnection);

    scrollDelayedLoader.setSingleShot(true);
    connect(&scrollDelayedLoader, &QTimer::timeout, this, &PreviewsModel::onTimerDelayedLoader, Qt::QueuedConnection);

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
            {
                const auto& t = captions.at(index).text;
                if (index == 0 && modelFilesAmount > 0)
                    res = QString("%1 (%2)").arg(t).arg(modelFilesAmount);
                else
                    res = t;
            }
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
    return static_cast<int>(modelFilesAmount);
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
        if (index.row() >= modelFilesAmount || index.row() < 0 || index.column() < 0 || index.column() >= static_cast<int>(captions.size()))
            return res;

        size_t col  = static_cast<decltype (col)>(index.column());
        size_t row = static_cast<decltype (row)>(index.row());

        const auto col_mode = captions.at(col).mode;
        if (role == MyScrolledView || role == MyMouseCursorRole)
        {
            utility::noconst(this)->scrolledTo(static_cast<int64_t>(row));
            if (role == MyMouseCursorRole)
            {
                //if (col_mode == DelegateMode::FILE_HYPERLINK || col_mode == DelegateMode::IMAGE_PREVIEW)

                if (col_mode == DelegateMode::IMAGE_PREVIEW)
                    return static_cast<int>(Qt::PointingHandCursor);

            }
            return res;
        }

        if (role == Qt::ToolTipRole)
        {
            return captions.at(col).tooltip;
        }
        PreviewsModelData itm;
        {
            std::lock_guard<decltype (listMut)> grd(listMut);
            if (row >=modelFiles.size())
                return res;
            itm = modelFiles.at(row);
        }
        const bool wasLoaded = itm.isLoaded();

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
                    if (wasLoaded)
                        res = itm.getPreview();
                    break;
                case DelegateMode::IMAGE_META:
                    if (wasLoaded)
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

    }
    return res;
}

bool PreviewsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    std::lock_guard<decltype (this->listMut)> grd(this->listMut);
    bool changed = setDataPriv(index, value, role);
    if (changed)
        emit dataChanged(index, index, QVector<int>() << role);
    return changed;
}

bool PreviewsModel::setDataPriv(const QModelIndex &index, const QVariant &value, int role)
{
    //warning!!! it's not locked here and do not emit "dataChanged"
    bool changed = false;
    if (index.isValid())
    {
        const int col    = index.column();
        const auto mode = captions.at(col).mode;
        auto set = [&col, &index, &changed, this](const QVariant& v)
        {
            size_t row = static_cast<decltype (row)>(index.row());
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
    }
    return changed;
}

//this is "usage role" like "Source, Ignored, Darks"
void PreviewsModel::setRoleForPriv(const QString &fileName, int role_id)
{
    const QVariant val(role_id);
    for (size_t row = 0, sz = modelFiles.size(); row < sz; ++row)
    {
        if (fileName == modelFiles.at(row).getFilePath())
        {
            setDataPriv(index(static_cast<int>(row), getFileRoleColumnId()), val, Qt::EditRole);
            break;
        }
    }
}
//this is "usage role" like "Source, Ignored, Darks"

void PreviewsModel::setRoleFor(const QString &fileName, int role_id)
{
    const static QVector<int> roles {Qt::EditRole};
    std::lock_guard<decltype (listMut)> grd(listMut);
    setRoleForPriv(fileName, role_id);
}

void PreviewsModel::setAllRole(int role_id)
{
    if (role_id > -1 && static_cast<size_t>(role_id) < fileRoles.size())
    {
        const QVariant val(role_id);
        std::lock_guard<decltype (this->listMut)> grd(this->listMut);
        for (int row = 0, size = rowCount(); row < size; ++row)
            setDataPriv(index(row, getFileRoleColumnId()), val, Qt::EditRole);

        emit dataChanged(index(0, getFileRoleColumnId()), index(rowCount() - 1, getFileRoleColumnId()), QVector<int>() << Qt::EditRole);
    }
}

void PreviewsModel::guessDarks()
{
    //trying to guess dark frames according to path name, which should contain "dark" word
    std::lock_guard<decltype (listMut)> grd(listMut);
    const static QVector<int> roles {Qt::EditRole};
    for (int row = 0, size = rowCount(); row < size; ++row)
    {
        const auto& itm = modelFiles.at(static_cast<size_t>(row));
        if (isDark(itm.getFilePath()))
        {
            auto ind = index(row, getFileRoleColumnId());
            if (setDataPriv(ind, 2, Qt::EditRole))
                emit dataChanged(ind, ind, roles);
        }
    }
}


void PreviewsModel::pickBests(const utility::runner_f_t& end_func, const double min_quality)
{
    return pickBests(end_func, 0, static_cast<int>(modelFiles.size()) - 1, min_quality);
}

void PreviewsModel::pickBests(const utility::runner_f_t &end_func, int from, int to, const double min_quality) //zero based indexes of the range
{
    from = std::max(0, from); from = std::min(from, static_cast<decltype(from)>(modelFiles.size()) - 1);
    to   = std::max(0, to);   to   = std::min(to,   static_cast<decltype(to)>(modelFiles.size()) - 1);

    const static auto spid = static_cast<size_t>(getFileRoleColumnId());


    //trying to pick bests amoung
    struct sort_t
    {
        QString file;
        double  weight;
    };

    bestPicker = utility::startNewRunner([this, from, to, end_func, min_quality](auto stop)
    {
        std::vector<sort_t> source;
        {
            std::lock_guard<decltype (listMut)> grd(listMut);

            source.reserve(modelFiles.size());

            for (int i = from; !*stop && i <= to; ++i)
            {
                auto& file = modelFiles.at(i);
                //skipping darks
                if (2 != file.getValue(spid, captions.at(spid).initialValue).toInt())
                {
                    source.push_back({file.getFilePath(), -1});
                }
            }
        }

        if (source.size() && !*stop)
        {
             //using previews because that cache lasts longer, so meta has chance to live in RAM long
            std::atomic<double> min(PREVIEW_LOADER.getMeta(source.at(0).file).precalcs.blureness);
            std::atomic<double> max(min.load());
            std::atomic<size_t> cntr(0);

            //it seems better to use 1 thread here (std::) in terms of HDD pressure. However let em be atomics
            std::find_if(source.begin(), source.end(), [&min, &max, &stop, &cntr](auto& s)->bool
            {
                auto w = s.weight =PREVIEW_LOADER.getMeta(s.file).precalcs.blureness;
                update_maximum(max, w);
                update_minimum(min, w);
                auto v = ++cntr;
                if (!*stop && (v % 10 == 0))
                    std::this_thread::sleep_for(std::chrono::milliseconds(5)); //hdd pressure
                return *stop;
            });
            const double accept = (max - min) * min_quality + min;

            if (!*stop)
            {
                std::lock_guard<decltype (listMut)> grd(listMut);
                ALG_NS::find_if(source.cbegin(), source.cend(), [this, &accept, &stop](const auto& it)->bool
                {
                    //using that fact, that modelFiles and source are in same string order
                    //if we do some other sort of modelFiles later - that may break

                    auto its = modelFiles.begin();
                    its = ALG_NS::find_if(its, modelFiles.end(),[&it, &stop](auto& s)->bool
                    {
                        return *stop || s.getFilePath() == it.file;
                    });

                    if (*stop || its == modelFiles.end())
                        return true;
                    its->valuesPerColumn[spid] = (it.weight >= accept) ? 1 : 0; //here 0-1 are indexes in list Ignore,Source,Dark
                    return *stop;
                });
            }
            if (!*stop) //not sure if we need this if
                simulateModelReset();

        }
        end_func(stop);
    });
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
    loadPreviews.reset();
    listFiles.reset();
    QFileInfo inf = currentFolder = QFileInfo(path);
    IMAGE_LOADER.gc();

    listFiles = utility::startNewRunner([this, recursive, inf](auto stop)
    {
        QString absPath = (inf.isDir())?inf.canonicalFilePath():inf.canonicalPath();

        QStringList filter;
        for (const auto& s : supportedExt)
        {
            filter << "*."+s.toLower();
            filter << "*."+s.toUpper();
        }
#ifdef USING_VIDEO_FS
        if (recursive)
            for (const auto& s : supportedVids)
            {
                filter << "*."+s.toLower();
                filter << "*."+s.toUpper();
            }
#endif
        using namespace utility;
        PreviewsModel::files_t pathes;
        std::vector<QString>   subfolders;
        subfolders.reserve(50);
        pathes.reserve(500);

        //if not recursive we still must add "darks" recursive
        if (!recursive)
        {
            QDirIterator directories(absPath, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while(directories.hasNext())
            {
                directories.next();
                const auto fp = directories.filePath();
                const auto ls = fp.split("/", QString::SkipEmptyParts);
                //darks must be always listed, even if "non-recursive" mode is, so "auto-guess" can work and it is more user friendly
                if (ls.size() && isDark(ls.back()))
                    continue;
                subfolders.push_back(fp);
            }
        }
        if (inf.isDir())
        {
            const static QString cachef(VFS_CACHED_FOLDER);
            QDirIterator it(absPath, filter, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext() && !(*stop))
            {
                it.next();
                if (!it.fileInfo().isDir())
                {
                    bool push = recursive;


                    if (!push)
                        push = !strcontains(it.filePath(), subfolders);

                    push = push && !strcontains(it.filePath(), cachef); //excluding own cache from the list, will access it using regular URI

                    if (push)
                        pathes.push_back(QFileInfo(it.filePath()));
                }

                if (pathes.capacity() - 5 < pathes.size())
                    pathes.reserve(static_cast<size_t>(pathes.capacity() * 1.25 + 10));
            }
        }
        else
            pathes.push_back(inf);

        if (*stop)
            pathes.clear(); //forced interrupted
        else
        {
            this->beginResetModel();
            this->haveFilesList(pathes, stop);
            this->endResetModel();
        }
        emit this->filesAreListed(absPath); //that will be needed by project loader mainly
    });
}

void PreviewsModel::simulateModelReset()
{
    beginResetModel();
    endResetModel();
}

void PreviewsModel::resetModel()
{
    listFiles.reset();
    loadPreviews.reset();

    std::lock_guard<decltype (listMut)> grd(listMut);
    beginResetModel();
    modelFilesAmount = 0;
    modelFiles.clear();
    endResetModel();
}

void PreviewsModel::scrolledTo(int64_t row)
{
    if (std::abs(row - urgentRowScrolled) > static_cast<decltype(previews_half_range)>(0.9 * previews_half_range))
    {
        urgentRowScrolled = row;

        //this + onTimerDelayedLoader() allows fast and smooth list scroll by user
        scrollDelayedLoader.start(StaticSettingsMap::getGlobalSetts().readInt("Int_scroll_delay"));
    }
}

void PreviewsModel::onTimerDelayedLoader()
{
    loadCurrentInterval();
}

PreviewsModel::~PreviewsModel()
{
    listFiles.reset();
    loadPreviews.reset();
}

const FileRolesList &PreviewsModel::getFileRoles()
{
    return fileRoles;
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
    modelFilesAmount = 0;
    modelFiles.reserve(list.size());

    for (const auto& fi : list)
    {
        if (*stop)
            break;
        auto s = fi.absoluteFilePath();

#ifdef USING_VIDEO_FS
        if (supportedVids.contains(fi.suffix().toLower()))
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
    modelFilesAmount = static_cast<int64_t>(modelFiles.size());
    emit headerDataChanged(Qt::Horizontal, 0, 0);
    qDebug() << "Files listed "<<modelFiles.size();
}

void PreviewsModel::loadCurrentInterval()
{
    IMAGE_LOADER.gc();
    loadPreviews = utility::startNewRunner([this](auto stop)
    {
        auto sz   = static_cast<size_t>(modelFilesAmount);
        auto from = static_cast<size_t>(std::max<int64_t>(0,  urgentRowScrolled - previews_half_range));
        auto to   = static_cast<size_t>(std::min<int64_t>(static_cast<int64_t>(sz), urgentRowScrolled + previews_half_range));

        using namespace std::literals;

        //qDebug() << "Previews started load";
        emit this->startedPreviewsLoad(false);
        const auto work = [&stop](){return !(*stop);}; //operations may take significant time during it thread could be stopped, so want to check latest always

        if (sz) //don't div by zero!
        {
            const double mul = 100. / (to - from + 1); //some optimization of the loop
            decltype(sz) total_loaded = 0;
            std::vector<decltype (from)> loaded_indexes;
            loaded_indexes.reserve(to - from + 1);

            for (decltype(sz) i = from; i < to && work(); ++i)
            {
                bool loaded = false;
                {
                    //we still need all those lock_guards, because 3 threads access it, 2 are serialized (list files, load files) but 3rd is GUI and it is not
                    std::lock_guard<decltype (listMut)> grd(listMut);
                    if (sz != modelFiles.size()) //test, if user changed folder - must stop load
                        break;
                    loaded   = modelFiles.at(i).loadPreview();

                    //if cannot load preview - make it ignored (happened with 0th frame of the vids for me)
                    if (modelFiles.at(i).brokenPreview)
                        this->setData(this->index(static_cast<int>(i), 2), 1, Qt::EditRole);
                }
                if (loaded)
                {
                    loaded_indexes.push_back(i);
                    ++total_loaded;
                }

                if (total_loaded)
                {
                    if (total_loaded % 10 == 0 && work())
                        emit this->loadProgress(static_cast<int>(total_loaded * mul));
                }
            }

            for (size_t i = 0, tot = loaded_indexes.size(), start = 0, end = 0; i < tot; ++i)
            {
                bool c1 = loaded_indexes.at(i) - loaded_indexes.at(start) == i - start;
                if (c1)
                {
                    end = i;
                }

                if (!c1 || i + 1 >= tot)
                {
                    QModelIndex si = this->index(static_cast<int>(loaded_indexes.at(start)), 0);
                    QModelIndex ei = this->index(static_cast<int>(loaded_indexes.at(end)), 0);
                    emit this->dataChanged(si, ei, roles);
                    start = end = i;
                    if (work())
                        std::this_thread::sleep_for(25ms);
                }
            }
        }
        if (work())
        {
            emit this->finishedPreviewsLoad();
            IMAGE_LOADER.gc(true); //not sure, maybe better to call it outside IF, but this one works smoother as for me
        }
    });
}

//----------------------------------------------------------------------------------------------------------------------------
//----------------------DELEGATE----------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

bool PreviewsDelegate::showLastClickedPreview(int shift, const QSize& lastSize, QAbstractItemModel *model)
{
    bool res = false;
    if (lastClickedPreview.isValid())
    {
        QModelIndex ind = lastClickedPreview.model()->index(lastClickedPreview.row() + shift, lastClickedPreview.column());
        if ((res = ind.isValid()))
        {
            if (MainWindow::instance())
            {
                //that what happens when we have MVD and want to connect something trivial, like radiobuttons, do I really need to implement view with 3 buttons
                //which can be bound to toolbar ?!?!gosh...hate those ideas in fact
                //(i just want field from "model" be visible as 3 buttons on toolbar, for such simple thing so many code all around)
                QModelIndex index = lastClickedPreview.model()->index(ind.row(), PreviewsModel::getFileRoleColumnId());
                ind.data(MyScrolledView); //tipping model, need to load previews
                const auto fileName = ind.data(MyGetPathRole).toString();
                const auto img      = IMAGE_LOADER.getImage(fileName);
                if (img != nullptr)
                {
                    const bool reset = lastSize != img->size(); //next line (openPreview) will change referenced lastSize to current
                    auto currRole = static_cast<size_t>(index.data(Qt::EditRole).toInt());
                    // qDebug() << "CurrRole: " << currRole;
                    MainWindow::instance()->openPreviewTab(img, fileName, currRole); //fixme: maybe do some signal/ slot for that
                    if (reset)
                        MainWindow::instance()->resetPreview(false);//fixme: maybe do some signal/ slot for that
                }
            }
        }
    }
    else
    {
        if (model)
        {
            lastClickedPreview = model->index(0, 0);
            res = showLastClickedPreview(shift, lastSize, nullptr);
        }
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
    {
        index.data(MyScrolledView);
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
    if (index.isValid())
    {
        auto me = dynamic_cast<QMouseEvent*>(event);
        const auto mode = captions.at(index.column()).mode;
        if (me && me->type() == QEvent::MouseButtonRelease)
        {

            if (me->button() == Qt::LeftButton)
            {
                if (mode == DelegateMode::FILE_HYPERLINK)
                {
                    QDesktopServices::openUrl(QString("file://%1").arg(IMAGE_LOADER.getFileLinkForExternalTools(index.data().toString())));
                    return true;
                }

                if (mode == DelegateMode::IMAGE_PREVIEW)
                {
                    const static QSize reset(-1, -1);
                    lastClickedPreview = index;
                    showLastClickedPreview(0, reset);
                    return true;
                }
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
            res->setFocusPolicy(Qt::StrongFocus);
            //res->setAutoFillBackground(true); //if commented, view's background will be used
            auto tmp = qobject_cast<QComboBox*>(res);
            if (tmp)
            {
                connect(tmp, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                        [this, tmp](int)
                {
                    emit utility::noconst(this)->commitData(tmp);
                    emit utility::noconst(this)->closeEditor(tmp);
                });
            }
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
                p->blockSignals(true);
                p->setCurrentIndex(ind);
                p->blockSignals(false);
            }
        }
    }
}

void PreviewsDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
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

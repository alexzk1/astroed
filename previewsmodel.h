#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QFileInfo>

#include "lua/lua_gen.h"
#include "utils/runners.h"
#include "previewsmodeldata.h"

class QFileInfo;
#ifdef USING_VIDEO_FS
extern const QStringList supportedVids;
#endif

class PreviewsModel : public QAbstractTableModel, public luavm::ProjectSaver, public luavm::ProjectLoader
{
    Q_OBJECT
public:

    explicit PreviewsModel(QObject *parent = 0);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void guessDarks();

    utility::runner_t pickBests(int from, int to);
    utility::runner_t pickBests();


    void setAllRole(int role_id); //should be same order as in fileRoles
    void setRoleFor(const QString& fileName, int role_id);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setCurrentFolder(const QString& path, bool recursive = false);
    void simulateModelReset();
    void resetModel();
    void scrolledTo(int64_t row);

    virtual void generateProjectCode(std::ostream& out) const override;
    virtual void loadProjectCode(const std::string& src) override;

    bool static isParsingVideo();
    int  static getSpecialColumnId(); //this column may have automatic changes in loop
    virtual ~PreviewsModel();
private slots:
    void onTimerDelayedLoader();
private:
    using files_t  = std::vector<QFileInfo>;
    using mfiles_t = std::vector<PreviewsModelData>;

    utility::runner_t listFiles;
    utility::runner_t loadPreviews;
    mutable std::recursive_mutex listMut;
    mfiles_t  modelFiles;
    mutable std::atomic<int64_t> urgentRowScrolled;
    std::atomic<int64_t> modelFilesAmount;
    std::map<int, QStringList> fixedCombosLists; //wana to do generic solution
    QTimer scrollDelayedLoader;
    QFileInfo currentFolder;

    void haveFilesList(const files_t& list, const utility::runnerint_t& stop);
    void loadCurrentInterval();
    bool setDataPriv(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    void setRoleForPriv(const QString &fileName, int role_id);
signals:
    void startedPreviewsLoad(bool scroll);
    void finishedPreviewsLoad();
    void loadProgress(int percents);
    void filesAreListed(const QString& rootPath);
    void loadedProject(const QString& root, bool recursive);
};


class PreviewsDelegate : public QStyledItemDelegate
{
public:
    PreviewsDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}

    bool showLastClickedPreview(int shift, const QSize &lastSize);
protected:
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
private:
    QModelIndex lastClickedPreview;
};

#pragma once

#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <queue>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "lua/lua_gen.h"
#include "utils/runners.h"
#include "previewsmodeldata.h"

class QFileInfo;
class PreviewsModel : public QAbstractTableModel, public luavm::LuaGenerator
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
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    void guessDarks();

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setCurrentFolder(const QString& path, bool recursive = false);
    void simulateModelReset();
    void scrolledTo(int64_t row);
    virtual void generateLuaCode(std::ostream& out) const override;

    bool static isParsingVideo();

    virtual ~PreviewsModel();
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
    void haveFilesList(const files_t& list, const utility::runnerint_t& stop);
    void loadCurrentInterval();
signals:
    void startedPreviewsLoad(bool scroll);
    void finishedPreviewsLoad();
    void loadProgress(int percents);
};


class PreviewsDelegate : public QStyledItemDelegate
{
public:
    PreviewsDelegate(QWidget *parent = 0) : QStyledItemDelegate(parent) {}

    bool showLastClickedPreview(int shift = 0, bool reset = true);
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

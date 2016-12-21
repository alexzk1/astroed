#pragma once

#include <vector>
#include <mutex>
#include <memory>

#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "utils/runners.h"
#include "previewsmodeldata.h"

class QFileInfo;
class PreviewsModel : public QAbstractTableModel
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

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void setCurrentFolder(const QString& path, bool recursive = false);
    void simulateModelReset();
    virtual ~PreviewsModel();
private:
    using files_t  = std::vector<QFileInfo>;
    using mfiles_t = std::vector<PreviewsModelData>;

    utility::runner_t listFiles;
    utility::runner_t loadPreviews;
    std::recursive_mutex listMut;
    mfiles_t  modelFiles;

    void haveFilesList(const files_t& list);
signals:
    void startedPreviewsLoad();
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
private:
    QModelIndex lastClickedPreview;
};

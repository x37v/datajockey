#ifndef TAGMODEL_H
#define TAGMODEL_H

#include <QAbstractItemModel>
#include <QMimeData>
#include "db.h"

class TagModel : public QAbstractItemModel {
  Q_OBJECT
  public:
    explicit TagModel(DB * db, QObject *parent = 0);
    virtual ~TagModel();

    void showAllTags(bool doit);

    virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex & index) const;
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    bool canDelete(const QModelIndex& index) const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData * mimeData(const QModelIndexList & indexes) const;
    static int idColumn() { return 1; }
  public slots:
    void setWork(int id);
    void createTag(QString name, QModelIndex parent);
  signals:
    void errorCreatingTag(QString errorString);

  private:
    DB * mDB;
    int mWorkID = 0; //zero means no work
    bool mShowAllTags = false;
    Tag * mTagRoot = nullptr;
};

//our own mime type for ids
class TagModelItemMimeData : public QMimeData {
  Q_OBJECT
  public:
    TagModelItemMimeData();
    static QString format();
    virtual QStringList formats() const;
    virtual bool hasFormat(const QString & mimeType) const;
    virtual QVariant retrieveData(const QString & mimeType, QVariant::Type type) const;
    void addItem(int id);
  private:
    QList<int> mData;
};

#endif // TAGMODEL_H

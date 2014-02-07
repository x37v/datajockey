#ifndef TAGMODEL_H
#define TAGMODEL_H

#include <QAbstractItemModel>
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
  signals:

  public slots:
    void setWork(int id);
  private:
    DB * mDB;
    int mWorkID = 0; //zero means no work
    bool mShowAllTags = false;
    Tag * mTagRoot = nullptr;
};

#endif // TAGMODEL_H

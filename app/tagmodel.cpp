#include "tagmodel.h"

TagModel::TagModel(DB *db, QObject *parent) :
  QAbstractItemModel(parent),
  mDB(db)
{
  mTagRoot = new Tag();
}

TagModel::~TagModel() {
  if (mTagRoot)
    delete mTagRoot;
}

void TagModel::showAllTags(bool doit) {
  beginResetModel();
  mShowAllTags = doit;
  Tag * c = mTagRoot->removeChildAt(0);
  while (c != nullptr) {
    delete c;
    c = mTagRoot->removeChildAt(0);
  }

  QList<Tag *> tags;
  if (mShowAllTags)
    tags = mDB->tags();
  else if (mWorkID)
    tags = mDB->tags(mWorkID);

  if (tags.length()) {
    for (int i = 0; i < tags.length(); i++)
      mTagRoot->appendChild(tags[i]);
  }

  endResetModel();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex & parent) const {
  if (!hasIndex(row, column, parent) || column != 0)
    return QModelIndex();

  Tag * parentTag = nullptr;
  if (!parent.isValid())
    parentTag = mTagRoot;
  else
    parentTag = static_cast<Tag *>(parent.internalPointer());

  Tag * child = parentTag->child(row);
  if (child)
    return createIndex(row, column, child);

  return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex & index) const {
  if (!index.isValid())
    return QModelIndex();

  Tag * tag = static_cast<Tag *>(index.internalPointer());
  if (tag->parent() == mTagRoot || tag->parent() == nullptr)
    return QModelIndex();

  int row = tag->parent()->childIndex(tag);
  return createIndex(row, 0, tag->parent());
}

int TagModel::rowCount(const QModelIndex & parent) const {
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid())
    return mTagRoot->children().size();
  Tag * tag = static_cast<Tag *>(parent.internalPointer());
  return tag->children().size();
}

int TagModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

QVariant TagModel::data(const QModelIndex & index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole)
    return QVariant();
  Tag * tag = static_cast<Tag *>(index.internalPointer());
  return tag->name();
}

void TagModel::setWork(int id) {
  beginResetModel();
  mWorkID = id;
  Tag * c = mTagRoot->removeChildAt(0);
  while (c != nullptr) {
    delete c;
    c = mTagRoot->removeChildAt(0);
  }
  if (mWorkID) {
    QList<Tag *> tags;
    tags = mDB->tags(mWorkID);
    for (int i = 0; i < tags.length(); i++)
      mTagRoot->appendChild(tags[i]);
  }
  endResetModel();
}


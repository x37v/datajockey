#include "tagmodel.h"

TagModel::TagModel(DB *db, QObject *parent) :
  QAbstractItemModel(parent),
  mDB(db)
{
}

void TagModel::showAllTags(bool doit) {
  beginResetModel();
  mShowAllTags = doit;
  if (mShowAllTags)
    mTags = mDB->tags();
  else if (mWorkID)
    mTags = mDB->tags(mWorkID);
  else
    mTags.clear();
  mTagParents.clear();
  for (auto t: mTags.toStdList()) {
    for (auto c: t->children().toStdList()) {
      mTagParents[c] = t;
    }
  }
  endResetModel();
}

QModelIndex TagModel::index(int row, int column, const QModelIndex & parent) const {
  if (!parent.isValid()) {
    if (row < mTags.size() && column == 0)
      return createIndex(row, column, mTags[row]);
  } else {
    Tag * tag = static_cast<Tag *>(parent.internalPointer());
    if (row < tag->children().size())
      return createIndex(row, column, tag->children().at(row));
  }

  return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex & index) const {
  Tag * tag = static_cast<Tag *>(index.internalPointer());
  auto it = mTagParents.find(tag);
  if (it == mTagParents.end())
    return QModelIndex();

  Tag * parent = *it;
  int parent_row = mTags.indexOf(parent);
  return createIndex(parent_row, 0, parent);
}

//currently only 2 levels deep
int TagModel::rowCount(const QModelIndex & parent) const {
  if (!parent.isValid())
    return mTags.length();
  else if (parent.row() < mTags.length())
    return mTags[parent.row()]->children().length();
  return 0;
}

int TagModel::columnCount(const QModelIndex& parent) const {
  if (!parent.isValid())
    return 1;
  else if (parent.parent().isValid())
    return 1;
  return 0;
}

QVariant TagModel::data(const QModelIndex & index, int /*role*/) const {
  Tag * tag = static_cast<Tag *>(index.internalPointer());
  return tag->name();
}

void TagModel::setWork(int id) {
  beginResetModel();
  mWorkID = id;
  if (mWorkID)
    mTags = mDB->tags(mWorkID);
  else
    mTags.clear();
  mTagParents.clear();
  for (auto t: mTags) {
    for (auto c: t->children()) {
      mTagParents[c] = t;
    }
  }
  endResetModel();
}


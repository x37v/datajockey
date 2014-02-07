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
  if (!parent.isValid()) {
    return createIndex(row, column, mTagRoot);
  } else {
    Tag * tag = static_cast<Tag *>(parent.internalPointer());
    if (row == tag->children().size())
      return createIndex(row, column, tag->children().at(row));
  }

  return QModelIndex();
}

QModelIndex TagModel::parent(const QModelIndex & index) const {
  Tag * tag = static_cast<Tag *>(index.internalPointer());
  int row = 0;
  if (tag->parent())
    row = tag->parent()->childIndex(tag);
  return createIndex(row, 0, tag);
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
  if (!parent.isValid()) {
    return 1;
  }
  Tag * tag = static_cast<Tag *>(parent.internalPointer());
  if (tag->children().size())
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


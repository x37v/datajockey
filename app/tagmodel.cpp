#include "tagmodel.h"

#define COUNT_COL 2

TagModel::TagModel(DB *db, QObject *parent) :
  QAbstractItemModel(parent),
  mDB(db)
{
  mTagRoot = new Tag(0, "tags");
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
  if (!hasIndex(row, column, parent) || column >= COUNT_COL)
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
  if (parent.column() >= COUNT_COL)
    return 0;

  if (!parent.isValid())
    return mTagRoot->children().size();
  Tag * tag = static_cast<Tag *>(parent.internalPointer());
  return tag->children().size();
}

int TagModel::columnCount(const QModelIndex& /*parent*/) const {
  return COUNT_COL;
}

QVariant TagModel::data(const QModelIndex & index, int role) const {
  if (!index.isValid() || role != Qt::DisplayRole)
    return QVariant();
  Tag * tag = static_cast<Tag *>(index.internalPointer());
  if (index.column() == 0)
    return tag->name();
  else
    return QString::number(tag->id());
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

void TagModel::createTag(QString tagName, QModelIndex parent) {
  Tag * parentTag = nullptr;
  //get the parent if there is one
  if (parent.isValid()) {
    //XXX for now, we just allow 1 level of tag depth
    if (parent.parent().isValid())
      parent = parent.parent();
    parentTag = static_cast<Tag *>(parent.internalPointer());
  } else
    parentTag = mTagRoot;

  if (mDB->tag_exists(tagName, parentTag)) {
    QString warning = "Tag with name '" + tagName + "'";
    if (parentTag)
      warning += " and parent '" + parentTag->name() + "'";
    warning += " already exists";
    emit(errorCreatingTag(warning));
    return;
  }

  int row = rowCount(parent);
  beginInsertRows(parent, row, row);
  /*Tag * tag =*/ mDB->tag_create(tagName, parentTag);
  //parent now has tag as a child, I think we don't have to do anything else..
  endInsertRows();
}

Qt::ItemFlags TagModel::flags(const QModelIndex &index) const {
  Qt::ItemFlags defaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (!index.isValid())
    return 0;
  //if it isn't a tag class then it can be dragged
  if (index.parent().isValid())
    return Qt::ItemIsDragEnabled | defaultFlags;
  else
    return defaultFlags;
}

bool TagModel::canDelete(const QModelIndex& index) const {
  return (index.parent().isValid());
}

Qt::DropActions TagModel::supportedDropActions() const {
  return Qt::CopyAction;
}

QStringList TagModel::mimeTypes() const {
  QStringList types;
  types << "application/tag-id-list";
  return types;
}

QMimeData * TagModel::mimeData(const QModelIndexList & indexes) const {
  TagModelItemMimeData * data = new TagModelItemMimeData;
  foreach(QModelIndex index, indexes){
    Tag * tag = static_cast<Tag *>(index.internalPointer());
    data->addItem(tag->id());
  }
  return data;
}

TagModelItemMimeData::TagModelItemMimeData(){
}

QStringList TagModelItemMimeData::formats() const {
  QStringList types;
  types << format();
  return types;
}

bool TagModelItemMimeData::hasFormat(const QString & mimeType) const {
  if(mimeType == format())
    return true;
  else
    return false;
}

QVariant TagModelItemMimeData::retrieveData(const QString & mimeType, QVariant::Type type) const {
  if(mimeType == format() && type == QVariant::List){
    QList<QVariant> intList;
    for(int i = 0; i < mData.size(); i++)
      intList.push_back(mData[i]);
    return QVariant::fromValue(intList);
  } else 
    return QVariant();
}

void TagModelItemMimeData::addItem(int id){ mData.push_back(id); }
QString TagModelItemMimeData::format() { return "application/tag-id-list"; }

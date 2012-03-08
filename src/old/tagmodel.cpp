/*
 *		Copyright (c) 2008 Alex Norman.  All rights reserved.
 *		http://www.x37v.info/datajockey
 *
 *		This file is part of Data Jockey.
 *		
 *		Data Jockey is free software: you can redistribute it and/or modify it
 *		under the terms of the GNU General Public License as published by the
 *		Free Software Foundation, either version 3 of the License, or (at your
 *		option) any later version.
 *		
 *		Data Jockey is distributed in the hope that it will be useful, but
 *		WITHOUT ANY WARRANTY; without even the implied warranty of
 *		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *		Public License for more details.
 *		
 *		You should have received a copy of the GNU General Public License along
 *		with Data Jockey.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QVariant>
#include <QList>
#include "tagmodel.hpp"
#include "treeitem.h"
#include <QSqlRecord>
#include <QStringList>

#define ID_COL 1
#define COUNT_COL 2


/*
select tags.id id,
        tag_classes.id class_id,
        tag_classes.name class,
        tags.name name,
        count(audio_work_tags.id) count
from tags
        inner join tag_classes on tags.tag_class_id = tag_classes.id
        left join audio_work_tags on audio_work_tags.tag_id = tags.id
group by tags.name
order by class, name
*/


TagModelItemMimeData::TagModelItemMimeData(){
}

QStringList TagModelItemMimeData::formats() const {
	QStringList types;
	types << "application/tag-id-list";
	return types;
}

bool TagModelItemMimeData::hasFormat(const QString & mimeType ) const {
	if(mimeType == "application/tag-id-list")
		return true;
	else
		return false;
}

QVariant TagModelItemMimeData::retrieveData ( const QString & mimeType, QVariant::Type type ) const {
	if(mimeType == "application/tag-id-list" && type == QVariant::List){
		QList<QVariant> intList;
		for(int i = 0; i < mData.size(); i++)
			intList.push_back(mData[i]);
		return QVariant::fromValue(intList);
	} else 
		return QVariant();
}

void TagModelItemMimeData::addItem(int id){
	mData.push_back(id);
}


const QString TagModel::cQueryStr(
		"select tags.id id,\n"
		"\ttag_classes.id class_id,\n"
		"\ttag_classes.name class,\n"
		"\ttags.name name,\n"
		"\tcount(audio_work_tags.id) count\n"
		"from tags\n"
		"\tinner join tag_classes on tags.tag_class_id = tag_classes.id\n"
		"\tleft join audio_work_tags on audio_work_tags.tag_id = tags.id\n"
		"group by tags.name\n"
		"order by class, name\n");

TagModel::TagModel(const QSqlDatabase & db, QObject * parent) :
	TreeModel(parent), mQuery(cQueryStr, db), mAddTagQuery("", db)
{
	mDb = db;
	mClassList = new QList<QPair<int, QString> *>;
	mQuery.exec(cQueryStr);
	QList<QVariant> rootData;
	rootData << "empty";
	setRoot(new TreeItem(rootData));
	buildFromQuery();
	setSupportedDragActions(Qt::ActionMask);
}

TagModel::~TagModel(){
	//XXX delete mClassList and its contents;
}

QSqlDatabase TagModel::db() const {
	return mDb;
}

int TagModel::idColumn(){
	return ID_COL;
}

int TagModel::countColumn(){
	return COUNT_COL;
}

QList<QPair<int, QString> *> * TagModel::classList() const {
	return mClassList;
}

Qt::ItemFlags TagModel::flags(const QModelIndex &index) const {
	Qt::ItemFlags defaultFlags = TreeModel::flags(index);

	if (index.isValid()) {
		//if it isn't a tag class then it can be dragged
		if(isTag(index))
			return Qt::ItemIsDragEnabled | defaultFlags;
		else
			return defaultFlags;
	} else
		return defaultFlags;
}

QStringList TagModel::mimeTypes() const {
	QStringList types;
	types << "application/tag-id-list";
	return types;
}

QMimeData * TagModel::mimeData ( const QModelIndexList & indexes ) const {
	TagModelItemMimeData * data = new TagModelItemMimeData;
	foreach(QModelIndex index, indexes){
		//add the index to our data list, only if it isn't a class
		if(isTag(index))
			data->addItem(index.sibling(index.row(), ID_COL).data().toInt());
	}
	return data;
}

bool TagModel::isTag(const QModelIndex index) const {
	if(index.isValid() && 
			index.parent().isValid() &&
			index.parent().internalPointer() != root()){
		return true;
	} else
		return false;
}

//simply find the first tag with this name
int TagModel::find(std::string tag_name){
	if(mQuery.first()){
		QSqlRecord rec = mQuery.record();
		for(int i = 0; i < root()->childCount(); i++){
			TreeItem * classItem = root()->child(i);
			for(int j = 0; j < classItem->childCount(); j++){
				TreeItem * tagItem = classItem->child(j);
				if(tagItem->data(0).toString().toStdString() == tag_name)
					return tagItem->data(ID_COL).toInt();
			}
		}
	}
	return -1;
}

//find the first tag with the class name and tag name given
int TagModel::find(std::string tag_name, std::string class_name){
	if(mQuery.first()){
		QSqlRecord rec = mQuery.record();
		for(int i = 0; i < root()->childCount(); i++){
			TreeItem * classItem = root()->child(i);
			if(classItem->data(0).toString().toStdString() == class_name){
				for(int j = 0; j < classItem->childCount(); j++){
					TreeItem * tagItem = classItem->child(j);
					if(tagItem->data(0).toString().toStdString() == tag_name)
						return tagItem->data(ID_COL).toInt();
				}
			}
		}
	}
	return -1;
}

void TagModel::addWorkTagAssociation(int work_id, int tag_id){
	QString queryStr;
	//first make sure this association doesn't already exist
	queryStr.sprintf("select * from tags\n"
			"\tjoin audio_work_tags on tags.id = audio_work_tags.tag_id\n"
			"where tags.id = %d AND audio_work_tags.audio_work_id = %d", tag_id, work_id);
	mAddTagQuery.exec(queryStr);
	//if it doesn't exist, then insert it
	if(!mAddTagQuery.first()){
		queryStr.sprintf("insert into audio_work_tags (audio_work_id, tag_id) values(%d, %d)", work_id, tag_id);
		mAddTagQuery.exec(queryStr);
		emit(tagAssociationMade(work_id, tag_id));
		//update association count
		updateTagCount(tag_id);
	}
}

void TagModel::removeWorkTagAssociation(int work_id, int tag_id){
	QString queryStr;
	queryStr.sprintf("delete from audio_work_tags where tag_id = %d AND audio_work_id = %d", tag_id, work_id);
	mAddTagQuery.exec(queryStr);

	//update association count
	updateTagCount(tag_id);
}

void TagModel::removeWorkTagAssociation(int work_id, QModelIndex tag_index){
	if(isTag(tag_index)){
		int tag_id = tag_index.sibling(tag_index.row(), ID_COL).data().toInt();
		removeWorkTagAssociation(work_id, tag_id);
	}
}

void TagModel::addTag(int classId, QString tagName){
	QString queryStr;

	//don't add a dup tag
	queryStr.sprintf("select * from tags where name = \"%s\" and tag_class_id = %d", tagName.toStdString().c_str(), classId);
	mAddTagQuery.exec(queryStr);
	if(mAddTagQuery.first())
		return;

	queryStr.sprintf("insert into tags (name, tag_class_id) values(\"%s\", %d)", tagName.toStdString().c_str(), classId);
	mAddTagQuery.exec(queryStr);

	//grab the tag id
	queryStr.sprintf("select id from tags where name = \"%s\" and tag_class_id = %d", tagName.toStdString().c_str(), classId);
	mAddTagQuery.exec(queryStr);

	if(mAddTagQuery.first()){
		//find the parent
		TreeItem * parent = NULL;
		//classes are children of the root
		for(int i = 0; i < root()->childCount(); i++){
			TreeItem * child = root()->child(i);
			//if the child's id is the same as our class id.. we've found it
			if(child->data(TagModel::idColumn()).toInt() == classId){
				parent = child;
				break;
			}
		}
		if(parent != NULL){
			beginInsertRows(createIndex(parent->row(), 0, parent), parent->childCount(), parent->childCount());
			QList<QVariant> itemData;
			itemData << tagName << mAddTagQuery.value(0).toInt();
			//this tag was just added, so there are zero associations
			itemData << 0;
			TreeItem * newTag = new TreeItem(itemData, parent);
			parent->appendChild(newTag);
			endInsertRows();
		} 
	} 

	//mQuery.exec(cQueryStr);
	//buildFromQuery();
}

void TagModel::addClassAndTag(QString className, QString tagName){
	QString queryStr;

	//make sure we don't already have the class
	queryStr.sprintf("select id from tag_classes where name = \"%s\"", 
			className.toStdString().c_str());
	mAddTagQuery.exec(queryStr);
	//if we already have that class then we just call the addTag with that
	//class_id and the tagName
	if(mAddTagQuery.first()){
		addTag(mAddTagQuery.value(0).toInt(), tagName);
		return;
	}

	//insert the tag class
	queryStr.sprintf("insert into tag_classes set name=\"%s\"", className.toStdString().c_str());
	mAddTagQuery.exec(queryStr);

	//insert the tag
	queryStr.sprintf("insert into tags set name=\"%s\", tag_class_id = (select id from tag_classes where name = \"%s\")",
			tagName.toStdString().c_str(), className.toStdString().c_str());
	mAddTagQuery.exec(queryStr);

	queryStr.sprintf("select tags.id id, tags.tag_class_id class_id from tags\n"
			"\njoin tag_classes on tags.tag_class_id = tag_classes.id\n"
			"where tags.name = \"%s\" and tag_classes.name = \"%s\"",
			tagName.toStdString().c_str(), className.toStdString().c_str());
	mAddTagQuery.exec(queryStr);
	
	//if we got the data then create our new items
	if(mAddTagQuery.first()){
		//QModelIndex rootIndex = createIndex(root()->row(), 0, root());
		//beginInsertRows(rootIndex, root()->childCount(), root()->childCount());

		//populate the tag class and tag item data..
		QList<QVariant> parentData;
		QList<QVariant> itemData;
		parentData << className << mAddTagQuery.value(1).toInt();
		//put an empty string at the end for count
		parentData << "";
		itemData << tagName << mAddTagQuery.value(0).toInt();
		itemData << 0;

		//create the nodes and insert them
		TreeItem * parent = new TreeItem(parentData, root());
		parent->appendChild(new TreeItem(itemData, parent));
		root()->appendChild(parent);

		//endInsertRows();
		reset();

		QPair<int, QString> classInfo(mAddTagQuery.value(1).toInt(), className);
		emit(classAdded(classInfo));
	}
}

void TagModel::buildFromQuery(){
	mClassList->clear();
	if(mQuery.first()){
		QSqlRecord rec = mQuery.record();
		int idCol = rec.indexOf("id");
		int classIdCol = rec.indexOf("class_id");
		int classCol = rec.indexOf("class");
		int nameCol = rec.indexOf("name");
		int countCol = rec.indexOf("count");

		QList<QVariant> rootData;
		rootData << "tag (class -> name)" << "id";
		rootData << "associations";
		TreeItem * newRoot = new TreeItem(rootData);

		//create the first class item
		QList<QVariant> parentData;
		parentData << mQuery.value(classCol).toString() << mQuery.value(classIdCol).toInt() ;
		//put an empty string at the end for count
		parentData << "";
		TreeItem * curParent = new TreeItem(parentData, newRoot);
		newRoot->appendChild(curParent);

		QPair<int, QString> * newClassListItem = new QPair<int, QString>(
				mQuery.value(classIdCol).toInt(),
				mQuery.value(classCol).toString());
		mClassList->push_back(newClassListItem);

		do {
			QList<QVariant> childData;
			int classId = mQuery.value(classIdCol).toInt();
			int id = mQuery.value(idCol).toInt();
			QString name(mQuery.value(nameCol).toString());
			QString className(mQuery.value(classCol).toString());

			//do we need a new parent? if so create it
			if(classId != parentData[1].toInt()){
				parentData.clear();
				parentData << className << classId;
				//put an empty string at the end for count
				parentData << "";
				curParent = new TreeItem(parentData, newRoot);
				newRoot->appendChild(curParent);
				newClassListItem = new QPair<int, QString>(classId, className);
				mClassList->push_back(newClassListItem);
			}
			//create the child (tag)
			childData << name << id << mQuery.value(countCol).toInt();
			curParent->appendChild(new TreeItem(childData, curParent));
		} while(mQuery.next());
		setRoot(newRoot);
	} else {
	}
	reset();
}	

//update the count 
void TagModel::updateTagCount(int tag_id){
	QString queryStr = 
		QString("select count(audio_work_tags.id), tag_classes.id class_id from tags\n"
				"\tjoin tag_classes on tags.tag_class_id = tag_classes.id\n"
				"\tleft join audio_work_tags on tags.id = audio_work_tags.tag_id\n"
				"where tags.id = %1 group by tag_classes.id;").arg(tag_id);
	mAddTagQuery.exec(queryStr);
	if(mAddTagQuery.first()){
		//grab the count
		int count = mAddTagQuery.value(0).toInt();
		int class_id = mAddTagQuery.value(1).toInt();
		//find the class
		for(int i = 0; i < root()->childCount(); i++){
			if(root()->child(i)->data(ID_COL).toInt() == class_id){
				//find the tag
				for(int j = 0; j < root()->child(i)->childCount(); j++){
					TreeItem * parent = root()->child(i);
					if(parent->child(j)->data(ID_COL).toInt() == tag_id){
						QModelIndex index = createIndex(parent->child(j)->row(), COUNT_COL, parent->child(j));
						//if we can set the data, do it, and emit the signal that we've done it
						if(parent->child(j)->setData(COUNT_COL, count))
							emit(dataChanged(index,index));
						break;
					}
				}
				break;
			}
		}
	}
}


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

#include "worktagmodelfilter.hpp"
#include "tagmodel.hpp"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QMimeData>
#include <QStringList>
#include <QString>

WorkTagModelFilter::WorkTagModelFilter(TagModel * model):
	QSortFilterProxyModel(model), mWorkTagsQuery("", model->db())
{
	setSourceModel(model);
	mModel = model;
	mWork = -1;
}

int WorkTagModelFilter::work(){
	return mWork;
}

TagModel * WorkTagModelFilter::sourceTagModel(){
	return mModel;
}

Qt::DropActions WorkTagModelFilter::supportedDropActions() const {
	return Qt::CopyAction;
}

Qt::ItemFlags WorkTagModelFilter::flags(const QModelIndex &index) const {
	Qt::ItemFlags defaultFlags = sourceModel()->flags(index);
	return Qt::ItemIsDropEnabled | defaultFlags;
}

//accept the drop data!
bool WorkTagModelFilter::dropMimeData( const QMimeData * data, 
		Qt::DropAction action, int row, int column, 
		const QModelIndex & parent ){
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	if(mWork < 0)
		return false;

	if(action == Qt::IgnoreAction)
		return true;

	if(!data->hasFormat("application/tag-id-list"))
		return false;
	//cast it!
	const TagModelItemMimeData *tagModelData = qobject_cast<const TagModelItemMimeData *>(data);
	if(tagModelData){
		//iterate over our data
		foreach(QVariant id, tagModelData->retrieveData("application/tag-id-list", QVariant::List).toList()){
			mModel->addWorkTagAssociation(mWork, id.toInt());
		}
		refilter();
		return true;
	} else
		return false;
}

void WorkTagModelFilter::setWork(int work){
	if(mWork != work){
		mWork = work;
		refilter();
		emit(workChanged(mWork));
	}
}

void WorkTagModelFilter::clear() {
	mWork = -1;
	mTagClassIds.clear();
	mTagIds.clear();
	invalidateFilter();
	emit(workChanged(mWork));
}

void WorkTagModelFilter::refilter() {
	QString queryStr("select tags.id id, tags.tag_class_id class_id from tags\n"
			"\tjoin audio_work_tags on audio_work_tags.tag_id = tags.id\n"
			"\tjoin audio_works on audio_works.id = audio_work_tags.audio_work_id\n"
			"\nwhere audio_works.id = ");
	QString id;
	id.setNum(mWork);
	queryStr.append(id);

	mTagClassIds.clear();
	mTagIds.clear();
	//populate the mTagClassIds and mTagIds sets
	mWorkTagsQuery.exec(queryStr);
	if(mWorkTagsQuery.first()){
		do {
			mTagIds.insert(mWorkTagsQuery.value(0).toInt());
			mTagClassIds.insert(mWorkTagsQuery.value(1).toInt());
		} while(mWorkTagsQuery.next());
	}

	invalidateFilter();
}

bool WorkTagModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
	//if my parent's model is the source model then this is a tag not a tag class
	if(sourceParent.model() == sourceModel()){
		QModelIndex rowIndex = sourceModel()->index(sourceRow, TagModel::idColumn(), sourceParent);
		if(mTagIds.find(rowIndex.data().toInt()) != mTagIds.end())
			return true;
		else
			return false;
	} else {
		//this is a parent..
		QModelIndex rowIndex = sourceModel()->index(sourceRow, TagModel::idColumn(), sourceParent);
		if(mTagClassIds.find(rowIndex.data().toInt()) != mTagClassIds.end())
			return true;
		else
			return false;
	}
}


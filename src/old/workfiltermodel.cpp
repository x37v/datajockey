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

#include "workfiltermodel.hpp"
#include "worktablemodel.hpp"

WorkFilterModel::WorkFilterModel(QObject * parent) :
	QObject(parent)
{
	qRegisterMetaType<WorkFilterModel *>("WorkFilterModel *");
}

WorkFilterModel::~WorkFilterModel(){
}

bool WorkFilterModel::beforeFilter(){
	//default, always filter
	return true;
}

WorkFilterModelProxy::WorkFilterModelProxy(WorkTableModel * parent) : 
	QSortFilterProxyModel(parent)
{
	mFilter = NULL;
	setSourceModel(parent);
}

bool WorkFilterModelProxy::lessThan(const QModelIndex &left,
		const QModelIndex &right) const {
	QVariant leftData = sourceModel()->data(left);
	QVariant rightData = sourceModel()->data(right);
	if(leftData.canConvert(QVariant::String)){
		//get the data
		QString leftArtist = 
			sourceModel()->data(left.sibling(left.row(), WorkTableModel::artistColumn)).toString().toCaseFolded();
		QString leftAlbum = 
			sourceModel()->data(left.sibling(left.row(), WorkTableModel::albumColumn)).toString().toCaseFolded();
		QString leftTitle = 
			sourceModel()->data(left.sibling(left.row(), WorkTableModel::titleColumn)).toString().toCaseFolded();
		int leftTrack = sourceModel()->data(left.sibling(left.row(), WorkTableModel::trackColumn)).toInt();
		QString rightArtist = 
			sourceModel()->data(right.sibling(right.row(), WorkTableModel::artistColumn)).toString().toCaseFolded();
		QString rightAlbum = 
			sourceModel()->data(right.sibling(right.row(), WorkTableModel::albumColumn)).toString().toCaseFolded();
		QString rightTitle = 
			sourceModel()->data(right.sibling(right.row(), WorkTableModel::titleColumn)).toString().toCaseFolded();
		int rightTrack = sourceModel()->data(right.sibling(right.row(), WorkTableModel::trackColumn)).toInt();

		//we sort specially for artist, title and album
		//artist, album, track, title
		if((unsigned int)left.column() == WorkTableModel::artistColumn){
			if(leftArtist != rightArtist)
				return leftArtist < rightArtist;
			if(leftAlbum != rightAlbum)
				return leftAlbum < rightAlbum;
			if(leftTrack != rightTrack)
				return leftTrack < rightTrack;
			return leftTitle < rightTitle;
		//title, artist, album
		} else if((unsigned int)left.column() == WorkTableModel::titleColumn){
			if(leftTitle != rightTitle)
				return leftTitle < rightTitle;
			if(leftArtist != rightArtist)
				return leftArtist < rightArtist;
			return leftAlbum < rightAlbum;
		//album, track, artist, title
		} else if((unsigned int)left.column() == WorkTableModel::albumColumn){
			if(leftAlbum != rightAlbum)
				return leftAlbum < rightAlbum;
			if(leftTrack != rightTrack)
				return leftTrack < rightTrack;
			if(leftArtist != rightArtist)
				return leftArtist < rightArtist;
			return leftTitle < rightTitle;
		}
		//otherwise just compare strings
		return leftData.toString() < rightData.toString();
	} else if(leftData.canConvert(QVariant::Double)){
		return leftData.toDouble() < rightData.toDouble();
	} else
		return true;
}

WorkFilterModel * WorkFilterModelProxy::workFilter() const {
	return mFilter;
}

void WorkFilterModelProxy::setFilter(WorkFilterModel * model){
	mFilter = model;
	mFiltering = true;
}

bool WorkFilterModelProxy::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const {
	if(!mFiltering || !mFilter)
		return true;
	else {
		QModelIndex rowIndex = sourceModel()->index(sourceRow, WorkTableModel::idColumn, sourceParent);
		return mFilter->acceptsWork(rowIndex.data().toInt());
	}
}

void WorkFilterModelProxy::filter(bool filter){
	if(mFiltering != filter){
		mFiltering = filter;
		//if we're not filtering then we need to invalidate
		if(!mFiltering){
			invalidateFilter();
			return;
		}
	}

	//test to see if we actually need to filter
	//if we have a filter model and its before filter returns true
	//then we filter
	if(mFiltering && mFilter && mFilter->beforeFilter())
		invalidateFilter();
}

void WorkFilterModelProxy::clearFilter(){
	if(mFiltering)
		filter(false);
}


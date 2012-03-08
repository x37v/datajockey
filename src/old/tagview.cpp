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

#include "tagview.hpp"
#include "tagmodel.hpp"
#include "treemodel.h"
#include "treeitem.h"
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QItemSelectionModel>

TagView::TagView(QAbstractItemModel * model, QWidget * parent) :
	QTreeView(parent)
{
	mModel = model;
	setModel(mModel);
	setColumnHidden(TagModel::idColumn(),true);

	//connect internal signals/slots
	QObject::connect(
			selectionModel(), SIGNAL(selectionChanged ( const QItemSelection &, const QItemSelection &)),
			this, SLOT(updateTagSelection()));
}

QList<int> TagView::selectedTagIds() const {
	QItemSelectionModel * selection = selectionModel();
	QList<int> ids;
	if(selection->hasSelection()){
		const TagModel * tagModel = qobject_cast<const TagModel *>(mModel);
		if(tagModel){
			foreach(QModelIndex index, selection->selection().indexes()){
				if(tagModel->isTag(index))
					ids.push_back(index.sibling(index.row(), TagModel::idColumn()).data().toInt()); 
			}
		}
	}
	return ids;
}

void TagView::updateTagSelection(){
	emit(tagSelectionChanged(selectedTagIds()));
}
